// File BaseStation.cc
#include "BaseStation.h"
#include <algorithm> // Untuk std::sort


namespace mbwa {

Define_Module(BaseStation);

BaseStation::~BaseStation() {}


unsigned int BaseStation::calculateCRC(const std::string& data, unsigned int polynomial) {
    double currentSignal = uniform(signalStrength - 0.7, signalStrength);
    double currentNoise = uniform(0.0, noiseLevel);
    if (currentSignal < currentNoise) {
        EV << "Signal:" <<currentSignal << " dan Noise: " << currentNoise<<"\n";
        EV << "Paket gagal karena sinyal lebih lemah dari noise.\n";
        return 0;
    }

    // Inisialisasi CRC dengan nilai standar
    unsigned int crc = 0xFFFFFFFF;

    // Loop melalui setiap karakter dalam data untuk menghitung CRC
    for (char c : data) {
        // XOR CRC yang telah dihitung dengan karakter yang digeser 24 bit
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



void BaseStation::initialize() {
    numChannels = par("numChannels").intValue();
    multiplexingType = par("multiplexingType").stringValue();
    tdmSlotDuration = par("tdmSlotDuration").doubleValue();
    noiseLevel = par("noiseLevel").doubleValue(); // Ambil nilai dari parameter di file .ini
    signalStrength = par("signalStrength").doubleValue(); // Opsional, jika digunakan

    channelUsage.resize(numChannels, false);
    channelTimers.resize(numChannels, simTime());

    if (multiplexingType == "OFDM") {
        numSubcarriers = par("numSubcarriers").intValue();
        subcarrierUsage.resize(numChannels * numSubcarriers, false);
        EV << "BaseStation diinisialisasi dengan " << numChannels << " kanal dan "
           << numSubcarriers << " subcarriers menggunakan OFDM.\n";
    } else {
        EV << "BaseStation diinisialisasi dengan " << numChannels << " kanal menggunakan "
           << multiplexingType << ".\n";
    }
}
//
void BaseStation::handleMessage(cMessage *msg) {
    if (strcmp(msg->getName(), "allocateChannelRequest") == 0) {
        int hostId = msg->par("hostId").longValue();
        int channelId = -1;

        if (multiplexingType == "TDM") {
            channelId = allocateTDMChannel(hostId);
        } else if (multiplexingType == "FDM") {
            channelId = allocateFDMChannel(hostId);
        } else if (multiplexingType == "OFDM") {
            channelId = allocateOFDMChannel(hostId);
        } else {
            EV << "Jenis multiplexing tidak dikenali!\n";
        }

        cMessage *response = new cMessage("allocateChannelResponse");
        response->addPar("channelId") = channelId;
        sendDirect(response, getSimulation()->getModule(hostId)->gate("in"));
        delete msg;
    } else if (strcmp(msg->getName(), "releaseChannelRequest") == 0) {
        int hostId = msg->par("hostId").longValue();
        releaseOFDMChannel(hostId);
        delete msg;
    } else if (msg->isPacket()) {
        cPacket *pk = check_and_cast<cPacket *>(msg);
        unsigned int receivedCRC = pk->par("CRC").longValue();
        unsigned int calculatedCRC = calculateCRC(pk->getName(), 0x04C11DB7);

        if (receivedCRC != calculatedCRC) {
            EV << "CRC mismatch. Paket disimpan di HARQ buffer untuk retransmisi.\n";
            harqBuffer[pk->getId()] = pk;
            cMessage *retransmitMsg = new cMessage("retransmitHARQ");
            scheduleAt(simTime() + 0.1, retransmitMsg);
        } else {
            EV << "Paket diterima dengan CRC valid.\n";
            delete pk;
        }
    }
}
int BaseStation::allocateTDMChannel(int hostId) {
    for (int i = 0; i < numChannels; ++i) {
        if (!channelUsage[i] || simTime() >= channelTimers[i]) {
            channelUsage[i] = true;
            channelTimers[i] = simTime() + tdmSlotDuration;
            hostToChannelMap[hostId] = i;

            // Jadwalkan pelepasan kanal
            cMessage *releaseMsg = new cMessage("releaseChannel");
            releaseMsg->addPar("channelId") = i;
            scheduleAt(channelTimers[i], releaseMsg);

            EV << "Channel " << i << " allocated to Host " << hostId
               << " using TDM (expires at " << channelTimers[i] << "s).\n";
            return i;
        }
    }
    EV << "No TDM slots available for Host " << hostId << ".\n";
    return -1;
}

int BaseStation::allocateFDMChannel(int hostId) {
    for (int i = 0; i < numChannels; ++i) {
        if (!channelUsage[i]) {
            channelUsage[i] = true;
            hostToChannelMap[hostId] = i;

            // Jadwalkan pelepasan kanal
            cMessage *releaseMsg = new cMessage("releaseChannel");
            releaseMsg->addPar("channelId") = i;
            scheduleAt(simTime() + 0.5, releaseMsg);  // Atur waktu untuk pelepasan kanal (misalnya 10 detik)

            EV << "Kanal " << i << " dialokasikan ke Host " << hostId << " menggunakan FDM.\n";
            return i;
        }
    }
    EV << "Tidak ada kanal FDM tersedia untuk Host " << hostId << "\n";
    return -1;  // Tidak ada kanal tersedia
}


int BaseStation::allocateOFDMChannel(int hostId) {
    for (int channel = 0; channel < numChannels; ++channel) {
        for (int subcarrier = 0; subcarrier < numSubcarriers; ++subcarrier) {
            int index = channel * numSubcarriers + subcarrier;

            if (index < subcarrierUsage.size() && !subcarrierUsage[index]) {
                subcarrierUsage[index] = true;
                hostToChannelMap[hostId] = index;
                EV << "Kanal " << channel << ", Subcarrier " << subcarrier
                   << " dialokasikan ke Host " << hostId << " menggunakan OFDM.\n";
                return index;
            }
        }
    }
    EV << "Semua kanal dan subcarrier OFDM sibuk. Tidak ada alokasi untuk Host " << hostId << ".\n";
    return -1;
}

void BaseStation::releaseOFDMChannel(int hostId) {
    auto it = hostToChannelMap.find(hostId);
    if (it != hostToChannelMap.end()) {
        int index = it->second;
        if (index >= 0 && index < subcarrierUsage.size()) {
            subcarrierUsage[index] = false;
            EV << "Subcarrier " << index << " dilepaskan oleh Host " << hostId << ".\n";
        }
        hostToChannelMap.erase(it);
    } else {
        EV << "Host " << hostId << " tidak memiliki alokasi subcarrier untuk dilepaskan.\n";
    }
}

void BaseStation::retransmitHARQ(int hostId, cPacket *packet) {
    // Mencari paket di buffer HARQ berdasarkan ID paket yang diterima
    auto it = harqBuffer.find(packet->getId());

    // Memeriksa apakah paket ditemukan dalam buffer
    if (it != harqBuffer.end()) {
        // Jika paket ditemukan, kirim ulang paket ke host yang sesuai
        // Mengirimkan paket yang ditemukan di buffer ke gate 'in' milik host
        sendDirect(it->second, getSimulation()->getModule(hostId)->gate("in"));

        // Menampilkan pesan informasi di log bahwa retransmisi paket berhasil dilakukan
        EV_INFO << "Retransmitting packet " << packet->getId() << " untuk Host " << hostId << ".\n";

        // Menghapus paket yang telah diretransmisikan dari buffer HARQ
        harqBuffer.erase(it);
    } else {
        // Jika paket tidak ditemukan di buffer HARQ, tampilkan pesan bahwa tidak ada paket untuk retransmisi
        EV_INFO << "Tidak ada paket di HARQ buffer untuk Host " << hostId << ".\n";
    }
}


void BaseStation::releaseChannel(int channelId) {
    if (multiplexingType == "OFDM") {
        if (channelId >= 0 && channelId < subcarrierUsage.size()) {
            subcarrierUsage[channelId] = false;
            EV << "Subcarrier " << channelId << " released.\n";
        }
    } else if (multiplexingType == "FDM") {
        if (channelId >= 0 && channelId < numChannels) {
            channelUsage[channelId] = false;
            EV << "Kanal FDM " << channelId << " dilepaskan.\n";
        }
    } else {
        if (channelId >= 0 && channelId < numChannels) {
            channelUsage[channelId] = false;
            EV << "Channel " << channelId << " released.\n";
        }
    }
}


}; // namespace

