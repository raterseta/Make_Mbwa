//File BaseStation.h
#ifndef __BASE_STATION_H_
#define __BASE_STATION_H_

#include <omnetpp.h>
#include <map>
#include <vector>
#include <string> // Untuk menggunakan std::string

using namespace omnetpp;

namespace mbwa {

class BaseStation : public cSimpleModule
{
  private:
    double noiseLevel; // Level noise untuk menentukan keberhasilan transmisi
    double signalStrength; // Kekuatan sinyal (opsional untuk variasi)
    std::map<int, cPacket*> harqBuffer;
    unsigned int calculateCRC(const std::string& data, unsigned int polynomial);
    std::vector<int> subcarrierUsage; // Status penggunaan subcarriers
    int numSubcarriers;              // Jumlah subcarriers yang tersedia

    // Parameter utama BaseStation
    std::string multiplexingType;         // Tipe multiplexing: TDM atau FDM
    int numChannels;                      // Jumlah kanal yang tersedia
    simtime_t tdmSlotDuration;            // Durasi slot waktu untuk TDM
    std::vector<bool> channelUsage;       // Status penggunaan tiap kanal
    std::vector<simtime_t> channelTimers; // Timer untuk TDM per kanal

    // Peta untuk QoS dan kanal
    std::map<int, int> hostToChannelMap;  // Peta dari Host ID ke Channel ID
    std::map<int, int> qosMap;            // Peta dari Host ID ke Prioritas QoS

  public:
    virtual ~BaseStation();

    // Fungsi untuk alokasi dan pembebasan kanal
//    int allocateChannelByQoS();

    int allocateChannel(int hostId, int qosPriority);
    void retransmitHARQ(int hostId, cPacket *packet);

    void releaseOFDMChannel(int hostId); // Deklarasi fungsi untuk melepas subcarrier OFDM
    void releaseChannel(int hostId);

  protected:
    // Fungsi utama OMNeT++

    virtual void initialize() override;
    virtual void handleMessage(cMessage *msg) override;

    // Fungsi khusus untuk multiplexing
    int allocateChannelByQoS(int hostId);

    int allocateOFDMChannel(int hostId);             // Alokasi subcarrier untuk OFDM
    int allocateTDMChannel(int hostId);   // Alokasi kanal dengan TDM
    int allocateFDMChannel(int hostId);   // Alokasi kanal dengan FDM
};

}; // namespace mbwa

#endif
