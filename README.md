# Selective Repeat ARQ Simulation

**Data Networks – Dr. Pakravan**  
---

## 📖 Overview

This repository contains the implementation of **Selective Repeat ARQ (Tanenbaum's Protocol 6)** using the **OMNeT++** simulation framework. The simulation models a high-latency, high-bandwidth network resembling a satellite or transcontinental link.

---

## 🚀 Key Features

- Selective Repeat Protocol with individual frame timers (`setContextPointer()`)
- Unified message structure (DATA, ACK, NAK)
- Out-of-order buffering with gap tracking
- NAK guard to prevent NAK storms
- Sequence number space \( M = 2W \)
- Goodput measurement (only successfully delivered frames)



---

## 🏗️ Network Architecture

```
+---------+         +----------+
|         |         |          |
| SrcNode |-------->| DstNode  |
|         |<--------|          |
+---------+         +----------+

Forward: 100 kbps, 50ms, PER = 0.0/0.2/0.3
Reverse: 100 kbps, 50ms, PER = 0.0
```

---

## 🧪 Simulation Configurations

| Configuration | Window Size | Forward PER | Reverse PER |
|---------------|-------------|-------------|-------------|
| `ErrorFree_W4` | 4 | 0.0 | 0.0 |
| `ErrorFree_W20` | 20 | 0.0 | 0.0 |
| `ErrorFree_W120` | 120 | 0.0 | 0.0 |
| `Lossy_W4_PER20` | 4 | 0.2 | 0.0 |
| `Lossy_W20_PER20` | 20 | 0.2 | 0.0 |
| `Lossy_W120_PER20` | 120 | 0.2 | 0.0 |
| `Lossy_W4_PER30` | 4 | 0.3 | 0.0 |
| `Lossy_W20_PER30` | 20 | 0.3 | 0.0 |
| `Lossy_W120_PER30` | 120 | 0.3 | 0.0 |

---

## 📊 Results Summary

### Error-Free Channel (PER = 0.0)

| Window Size | Theoretical | Measured | Discrepancy |
|-------------|-------------|----------|-------------|
| W = 4 | 3.96% | 3.94% | 0.4% |
| W = 20 | 19.80% | 19.72% | 0.4% |
| W = 120 | 100% | 99.89% | 0.1% |

### Lossy Channel (PER = 0.3)

| Window Size | Theoretical | Measured | Discrepancy |
|-------------|-------------|----------|-------------|
| W = 4 | 2.77% | 2.04% | 26.3% |
| W = 20 | 13.86% | 6.84% | 50.7% |
| W = 120 | 70.0% | 26.43% | 62.2% |

---

## 📝 Sample Logs

```
# Successful Frame Delivery
src: sent data frame 0 at 0.0000
dst: received data frame 0
dst: buffering frame 0
dst: passing frame 0 to network layer
dst: sent ACK with ackNum 0 at 0.0510
src: received ACK. Frame 0 confirmed.

# Out-of-Order Buffering and NAK
dst: received data frame 16
dst: buffering frame 16
dst: frame out of order. Sending NAK for 15
src: received NAK for frame 15. Retransmitting.

# Timeout Handling
src: timeout on frame 16 - RETRANSMITTING ONLY THIS FRAME
```

---

## 🧠 Implementation Highlights

```cpp
// Individual Frame Timers
int seqSpaceSize = maxSeqNum + 1;
tx_timers.resize(seqSpaceSize, nullptr);
for (int i = 0; i < seqSpaceSize; i++) {
    tx_timers[i] = new cMessage("timeout");
    tx_timers[i]->setContextPointer((void*)(uintptr_t)i);
}

// Receiver Buffer Arrays
std::vector<bool> arrived;
std::vector<SRMessage*> in_buf;

// NAK Guard
if (seq > frame_expected && no_nak) {
    sendFeedback(NAK_FRAME, frame_expected);
    no_nak = false;
}

// Sequence Number Space
maxSeqNum = (2 * windowSize) - 1;
int seqSpaceSize = maxSeqNum + 1;
```

---

## 🚀 How to Run

1. Clone the repository:
```bash
git clone https://github.com/yourusername/Selective-Repeat-ARQ-OMNeTpp.git
```

2. Import into OMNeT++ IDE (File → Import → Existing Projects)

3. Build the project (Right-click → Build Project)

4. Select a configuration and Run

---

## 🔍 Key Observations

- Error-free scenarios match theory within 0.4%
- Discrepancy grows with window size in lossy scenarios
- Pipeline stalling causes effective RTT doubling
- Large windows amplify the impact of losses

---

## 🛠️ Technologies

- OMNeT++ 6.0
- C++ 11
- LaTeX

---

## 📚 References

1. Tanenbaum, A. S., & Wetherall, D. J. (2011). *Computer Networks* (5th ed.). Pearson.
2. OMNeT++ User Manual. (2022). https://omnetpp.org/documentation/

---

## 👩‍🎓 Author

**Amitis Mirabedini**  
