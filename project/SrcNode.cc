/*
 * SrcNode.cc
 *
 * Created on: Jun 22, 2026
 * Author: Notebook
 */

#include "SrcNode.h"
#include <iomanip>  // Required for explicit decimal logging layout control

Define_Module(SrcNode);

void SrcNode::initialize() {
    windowSize = par("windowSize");
    timeout = par("timeout");

    // Sequence space size = MaxSeq + 1 = 2 * windowSize
    maxSeqNum = (2 * windowSize) - 1;
    int seqSpaceSize = maxSeqNum + 1;

    tx_timers.resize(seqSpaceSize, nullptr);
    out_buf.resize(seqSpaceSize, nullptr);
    acked_frames.resize(seqSpaceSize, false);

    for (int i = 0; i < seqSpaceSize; i++) {
        tx_timers[i] = new cMessage("timeout");
        tx_timers[i]->setContextPointer((void*)(uintptr_t)i);
    }

    tx_finish_timer = new cMessage("tx_finish");

    // Fill the initial window with new frames
    while (S_upper < windowSize) {
        sendDataFrame(S_upper);
        S_upper++;
    }

    utilizationSignal = registerSignal("channelUtilization");
}

void SrcNode::sendDataFrame(int seqNum) {
    int slot = seqNum % (maxSeqNum + 1);

    if (out_buf[slot] != nullptr) {
        delete out_buf[slot];
    }

    SRMessage *msg = new SRMessage("DATA_FRAME");
    msg->setType(DATA_FRAME);
    msg->setSeqNum(seqNum);
    msg->setPayload("100-bit-payload-string");
    msg->setBitLength(100);

    out_buf[slot] = msg->dup();
    acked_frames[slot] = false;

    // Push into transmission queue (bool isNewFrame = true)
    mac_queue.push(std::make_pair(msg, true));
    startTransmission();
}

void SrcNode::startTransmission() {
    if (mac_queue.empty()) return;

    cGate *outGate = gate("out");
    cChannel *ch = outGate->getTransmissionChannel();
    simtime_t txFinishTime = ch ? ch->getTransmissionFinishTime() : simTime();

    // If the physical wire is busy, schedule a wake-up call
    if (txFinishTime > simTime()) {
        if (!tx_finish_timer->isScheduled()) {
            scheduleAt(txFinishTime, tx_finish_timer);
        }
        return;
    }

    // Channel is free – dequeue the front packet
    auto tx_pair = mac_queue.front();
    mac_queue.pop();

    SRMessage *msg = tx_pair.first;

    // Log every transmission (including retransmissions)
    EV << "src: sent data frame " << msg->getSeqNum() << " at "
       << std::fixed << std::setprecision(4) << simTime().dbl() << "\n";

    // Start the timeout timer
    int slot = msg->getSeqNum() % (maxSeqNum + 1);
    if (tx_timers[slot]->isScheduled()) {
        cancelEvent(tx_timers[slot]);
    }
    scheduleAt(simTime() + timeout, tx_timers[slot]);

    // Send the frame
    send(msg, "out");
    totalBitsTransmitted += 100;

    // Schedule wake-up if more packets are waiting
    simtime_t nextTxFinishTime = ch ? ch->getTransmissionFinishTime() : simTime();
    if (!mac_queue.empty() && !tx_finish_timer->isScheduled()) {
        scheduleAt(nextTxFinishTime, tx_finish_timer);
    }
}

void SrcNode::handleMessage(cMessage *msg) {
    if (msg->isSelfMessage()) {
        if (msg == tx_finish_timer) {
            startTransmission();
            return;
        }

        int slot = (int)(uintptr_t)msg->getContextPointer();

        // Timeout – retransmit only this frame
        if (out_buf[slot] != nullptr && !acked_frames[slot]) {
            int seqToResend = out_buf[slot]->getSeqNum();
            EV << "src: timeout on frame " << seqToResend << " - RETRANSMITTING ONLY THIS FRAME\n";
            mac_queue.push(std::make_pair(out_buf[slot]->dup(), false));
            startTransmission();
        }
    } else {
        // Drop corrupted feedback
        if (msg->isPacket() && static_cast<cPacket*>(msg)->hasBitError()) {
            delete msg;
            return;
        }
        SRMessage *incoming = check_and_cast<SRMessage *>(msg);

        if (incoming->getType() == ACK_FRAME) {
            int ackNum = incoming->getAckNum();
            EV << "src: received ACK. Frame " << ackNum << " confirmed.\n";

            if (ackNum >= S_lower && ackNum < S_upper) {
                while (S_lower <= ackNum) {
                    int slot = S_lower % (maxSeqNum + 1);

                    totalGoodputBits += 100;

                    if (out_buf[slot] != nullptr) {
                        delete out_buf[slot];
                        out_buf[slot] = nullptr;
                    }
                    acked_frames[slot] = false;
                    if (tx_timers[slot]->isScheduled()) {
                        cancelEvent(tx_timers[slot]);
                    }
                    S_lower++;
                }
                while (S_upper < S_lower + windowSize) {
                    sendDataFrame(S_upper);
                    S_upper++;
                }
            }
        }
        else if (incoming->getType() == NAK_FRAME) {
            int nakNum = incoming->getAckNum();
            EV << "src: received NAK for frame " << nakNum << ". Retransmitting.\n";

            if (nakNum >= S_lower && nakNum < S_upper) {
                int slot = nakNum % (maxSeqNum + 1);
                if (out_buf[slot] != nullptr) {
                    mac_queue.push(std::make_pair(out_buf[slot]->dup(), false));
                    startTransmission();
                }
            }
        }
        delete incoming;
    }
}

void SrcNode::finish() {
    cancelAndDelete(tx_finish_timer);

    for (auto timer : tx_timers) {
        cancelAndDelete(timer);
    }
    tx_timers.clear();

    for (auto &buf : out_buf) {
        if (buf != nullptr) {
            delete buf;
            buf = nullptr;
        }
    }
    out_buf.clear();

    // Emit utilization statistic
    double simDuration = simTime().dbl();
    if (simDuration > 0) {
        double goodput = (double)totalGoodputBits / (100000.0 * simDuration);
        emit(utilizationSignal, goodput);
    }
}
