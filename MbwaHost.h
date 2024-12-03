//File MbwaHost.h
#ifndef __MBWA_HOST_H_
#define __MBWA_HOST_H_

#include <omnetpp.h>
#include "BaseStation.h" // Include untuk BaseStation

using namespace omnetpp;

namespace mbwa {

class MbwaHost : public cSimpleModule {
  private:
    mbwa::BaseStation* server; // Pointer ke BaseStation
    int qosPriority;           // Prioritas QoS
    simtime_t radioDelay;      // Delay radio
    double txRate;             // Transmission rate
    cPar *iaTime;              // Waktu antar kedatangan paket
    cPar *pkLenBits;           // Panjang paket
    simtime_t positionUpdateLimit; // Batas waktu update posisi
    simtime_t updateStartTime;     // Waktu mulai update posisi

    enum { IDLE = 0, TRANSMIT = 1 } state; // State mesin
    simsignal_t stateSignal;
    int pkCounter;

    double x, y;                       // Posisi
    double speed, direction;           // Kecepatan dan arah
    cMessage *endTxEvent = nullptr;    // Event untuk transmisi
    cMessage *updatePositionEvent = nullptr; // Event untuk update posisi

    const double propagationSpeed = 299792458.0; // Kecepatan cahaya untuk radioDelay

  public:
    virtual ~MbwaHost();

  protected:
    virtual void initialize() override;
    virtual void handleMessage(cMessage *msg) override;
    virtual void refreshDisplay() const override;
    simtime_t getNextTransmissionTime();
    void handleChannelResponse(cMessage *msg);
};

}; // namespace mbwa

#endif
