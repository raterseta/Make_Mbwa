//File MbwaHost.cc
#include "MbwaHost.h"
#include "BaseStation.h"
#include <bitset> // Untuk CRC


// Fungsi untuk menghitung CRC berdasarkan data dan polinomial yang diberikan
unsigned int calculateCRC(const std::string& data, unsigned int polynomial) {
    unsigned int crc = 0xFFFFFFFF; // Inisialisasi CRC dengan nilai awal standar 0xFFFFFFFF

    // Loop melalui setiap karakter dalam data untuk menghitung CRC
    for (char c : data) {
        // XOR CRC yang telah dihitung dengan karakter yang telah digeser 24 bit
        crc ^= (static_cast<unsigned int>(c) << 24);

        // Proses setiap bit dalam karakter (8 bit)
        for (int i = 0; i < 8; ++i) {
            // Cek jika bit paling signifikan (bit ke-31) CRC bernilai 1
            if (crc & 0x80000000) {
                // Jika ya, geser CRC satu posisi ke kiri dan XOR dengan polinomial
                crc = (crc << 1) ^ polynomial;
            } else {
                // Jika tidak, geser CRC satu posisi ke kiri tanpa XOR
                crc <<= 1;
            }
        }
    }

    // Kembalikan nilai CRC yang telah dihitung
    return crc;
}


namespace mbwa {

Define_Module(MbwaHost);

MbwaHost::~MbwaHost() {
    cancelAndDelete(endTxEvent);
    cancelAndDelete(updatePositionEvent);
}

void MbwaHost::initialize() {
    positionUpdateLimit = par("positionUpdateDuration").doubleValue(); // Ambil dari omnetpp.ini
    updateStartTime = simTime(); // Catat waktu mulai
    qosPriority = par("qosPriority").intValue(); // Inisialisasi nilai QoS
    EV << "Host " << getId() << " initialized with QoS Priority: " << qosPriority << "\n";
    stateSignal = registerSignal("state");

    // Dapatkan pointer ke BaseStation
    server = dynamic_cast<BaseStation*>(findModuleByPath("baseStation"));
    if (!server) {
        throw cRuntimeError("BaseStation tidak ditemukan. Pastikan nama modul sudah benar.");
    }

    txRate = par("txRate").doubleValue();
    iaTime = &par("iaTime");
    pkLenBits = &par("pkLenBits");

    // Inisialisasi posisi dan parameter pergerakan
    x = par("x").doubleValue();
    y = par("y").doubleValue();
    speed = par("speed").doubleValue();
    direction = par("direction").doubleValue();

    endTxEvent = new cMessage("send/endTx");
    updatePositionEvent = new cMessage("updatePosition");

    state = IDLE;
    emit(stateSignal, state);
    pkCounter = 0;

    // Jadwalkan event untuk update posisi pertama kali
    scheduleAt(simTime() + 1, updatePositionEvent);

    // Jadwalkan event pertama untuk transmisi paket
    scheduleAt(getNextTransmissionTime(), endTxEvent);
}

void MbwaHost::handleMessage(cMessage *msg) {
    if (msg == updatePositionEvent) {
        // Update posisi berdasarkan kecepatan dan arah
        x += speed * cos(direction * M_PI / 180.0);
        y += speed * sin(direction * M_PI / 180.0);
        refreshDisplay();

        EV << "Update posisi selesai untuk Host " << getId() << " pada t=" << simTime() << "\n";

        // Jadwalkan ulang event update posisi selama t < 30s
        if (simTime() - updateStartTime < positionUpdateLimit) {
            scheduleAt(simTime() + 1, updatePositionEvent);  // Jadwalkan ulang
        }

        // Setelah update posisi, jadwalkan koneksi ulang ke server
        if (!endTxEvent->isScheduled()) {
            scheduleAt(simTime(), endTxEvent);
        }
    } else if (msg == endTxEvent) {
        if (state == IDLE) {
            // Kirim permintaan alokasi kanal ke BaseStation
            cMessage *allocateMsg = new cMessage("allocateChannelRequest");
            allocateMsg->addPar("hostId") = getId();
            allocateMsg->addPar("qosPriority") = qosPriority;

            sendDirect(allocateMsg, server->gate("in"));
        } else if (state == TRANSMIT) {
            // Kirim pesan pelepasan subcarrier ke BaseStation
            cMessage *releaseMsg = new cMessage("releaseChannelRequest");
            releaseMsg->addPar("hostId") = getId();
            sendDirect(releaseMsg, server->gate("in"));
            // Transmisi selesai, kembali ke IDLE
            state = IDLE;
            emit(stateSignal, state);

            // Jadwalkan transmisi berikutnya setelah interval tertentu
            if (simTime() - updateStartTime < positionUpdateLimit) {
                scheduleAt(simTime() + iaTime->doubleValue(), endTxEvent);
            }
        }
    } else if (strcmp(msg->getName(), "allocateChannelResponse") == 0) {
        handleChannelResponse(msg);
    } else {
        delete msg;
    }
}



void MbwaHost::handleChannelResponse(cMessage *msg) {
    if (strcmp(msg->getName(), "allocateChannelResponse") == 0) {
        int channelId = msg->par("channelId").longValue();
        if (channelId != -1) {
            char pkname[40];
            snprintf(pkname, sizeof(pkname), "pk-%d-#%d", getId(), pkCounter++);
            cPacket *pk = new cPacket(pkname);
            pk->setBitLength(pkLenBits->intValue());

            // Hitung CRC untuk paket ini
            unsigned int crc = calculateCRC(pkname, 0x04C11DB7); // Generator CRC-32
            pk->addPar("CRC") = crc;

            sendDirect(pk, server->gate("in"));
        } else {
            EV << "Host " << getId() << " could not get a channel.\n";
        }
        delete msg;
    }
}


void MbwaHost::refreshDisplay() const {
    getDisplayString().setTagArg("p", 0, x);
    getDisplayString().setTagArg("p", 1, y);

    if (state == IDLE) {
        getDisplayString().setTagArg("i", 1, "");
    } else if (state == TRANSMIT) {
        getDisplayString().setTagArg("i", 1, "yellow");
    }
}

simtime_t MbwaHost::getNextTransmissionTime() {
    return simTime() + iaTime->doubleValue();
}

}; // namespace
