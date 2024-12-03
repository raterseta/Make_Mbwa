//File MbwaNetwork.ned
network MbwaNetwork
{
    parameters:
        int numHosts;
        double txRate @unit(bps);
        string modulationType = default("BPSK");
        double errorProbability = default(0.1);
        @display("bgi=background/terrain,s;bgb=1000,1000");

    submodules:
        baseStation: BaseStation {
            x = 500m;
            y = 500m;
            txRate = parent.txRate;
            modulationType = parent.modulationType;
            errorProbability = parent.errorProbability;
        }
        host[numHosts]: MbwaHost {
            txRate = parent.txRate;
            modulationType = parent.modulationType;
            errorProbability = parent.errorProbability;
            x = uniform(0m, 1000m);
            y = uniform(0m, 1000m);
        }
}



//File MbwaHost.ned
simple MbwaHost
{
    parameters:
//        double qosPriority = default(1);  // Prioritas QoS untuk setiap host
        int qosPriority = default(1);
        double positionUpdateLimit @unit(s) = default(10s); // Batas waktu update posisi
        @signal[state](type="long");   
        @statistic[radioState](source="state";title="Radio state";enum="IDLE=0,TRANSMIT=1";record=vector);
        double positionUpdateDuration @unit(s) = default(60s); // Durasi update posisi
        double txRate @unit(bps);                // Transmission rate
        string modulationType = default("BPSK"); // Tipe modulasi (BPSK, QPSK, QAM16, dll.)
        double errorProbability = default(0.1);  // Probabilitas error untuk kanal
        volatile int pkLenBits @unit(b);         // Packet length in bits
        volatile double iaTime @unit(s);         // Packet interarrival time
        double x @unit(m);                       // X coordinate of the host
        double y @unit(m);                       // Y coordinate of the host
        double signalStrength = default(1.0);    // Kekuatan sinyal host
        double idleAnimationSpeed;               // Kecepatan animasi saat idle
        double transmissionEdgeAnimationSpeed;   // Kecepatan animasi saat awal/akhir transmisi
        double midTransmissionAnimationSpeed;    // Kecepatan animasi saat sedang transmisi
        double speed @unit(mps) = default(30mps); // Kecepatan bergerak
        double direction @unit(deg) = default(uniform(0, 360deg)); // Arah bergerak
        bool controlAnimationSpeed = default(true);
        @display("i=device/laptop");
   gates:
        input in @directIn; // Gate input untuk menerima respons alokasi
}

//File BaseStation.ned
simple BaseStation {
    parameters:
        double signalStrength = default(1.0);  // Tambahkan kekuatan sinyal
		double noiseLevel = default(1.5);    // Tambahkan level noise
        
        int numSubcarriers = default(10); // Jumlah subcarriers OFDM
        int numChannels;
        string multiplexingType = default("TDM"); // Pilihan: "TDM" atau "FDM"
        double tdmSlotDuration @unit(s) = default(0.2s); // Durasi slot untuk TDM
        double txRate @unit(bps);
        string modulationType = default("BPSK");
        double interferenceThreshold = default(0.5);
//        double signalStrength = default(1.0);
//        double noiseLevel = default(0.01);
        double errorProbability = default(0.1);
        double x @unit(m);
        double y @unit(m);
        @display("i=device/antennatower_l");
    gates:
        input in @directIn;
}