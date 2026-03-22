# Discussion: Existing Products vs Custom Path for 100G/200G

## Goal
Evaluate whether existing products can satisfy FPGA-to-Windows receive requirements when scaling beyond current 10GbE Phase 1 into 100G and 200G.

## Current Baseline
- Current implementation target: Windows C++ UDP receiver on 10GbE.
- Phase 1 acceptance target: sustained goodput >= 8 Gbps.
- FPGA sender is already implemented and treated as fixed.

## Product Categories to Evaluate

### 1. High-speed standard NIC + tuned application
- Example vendors: NVIDIA (ConnectX), Intel, Broadcom.
- Best fit: move from 10GbE to 25/40/100GbE with minimum architecture change.
- Limitation: standard Windows socket path can become bottleneck at very high packet rates.

### 2. SmartNIC / capture acceleration cards
- Example vendors: Napatech, Endace class solutions.
- Best fit: high sustained ingest with strong observability and timestamp support.
- Limitation: higher hardware and SDK integration cost.

### 3. Vendor SDK or kernel-bypass receive stack
- Best fit: 100G+ goals where normal UDP stack is insufficient.
- Limitation: stronger vendor lock-in and higher integration complexity.

### 4. Full custom driver and DMA-centric design
- Best fit: 200G class throughput with deep hardware/software co-design.
- Limitation: highest development and validation effort.

## CT Raw Data Scene: Dedicated Acquisition Card vs Ethernet

### Style A: Executive Comparison

#### Context
- Scene: CT gantry raw data transmission to recon PC receiver.
- Current baseline remains Phase 1 Windows UDP on 10GbE.
- Near-term decision target is 100G scaling, with 200G as follow-on planning.

#### Options in Scope
1. Dedicated SmartNIC/capture card (vendor SDK driven).
2. Dedicated FPGA/custom acquisition card installed in recon PC.
3. Standard high-speed Ethernet NIC + tuned UDP software path.

#### Head-to-Head (Decision Maker View)

| Option | 100G Throughput Potential | Windows Integration Risk | Time-to-Value | Cost/Lock-in | Executive Fit |
|---|---|---|---|---|---|
| Dedicated SmartNIC/capture card | High | Mid-High | Mid | High | Strong when UDP path misses target and vendor SDK is mature on Windows |
| Dedicated FPGA/custom acquisition card | Very High | High | Low-Mid | Very High | Best absolute headroom, but only if long integration cycle is acceptable |
| Standard high-speed Ethernet NIC | Mid-High to High | Low-Mid | High | Low-Mid | Preferred first move for 100G because delivery risk is lowest |

#### Executive Recommendation (100G First)
1. Keep Ethernet NIC path as primary route for near-term 100G proof because it gives fastest delivery with acceptable risk.
2. Promote SmartNIC/capture card to Plan-B when measured socket-path throughput, loss, or CPU cannot meet acceptance gate.
3. Reserve dedicated FPGA/custom acquisition card for roadmap stage where 200G-class sustained ingest is mandatory and lock-in is acceptable.

### Style B: Engineering Comparison

#### Technical Axes
1. Sustained goodput under real payload mix.
2. Packet-rate tolerance (small packet pressure vs jumbo-enabled path).
3. Receiver CPU cost, core pinning, and NUMA locality sensitivity.
4. Driver/SDK dependency and Windows support maturity.
5. Observability depth (timestamps, drops by queue, reorder visibility).

#### Engineering Head-to-Head

| Axis | Dedicated SmartNIC/capture card | Dedicated FPGA/custom acquisition card | Standard high-speed Ethernet NIC |
|---|---|---|---|
| Sustained ingest ceiling | High at 100G; often stable with vendor pipeline | Very high, potentially best for deterministic ingest | High if NIC queueing + app threading are tuned correctly |
| Small-packet PPS pressure | Better than generic socket path due to optimized ingest stack | Best potential when DMA/driver are co-designed for workload | Most sensitive to Windows UDP kernel path overhead |
| Host CPU overhead | Medium; may reduce host parsing burden via SDK features | Low-Medium possible, but depends on custom software quality | Medium-High at high PPS; can improve with jumbo + multi-queue |
| Windows dependency risk | Vendor-specific SDK quality varies; evaluate early | Highest risk: custom driver lifecycle and signing/maintenance burden | Lowest risk; mainstream driver ecosystem |
| Timestamp/observability | Strong hardware timestamp and capture diagnostics | Can be excellent if built-in, but requires custom implementation | Adequate for baseline; advanced telemetry may need extra tooling |
| Integration complexity | Medium-High | Very High | Low-Medium |
| Scalability toward 200G | Good with right card family or multi-link | Very good when architecture is dedicated end-to-end | Possible via dual 100G or specialized NIC, but software path becomes critical |

#### Engineering Escalation Triggers
Move from standard Ethernet NIC path to dedicated-card path if one or more conditions persist after tuning:
1. Sustained goodput cannot hold target for full acceptance duration.
2. Estimated packet loss or reorder remains above threshold under target load.
3. Host CPU utilization stays too high to leave operational margin.
4. Packet-size profile is small-message dominant and PPS bottleneck cannot be relieved by jumbo/multi-queue tuning.

#### Platform Implication
- Windows is preferred for present integration path.
- Linux remains valid escalation path when 100G/200G headroom requires kernel-bypass ecosystem maturity beyond practical Windows limits.

#### Consolidated Recommendation
1. Execute 100G proof on standard high-speed Ethernet NIC first, with strict measurable acceptance gates.
2. Keep SmartNIC/capture card as first escalation option if gates fail.
3. Consider dedicated FPGA/custom acquisition card only when long-term throughput determinism justifies high implementation and maintenance cost.

## Practical Feasibility
- 10GbE: feasible with current architecture.
- 100G: feasible with high-end NIC and optimized software path.
- 200G: feasible but likely requires either dual 100G aggregation or specialized 200G platform plus multi-queue parallel receive architecture.

## Bottlenecks to Watch
1. Windows kernel UDP path overhead at high packet rates.
2. NUMA and PCIe topology mismatch between NIC and CPU cores.
3. Memory bandwidth pressure and cache locality in parser/statistics path.
4. Packet-rate limits with small payload sizes.

## NIC Feature Background and Solution Impact

| Feature | Why It Matters | Impact on Our Solution | Priority |
|---|---|---|---|
| DMA | Moves packet data from NIC to host memory with minimal CPU copy overhead | Critical for sustained throughput and CPU headroom in 10G-to-100G scaling | High |
| RSS/multi-queue | Spreads RX processing across multiple queues and CPU cores | Key to avoid single-core bottleneck and support parallel receiver threads | High |
| Interrupt moderation | Batches interrupts to reduce context-switch overhead at high PPS | Improves CPU efficiency; must balance throughput and latency | Medium |
| RX/TX ring buffer | Defines burst absorption depth before packet drops happen | Directly affects drop behavior during transient load spikes | High |
| Checksum offload | Offloads checksum calculation/verification to NIC hardware | Reduces per-packet CPU cost in UDP receive path | Medium |
| RSC/LRO | Coalesces packets to reduce per-packet processing pressure | Useful when packet-rate pressure dominates over pure bandwidth | Medium |
| Jumbo frame support | Reduces packet rate for the same goodput | Strong lever for 100G/200G when end-to-end path supports jumbo MTU | High |
| Hardware timestamp | Provides precise packet timing at NIC level | Improves loss/jitter/order diagnosis and root-cause localization | Medium |
| SR-IOV | Enables virtualized direct NIC access to VMs | Low relevance for current bare-metal baseline; useful if virtualization is introduced later | Low |

## NIC Vendor Product Indicative Price Ranges (USD)

Notes:
- Ranges below are planning-level indicative ranges (USD), not procurement quotes.
- Values can vary by port count, optics bundle, support contract, and regional channel pricing.
- Scope of this table is restricted to 100G-class cards only.
- This revision prefers listings marked as new and excludes explicit REFURBISHED/USED/LIKE NEW/Open Box entries where visible.
- Accessory-only entries (for example bracket-only) and non-100G SKUs are excluded from range calculation.
- China-brand additions are included as separate rows: e-commerce-visible compatible cards and domestic enterprise-channel cards.
- Add an explicit TP-LINK row as a representative well-known China brand in enterprise networking channels.
- Price range is now expressed as qualitative bands (Low/Mid/High/Very High) for later manual numeric adjustment.

| Vendor / Product Family | Product Class | Typical Speed Fit | Indicative Price Band | Cost Position |
|---|---|---|---|---|
| Intel Ethernet (E810-CQDA1/CQDA2 class) | Standard Ethernet NIC | 100G | Low | Low-Mid |
| Broadcom Ethernet (100G adapter class) | Standard Ethernet NIC | 100G | Low | Low-Mid |
| NVIDIA/Mellanox ConnectX-6 Dx family | Standard Ethernet NIC / performance NIC | 100G | Mid | Mid |
| NVIDIA/Mellanox BlueField family | SmartNIC/DPU | 100G | High | Mid-High |
| TP-LINK enterprise networking products (100G card/channel ecosystem, where available) | Enterprise Ethernet adapter/channel product | 100G | Mid | Low-Mid |
| China domestic compatible 100G NIC brands (for example Vogzone/C&N/Euqvos/HUNTION/JVFYI) | Standard Ethernet NIC / compatible NIC | 100G | Low | Low-Mid |
| China enterprise-channel 100G NIC brands (for example Inspur/H3C ecosystem cards) | Enterprise Ethernet NIC (channel-oriented) | 100G | Mid | Mid |
| Napatech capture adapter family | SmartNIC/capture acceleration card | 100G | High | High |
| Endace DAG platform family | Capture/recording acceleration platform | 100G | Very High | High-Very High |
| Dedicated FPGA/custom acquisition card | Custom dedicated card | 100G | Very High | Very High |

Key guidance:
1. Prioritize DMA, RSS/multi-queue, and ring-buffer sizing before considering architectural escalation.
2. Tune interrupt moderation and RSC/LRO only with measured latency/CPU trade-off data.
3. Validate jumbo and hardware timestamp features end-to-end across the current direct-connect path (FPGA, optics/cable path, NIC, driver, OS) before using them as acceptance assumptions; include switch validation only if a switched topology is introduced later.

## Jumbo Frame Position
- Jumbo Frames are not mandatory for Phase 1 bring-up on 10GbE.
- Jumbo Frames are strongly recommended for future 100G and 200G targets because they reduce packets-per-second pressure on FPGA logic, NIC queues, and Windows receiver threads.
- Use standard MTU first if it simplifies integration, then benchmark jumbo-enabled runs as a controlled optimization step.
- Jumbo Frames only help when the entire current path supports them: FPGA MAC/IP, optics/cable path, NIC, and Windows NIC configuration; add switch compatibility only if switch insertion becomes part of the topology later.
- If actual workload is dominated by small messages, jumbo support may have limited benefit and multi-queue design becomes more important.

## RDMA Position
- RDMA is not required for Phase 1 or initial 10GbE bring-up.
- RDMA is optional for 100G and becomes a stronger candidate for 200G when host CPU overhead, packet-rate pressure, or copy overhead becomes the main bottleneck.
- Stay on UDP if custom packet framing is acceptable and measured performance meets the target with manageable loss and CPU usage.
- Consider RDMA when zero-copy transfer semantics, lower host overhead, or memory-to-memory data movement become more important than implementation simplicity.
- On Windows, RDMA adoption should be treated as a platform decision because hardware support, drivers, and vendor dependencies become more restrictive.

## Simple OS Decision Table

| Target Path | Recommended OS | Reason |
|---|---|---|
| UDP receive path | Windows Pro | Lowest integration friction for current Phase 1 WinSock baseline. |
| RDMA receive path | Windows Server | Better alignment with enterprise RDMA deployment patterns and driver/support matrices. |
| Maximum 100G/200G headroom | Linux | Strongest ecosystem for high-end NIC tuning, kernel-bypass stacks, and ultra-high-throughput optimization. |

## Recommended Strategy
1. Keep Phase 1 focused on robust 10GbE validation and metric accuracy.
2. Run a structured product evaluation for 100G options before major refactor.
3. Decide early whether 200G means single-link 200G or multi-link aggregation.
4. Only move to driver-level, kernel-bypass, or RDMA architecture if measured bottlenecks prove socket path is insufficient.

## Decision Matrix Template

| Option | Expected Throughput | Windows Support Risk | Integration Effort | Cost Level | Recommendation |
|---|---:|---|---|---|---|
| Standard NIC + WinSock tuning | Mid-High (10G to practical 100G with tuning) | Low-Mid | Low-Mid | Low-Mid | Primary path for 100G proof |
| SmartNIC/capture card + vendor SDK | High (100G stable ingest) | Mid-High | Mid-High | High | First escalation if Ethernet socket path misses gate |
| Dedicated FPGA/custom acquisition card | Very High (100G+/200G class with co-design) | High | Very High | Very High | Reserve for deterministic high-end roadmap |
| Kernel-bypass stack | High (100G+) | Mid-High | High | Mid-High | Evaluate if phase-2 target is missed |
| Custom driver + DMA path | Very High (200G class) | High | Very High | High | Reserve for later phase |

## 100G Weighted Scoring Version
- Score scale: 1-5 (higher is better).
- Weighted total (0-100) = sum(score x weight).
- Normalized total (0-5) = weighted total / 20.

| Criteria | Weight | Ethernet | SmartNIC/capture | Dedicated FPGA/custom |
|---|---:|---:|---:|---:|
| Ease of Integration | 15 | 5 | 3 | 2 |
| Windows Maturity & Support | 20 | 5 | 3 | 2 |
| Time-to-Value | 15 | 5 | 3 | 1 |
| Cost & Vendor Lock-in | 15 | 5 | 3 | 2 |
| 100G Throughput Potential | 20 | 4 | 5 | 5 |
| CPU Efficiency | 10 | 3 | 5 | 5 |
| Observability Depth | 5 | 4 | 4 | 2 |
| Total | 100 | 92.00 / 4.60 | 73.00 / 3.65 | 55.00 / 2.75 |

Interpretation: For near-term 100G delivery, Ethernet-first remains the best overall choice because it maximizes integration speed and Windows maturity with acceptable performance headroom. Escalate to SmartNIC/capture only when repeated gate failures remain after tuning (throughput, loss/reorder, CPU margin).

## Key Decisions Needed Next
1. Is Windows mandatory for 100G/200G production deployment?
2. Is vendor lock-in acceptable if it shortens time to 100G?
3. Is 200G required on one link, or acceptable as 2x100G aggregate?
4. What packet size profile dominates real workload (small packets vs large payload)?
5. Is zero-copy or direct memory placement important enough to justify RDMA complexity?

## Immediate Action Items
1. Build a vendor short-list and request Windows SDK capability details.
2. Define a 100G proof target and acceptance thresholds, including sustained duration and CPU ceiling.
3. Add receiver-side strict acceptance controls (loss/order thresholds) for fair product comparisons.
4. Add packet-size observability so jumbo-vs-standard MTU can be measured instead of guessed.
5. Run controlled A/B benchmarking across three lanes: standard Ethernet NIC, SmartNIC/capture card, and dedicated FPGA/custom acquisition card.
6. Record UDP baseline CPU usage before considering RDMA or dedicated-card migration.
7. Build go/no-go rule: stay on Ethernet if target is met; escalate to dedicated-card evaluation only when repeated gate failure is observed.

## 10GbE Data Loss Discussion and Localization Method

Current topology note:
- The current validation scene is direct connect only: FPGA to recon PC NIC without a switch in the active path.
- Any switch-related checks are future-extension items, not part of the present baseline acceptance path.

### Current Status
- No confirmed 10GbE data-loss incident is recorded yet.
- At this stage, 10GbE loss is treated as a risk and visibility gap that must be validated with end-to-end counters.

### Fault-Domain Segmentation
1. FPGA sender (TX generation and sequence continuity).
2. Direct link path (optic/cable path, CRC, physical stability, MTU consistency).
3. NIC and driver path (RX queue depth, drops, ring overflow).
4. UDP socket and receiver application (buffer overflow, recv rate, sequence gap).

### Localization Checklist

| Stage | Counter or Signal | If mismatch, likely location and action |
|---|---|---|
| Sender TX | FPGA sent packet count and sequence continuity | If sender count/sequence is unstable, investigate FPGA TX logic and pacing first. |
| Direct Link | CRC/error indications and link stability on both ends | If link errors increase, inspect cable, optic, NIC port state, and MTU alignment on the direct-connect path. |
| NIC RX | NIC RX packets vs RX drops and queue overflow counters | If RX drops rise while link is clean, tune driver/ring/interrupt settings and verify firmware. |
| UDP/Socket | UDP recv error/drop counters and socket buffer usage | If NIC is clean but UDP drops rise, increase socket buffers and reduce per-packet processing latency. |
| App Sequence | Application sequence gap/reorder counters | If UDP layer looks clean but app shows gaps, profile app pipeline and queue handoff bottlenecks. |

### Immediate Next Actions
1. Add synchronized 1-second snapshots for FPGA TX, direct-link status, NIC RX, UDP socket, and app sequence counters.
2. Run staged load tests (2G -> 4G -> 6G -> 8G) and identify the first stage where counter divergence appears.
3. Execute A/B isolation runs on the current direct-connect topology (baseline driver vs tuned driver, baseline NIC settings vs tuned NIC settings) to lock fault domain.
4. Define an escalation rule: once a stage exceeds loss threshold for consecutive windows, assign owner and run targeted remediation before retest.

### Windows Counter Collection Command Checklist

| Layer | Command (PowerShell) | Key Metrics | Interpretation |
|---|---|---|---|
| NIC link and driver | `Get-NetAdapterStatistics -Name "<NIC_NAME>"` | ReceivedBytes, ReceivedUnicastPackets, ReceivedDiscardedPackets, ReceivedPacketErrors | If discards/errors increase while sender is stable, suspect NIC or link path. |
| NIC advanced settings | `Get-NetAdapterAdvancedProperty -Name "<NIC_NAME>"` | RSS, interrupt moderation, receive buffers, jumbo settings | Mismatch from expected tuning indicates driver config drift. |
| UDP stack | `netstat -s -p udp` | Datagrams Received, Receive Errors, No Ports | Rising receive errors indicates socket or kernel-path pressure. |
| IP stack drops | `netstat -s -p ip` | Received Header Errors, Received Address Errors, Datagrams Discarded | Increases suggest pre-socket drop in IP processing path. |
| Per-second baseline | `Get-Counter '\Network Interface(*)\Packets Received Discarded','\Network Interface(*)\Packets Received Errors','\Processor(_Total)\% Processor Time' -SampleInterval 1 -MaxSamples 60` | Discard/error trend and CPU trend over 60 seconds | First metric that diverges marks likely loss domain and escalation owner. |

Recommended run sequence:
1. Capture all command outputs once before load (baseline snapshot).
2. Capture per-second counters during a 60-second run at current target load.
3. Capture a final post-run snapshot and compare delta between sender count, NIC RX, UDP received, and app sequence counters.
4. If mismatch first appears at NIC metrics, tune NIC/driver first; if mismatch first appears at UDP/IP counters, tune socket/thread pipeline first.

## One-Page Round-Based Statistical Test Report (Ready to Use)

Use this template for one reliability test round under CT model assumptions.

### A. Campaign Header

| Field | Value |
|---|---|
| Report ID | |
| Test Round ID (build tag) | |
| Test Horizon | One day / One week / Deep round |
| Date Range | |
| DUT Version (receiver/app) | |
| FPGA Sender Version | |
| NIC Model / Driver / Firmware | |
| OS / Build | |
| Topology | Direct connect (FPGA -> recon PC NIC) |
| Traffic Profile | Large / Mixed / Small-PPS stress |
| Operator / Reviewer | |

### B. Fixed Statistical Model and Gates

| Item | Definition |
|---|---|
| Window definition | 1 scan-equivalent burst = 10 seconds |
| Horizon windows | One day = 300, One week = 2100, Deep round = 10000 |
| Statistical gate model | Band-based pass/fail by success-rate threshold |
| Practical gate formula | Allowed failed windows = floor((1 - threshold) * windows) |
| Optional strict gate | 0 failed windows per round |
| Throughput gate | Sustained goodput >= 8.0 Gbps in assessed windows |
| CPU gate | Average CPU usage (Processor(_Total) % Processor Time) <= 30% |

| Band | Success Rate Threshold | Allowed Failed Windows | Minimum Passed Windows | Typical Usage |
|---|---:|---:|---:|---|
| L1 | >= 90.00% | <= 1000 | >= 9000 | Frequent smoke trending |
| L2 | >= 95.00% | <= 500 | >= 9500 | Frequent stable tracking |
| L3 | >= 99.00% | <= 100 | >= 9900 | Frequent acceptance baseline |
| L4 | >= 99.99% | <= 1 | >= 9999 | Deep reliability validation |

| Horizon | Windows | 90.00% | 95.00% | 99.00% | 99.99% |
|---|---:|---:|---:|---:|---:|
| One day | 300 | <= 30 | <= 15 | <= 3 | 0 |
| One week | 2100 | <= 210 | <= 105 | <= 21 | 0 |
| Deep round | 10000 | <= 1000 | <= 500 | <= 100 | <= 1 |

### C. Measured Result Summary

| Metric | Measured | Gate | Pass/Fail |
|---|---:|---:|---|
| Total windows | | depends on selected horizon | |
| Passed windows | | from selected horizon and threshold | |
| Failed windows | | from selected horizon and threshold | |
| Zero-loss success rate (%) | | from selected band threshold | |
| Avg goodput (Gbps) | | >= 8.0 | |
| Avg CPU usage (%) | | <= 30 | |
| Crash/Hang events | | 0 | |
| Invalid packet total | | 0 (or approved threshold) | |
| Out-of-order total | | project threshold | |

### D. Failure Breakdown (fill if any)

| Window ID | Time | Domain (Sender/Link/NIC/UDP/App) | Symptom | Immediate Action | Retest Result |
|---|---|---|---|---|---|
| | | | | | |
| | | | | | |
| | | | | | |

### E. Evidence Checklist

| Evidence Item | Collected (Y/N) | Artifact Path |
|---|---|---|
| Receiver final summaries (all windows) | | |
| Sender TX counters / sequence logs | | |
| NIC statistics snapshots | | |
| UDP/IP stack counters | | |
| 1-second CPU/discard/error counters | | |
| Tuning config snapshot (RSS/ring/interrupt/jumbo) | | |

### F. Final Decision

| Decision Item | Value |
|---|---|
| Round result | PASS / FAIL |
| Method type | Frequent / Deep |
| Band achieved | L1 / L2 / L3 / L4 |
| Gate violated (if FAIL) | Statistical / Throughput / CPU / Stability |
| Owner | |
| Corrective action ID | |
| Re-test due date | |
| Reviewer sign-off | |

## Primary Later-Stage Method: Per-Scan Success-Rate Confidence

For early Phase 0.5, keep the lightweight bring-up method.
For later Phase 0.5 and 100G qualification, use per-scan success probability as the primary statistical gate.
Because 10G Phase 0.5 is transitional for technical verification, later Phase 0.5 uses a weaker confidence bucket than 100G.

### Target Definition
- Phase 0.5 later-stage target: p0 = 99.50% = 0.995.
- 100G qualification target: p0 = 99.99% = 0.9999.
- One window equals one scan-equivalent burst (10 seconds).
- Success means no packet loss, no invalid packet, no crash/hang, and throughput/CPU gates are met.

### Zero-Failure Confidence Method (Governance Baseline)
Assume n windows are tested and observed failed windows k = 0.

One-sided confidence claim for per-scan success rate:

$$
P(p \ge p_0) \text{ at confidence } C
$$

Required sample size for zero-failure evidence:

$$
n \ge \frac{\ln(1-C)}{\ln(p_0)}
$$

For p0 = 0.995 (Phase 0.5 later-stage):

| Confidence C | Required windows n (k=0) |
|---:|---:|
| 90% | 460 |
| 95% | 598 |
| 99% | 919 |

For p0 = 0.9999 (100G qualification):

| Confidence C | Required windows n (k=0) |
|---:|---:|
| 90% | 23025 |
| 95% | 29956 |
| 99% | 46050 |

Default policy for this project:
- Phase 0.5 later-stage validation gate: p0 = 99.50%, 95% confidence with n >= 598 and k = 0.
- 100G qualification gate: 95% confidence with n >= 29956 and k = 0.
- Optional stronger release evidence: 99% confidence with n >= 46050 and k = 0.

Practical rounding policy:
- Use the next rounded-up operational bucket (for example 25000, 30000, 50000 windows).

### How To Use With Existing Monitoring Windows
1. Keep one-day, one-week, and 10000-window campaigns as supplemental engineering monitoring and trend dashboards.
2. Use per-scan confidence evidence as the primary qualification decision gate for later Phase 0.5 and 100G.
3. If zero-failure evidence cannot be reached in target confidence bucket, block qualification and trigger root-cause follow-up.

### Reporting Add-on Fields (Optional)
Add these fields to campaign summary when using this method:
- Target per-scan success rate p0 (Phase 0.5 later default 99.50%; 100G default 99.99%).
- Confidence level C (90%/95%/99%).
- Tested windows n.
- Observed failed windows k.
- Whether k = 0 criterion is met.
- Whether n meets required bucket for selected C.