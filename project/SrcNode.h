#ifndef SRCNODE_H_
#define SRCNODE_H_

#include <omnetpp.h>
#include <vector>
#include <utility>
#include <queue>
#include "SRMessage_m.h"

using namespace omnetpp;

class SrcNode : public cSimpleModule
{
  private:
    int windowSize;
    double timeout;


    // Transmission queue: pair<message, isNewFrame> (isNewFrame is kept but not used for logging)
    std::queue<std::pair<SRMessage*, bool>> mac_queue;
    cMessage *tx_finish_timer = nullptr;

    int S_lower = 0;             // Oldest unacknowledged frame
    int S_upper = 0;             // Next frame to sequence and send
    int maxSeqNum;               // Sequence space boundary

    std::vector<cMessage*> tx_timers;
    std::vector<SRMessage*> out_buf;
    std::vector<bool> acked_frames; // Track specific ACKs inside the current window

    simsignal_t utilizationSignal;
    long totalBitsTransmitted = 0;
    long totalGoodputBits = 0;

    void sendDataFrame(int seqNum);
    void startTransmission();

  protected:
    virtual void initialize() override;
    virtual void handleMessage(cMessage *msg) override;
    virtual void finish() override;
};

#endif /* SRCNODE_H_ */
