/*
 * DstNode.cc
 *
 * Created on: Jun 22, 2026
 * Author: Notebook
 */

#include <iomanip>
#include "DstNode.h"

Define_Module(DstNode);

void DstNode::initialize() {
    windowSize = par("windowSize");
    int seqSpaceSize = 2 * windowSize;
    arrived.resize(seqSpaceSize, false);
    in_buf.resize(seqSpaceSize, nullptr);

    frame_expected = 0;
    too_far = windowSize;
    no_nak = true;

    tx_finish_timer = new cMessage("tx_finish");
}

void DstNode::sendFeedback(int type, int num) {
    SRMessage *feedback = new SRMessage(type == ACK_FRAME ? "ACK" : "NAK");
    feedback->setType(type);
    feedback->setAckNum(num);
    feedback->setBitLength(40); // 40-bit ACK/NAK frame

    txQueue.push(feedback);
    startTransmission();
}

void DstNode::startTransmission() {
    if (txQueue.empty()) return;

    cGate *outGate = gate("out");
    cChannel *ch = outGate->getTransmissionChannel();
    simtime_t txFinishTime = ch ? ch->getTransmissionFinishTime() : simTime();

    // If channel is busy, schedule wake-up
    if (txFinishTime > simTime()) {
        if (!tx_finish_timer->isScheduled()) {
            scheduleAt(txFinishTime, tx_finish_timer);
        }
        return;
    }

    // Channel free – send the next frame
    SRMessage *msg = txQueue.front();
    txQueue.pop();

    send(msg, "out");

    // Schedule next transmission if queue not empty
    if (!txQueue.empty()) {
        simtime_t nextTxFinishTime = ch ? ch->getTransmissionFinishTime() : simTime();
        if (!tx_finish_timer->isScheduled()) {
            scheduleAt(nextTxFinishTime, tx_finish_timer);
        }
    }
}

void DstNode::handleMessage(cMessage *msg) {
    if (msg->isSelfMessage()) {
        if (msg == tx_finish_timer) {
            startTransmission();
            return;
        }
        // (no other self-messages expected)
        delete msg;
        return;
    }

    // Drop corrupted packets
    if (msg->isPacket() && static_cast<cPacket*>(msg)->hasBitError()) {
        delete msg;
        return;
    }

    SRMessage *incoming = check_and_cast<SRMessage *>(msg);

    if (incoming->getType() == DATA_FRAME) {
        int seq = incoming->getSeqNum();
        EV << "dst: received data frame " << seq << "\n";

        int seqSpaceSize = 2 * windowSize;

        if (seq >= frame_expected && seq < too_far) {
            int slot = seq % seqSpaceSize;

            if (!arrived[slot]) {
                arrived[slot] = true;
                in_buf[slot] = incoming->dup();
                EV << "dst: buffering frame " << seq << "\n";
            }

            // NAK guard: send NAK only once per missing block
            if (seq > frame_expected && no_nak) {
                EV << "dst: frame out of order. Sending NAK for " << frame_expected << "\n";
                sendFeedback(NAK_FRAME, frame_expected);
                no_nak = false;
            }

            // Deliver consecutive in-order frames
            while (arrived[frame_expected % seqSpaceSize]) {
                int currSlot = frame_expected % seqSpaceSize;
                EV << "dst: passing frame " << frame_expected << " to network layer\n";
                delete in_buf[currSlot];
                in_buf[currSlot] = nullptr;
                arrived[currSlot] = false;

                frame_expected++;
                too_far++;
                no_nak = true; // reset NAK guard
            }

            // Send cumulative ACK
            int cumulativeAck = frame_expected - 1;
            if (cumulativeAck >= 0) {
                EV << "dst: sent ACK with ackNum " << cumulativeAck << " at "
                   << std::fixed << std::setprecision(4) << simTime().dbl() << "\n";
                sendFeedback(ACK_FRAME, cumulativeAck);
            }

        } else if (seq < frame_expected) {
            // Duplicate old frame – resend last ACK
            int cumulativeAck = frame_expected - 1;
            if (cumulativeAck >= 0) {
                EV << "dst: sent ACK with ackNum " << cumulativeAck << " at "
                   << std::fixed << std::setprecision(4) << simTime().dbl() << "\n";
                sendFeedback(ACK_FRAME, cumulativeAck);
            }
        }
        // else seq >= too_far: ignore (outside window)
    }

    delete incoming;
}

void DstNode::finish() {
    // Delete transmission timer
    cancelAndDelete(tx_finish_timer);

    // Delete any pending messages in the queue
    while (!txQueue.empty()) {
        delete txQueue.front();
        txQueue.pop();
    }

    // Delete any buffered frames
    for (auto &buf : in_buf) {
        if (buf != nullptr) {
            delete buf;
            buf = nullptr;
        }
    }
    in_buf.clear();
}
