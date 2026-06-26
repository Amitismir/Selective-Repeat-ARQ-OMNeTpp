#ifndef DSTNODE_H_
#define DSTNODE_H_

#include <omnetpp.h>
#include <vector>
#include <queue>
#include "SRMessage_m.h"

using namespace omnetpp;

class DstNode : public cSimpleModule
{
private:
    int windowSize;
    int frame_expected = 0;
    bool no_nak = true;
    int too_far = 0;

    std::vector<bool> arrived;
    std::vector<SRMessage*> in_buf;

    // Transmission queue for ACK/NAK frames
    std::queue<SRMessage*> txQueue;
    cMessage *tx_finish_timer = nullptr;

    void sendFeedback(int type, int num);
    void startTransmission();

protected:
    virtual void initialize() override;
    virtual void handleMessage(cMessage *msg) override;
    virtual void finish() override;
};

#endif /* DSTNODE_H_ */
