# Prompt For PPT Generation (EN)

Use the project materials below to create a professional technical-decision PPT in English.

## Source Basis
- plan/plan_en.md
- requirement/discussion.md
- requirement/requirement_phase1.md

## Presentation Goal
Create a decision-support presentation for internal review on CT raw-data transmission from gantry to recon PC, covering:
- current 10GbE Phase 0.5 goal,
- 100G near-term strategy,
- dedicated acquisition card vs Ethernet comparison,
- key technical bottlenecks and NIC feature impact,
- phased execution and go/no-go gates.

## Audience
- technical managers,
- system architects,
- FPGA/software engineers,
- procurement or platform decision stakeholders.

## Output Requirements
- Language: English.
- Tone: professional, concise, decision-oriented.
- Length: exactly 15 slides.
- Output format: slide-by-slide content.
- For each slide, provide:
  - slide title,
  - 3 to 6 bullets,
  - optional speaker note,
  - suggested visual (table, flow, comparison, timeline, checklist, heatmap, etc.).

## Content Requirements
The PPT must follow this exact 15-slide blueprint:
1. Title and executive framing (what decision is being made and why now).
2. Project objective, scope, and current direct-connect topology assumption.
3. Project technical route: Ethernet-first.
4. Dedicated acquisition card vs Ethernet: multi-perspective comparison.
5. Fixed 100G comparison table (use exact provided values).
6. Fixed 100G weighted scoring matrix (use exact provided values).
7. Fixed NIC key features table and tuning priorities.
8. Current 10GbE Phase 0.5 target and lifecycle-oriented acceptance baseline.
9. 10GbE data-loss current status and fault-domain segmentation.
10. Fixed data-loss localization checklist and immediate actions.
11. Fixed Windows counter collection table and evidence workflow.
12. Vendor landscape and platform view (Windows and Linux), using the fixed numeric vendor price-range table (USD).
13. Detailed test-method design.
14. Phased roadmap (Phase 0.5, Phase 1, Phase 2, Phase 3) with acceptance gates and escalation triggers.
15. Recommended strategy, next actions, and final management decision points.

Do not merge, remove, or reorder these 15 slides.

## Key Messages To Preserve
- No confirmed 10GbE data-loss incident yet; current status is risk/visibility gap.
- The current scene uses direct connect only between FPGA and recon PC NIC; switch topology is not part of the present baseline.
- Ethernet-first remains the preferred near-term 100G path.
- SmartNIC/capture card is the first escalation path if repeated gates fail after tuning.
- Dedicated FPGA/custom card is reserved for stronger deterministic throughput needs.
- Windows is preferred for present path, Linux remains acceptable as escalation platform.
- 10G baseline must produce trusted evidence before 100G procurement and benchmarking.

## Important Data To Highlight
- Phase 1 baseline: sustained goodput >= 8 Gbps.
- CPU gate: average CPU usage <= 30% during assessed acceptance windows.
- 10G acceptance uses two layers: bring-up gate (>= 60 seconds and 3 consecutive pass runs) and lifecycle reliability gate aligned to CT operation assumptions (10-year service life, about 300 patients/day, about 10-second raw-data burst per scan).
- Per-scan confidence acceptance is the primary later-stage method:
  - Phase 0.5 later-stage (validation-oriented, weaker than 100G): p0 = 99.50% at 95% confidence, requiring >= 598 zero-failure windows.
  - 100G qualification (stricter): p0 = 99.99% at 95% confidence, requiring >= 29956 zero-failure windows.
  - Optional stronger release evidence: 99% confidence with >= 46050 zero-failure windows.
  Supplemental day/week/deep windows can be shown as trend-monitoring views, but they do not replace confidence-based qualification.
- 100G weighted scoring:
  - Ethernet: 92.00 / 4.60
  - SmartNIC/capture: 73.00 / 3.65
  - Dedicated FPGA/custom: 55.00 / 2.75

## Final Test Criteria And Method (Must Be Explicitly Included)
- The PPT must explicitly present the one-sided confidence, zero-failure sample-size formula for per-scan qualification:
  - n >= ln(1 - C) / ln(p0), where n is required zero-failure windows, C is confidence, and p0 is target per-scan success probability.
- Final qualification criteria to present as fixed gates:
  - Phase 0.5 later-stage gate: p0 = 99.50%, C = 95%, n >= 598, with k = 0 failures.
  - 100G qualification gate: p0 = 99.99%, C = 95%, n >= 29956, with k = 0 failures.
  - Optional stronger 100G release evidence: C = 99%, n >= 46050, with k = 0 failures.
- Final execution method to present as fixed workflow:
  - Step 1: Run bring-up gate first (>= 60 seconds and 3 consecutive pass runs).
  - Step 2: Enter per-scan logging mode and count windows at scan granularity.
  - Step 3: Apply confidence gate with zero-failure rule (k = 0) against required n.
  - Step 4: Enforce CPU gate in parallel (average CPU <= 30% in assessed windows).
  - Step 5: If any failure appears before n is reached, classify as "not qualified", trigger fault-domain localization, tuning, and full re-run.
  - Step 6: Use day/week/deep windows only for trend monitoring, not as replacement for qualification.

## Fixed 100G Comparison Table To Use

| Option | Expected Throughput (100G target) | Packet-Rate Tolerance | CPU Overhead Risk | Windows Integration Risk | Implementation Effort | Cost Level | Recommendation |
|---|---|---|---|---|---|---|---|
| Standard high-speed Ethernet NIC + tuned UDP | Mid-High to High | Medium (small-packet PPS sensitive) | Medium-High | Low-Mid | Low-Mid | Low-Mid | Primary path for near-term 100G proof |
| SmartNIC / capture acceleration card | High | High | Medium | Mid-High (SDK dependent) | Mid-High | High | First escalation if Ethernet lane repeatedly fails |
| Dedicated FPGA/custom acquisition card | Very High | Very High | Low-Medium (if co-designed well) | High | Very High | Very High | Use when deterministic throughput is mandatory |

## Fixed 100G Weighted Scoring Matrix To Use

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

Use these exact values in the presentation and do not re-score or reinterpret the numeric matrix.

## Fixed NIC Key Features Table To Use

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

Use this exact NIC feature table in the presentation and keep the stated priorities and impacts unchanged.

## Fixed NIC Vendor Product Indicative Price Ranges To Use (USD)

Notes:
- Use these as planning-level indicative price bands for later numeric refinement.
- Scope restriction: only consider 100G cards in this table.
- Prefer new listings and exclude explicit REFURBISHED/USED/LIKE NEW/Open Box entries when visible.
- Exclude accessory-only and non-100G listings from range calculation.
- Include dedicated China-brand rows to reflect domestic compatible-card and enterprise-channel options.
- Include an explicit TP-LINK row as a well-known China brand reference.
- Use qualitative bands only: Low, Mid, High, Very High.

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

Use this exact qualitative price-band table in the presentation; you can refine to numeric ranges later if needed.

## Fixed 10GbE Data-Loss Analysis Content To Use

### Current Status
- No confirmed 10GbE data-loss incident is recorded yet.
- At this stage, 10GbE loss is treated as a risk and visibility gap that must be validated with end-to-end counters.
- The current validation topology is direct connect only between FPGA and recon PC NIC.

### Fault-Domain Segmentation
1. FPGA sender (TX generation and sequence continuity).
2. Direct link path (optic/cable path, CRC, physical stability, MTU consistency).
3. NIC and driver path (RX queue depth, drops, ring overflow).
4. UDP socket and receiver application (buffer overflow, recv rate, sequence gap).

### Fixed Data-Loss Localization Checklist To Use

| Stage | Counter or Signal | If mismatch, likely location and action |
|---|---|---|
| Sender TX | FPGA sent packet count and sequence continuity | If sender count or sequence is unstable, investigate FPGA TX logic and pacing first. |
| Direct Link | CRC, error indications, and link stability on both ends | If link errors increase, inspect cable, optic, NIC port state, and MTU alignment on the direct-connect path. |
| NIC RX | NIC RX packets vs RX drops and queue overflow counters | If RX drops rise while link is clean, tune driver, ring, and interrupt settings and verify firmware. |
| UDP/Socket | UDP receive error or drop counters and socket buffer usage | If NIC is clean but UDP drops rise, increase socket buffers and reduce per-packet processing latency. |
| App Sequence | Application sequence gap or reorder counters | If UDP layer looks clean but app shows gaps, profile app pipeline and queue handoff bottlenecks. |

### Fixed Windows Counter Collection Table To Use

| Layer | Command (PowerShell) | Key Metrics | Interpretation |
|---|---|---|---|
| NIC link and driver | `Get-NetAdapterStatistics -Name "<NIC_NAME>"` | ReceivedBytes, ReceivedUnicastPackets, ReceivedDiscardedPackets, ReceivedPacketErrors | If discards or errors increase while sender is stable, suspect NIC or link path. |
| NIC advanced settings | `Get-NetAdapterAdvancedProperty -Name "<NIC_NAME>"` | RSS, interrupt moderation, receive buffers, jumbo settings | Mismatch from expected tuning indicates driver configuration drift. |
| UDP stack | `netstat -s -p udp` | Datagrams Received, Receive Errors, No Ports | Rising receive errors indicate socket or kernel-path pressure. |
| IP stack drops | `netstat -s -p ip` | Received Header Errors, Received Address Errors, Datagrams Discarded | Increases suggest pre-socket drop in IP processing path. |
| Per-second baseline | `Get-Counter '\Network Interface(*)\Packets Received Discarded','\Network Interface(*)\Packets Received Errors','\Processor(_Total)\% Processor Time' -SampleInterval 1 -MaxSamples 60` | Discard or error trend and CPU trend over 60 seconds | First metric that diverges marks likely loss domain and escalation owner. |

### Fixed Immediate Actions To Use
1. Add synchronized 1-second snapshots for FPGA TX, direct-link status, NIC RX, UDP socket, and app sequence counters.
2. Run staged load tests (2G -> 4G -> 6G -> 8G) and identify the first stage where counter divergence appears.
3. Execute A/B isolation runs on the current direct-connect topology (baseline driver vs tuned driver, baseline NIC settings vs tuned NIC settings) to lock the fault domain.
4. Define an escalation rule: once a stage exceeds the loss threshold for consecutive windows, assign owner and run targeted remediation before retest.

Use this exact data-loss analysis structure in the presentation. Do not reduce it to a generic risk statement.

## Style Guidance
- Avoid marketing tone.
- Prefer tables, comparison charts, and execution timelines.
- Make tradeoffs explicit: throughput, integration risk, cost, lock-in, observability, scalability.
- Show clear distinction between current short-term objective (10G) and near-term strategy (100G).
- Keep content suitable for direct conversion into PowerPoint.

## Validation Checklist (Must Pass)
- Output contains exactly 15 slides.
- All fixed tables/matrices in this prompt are included with unchanged numeric values.
- Direct-connect-only topology is clearly stated and not replaced by switch-path assumptions.
- No mandatory slide in the 15-slide blueprint is skipped or merged.

## Final Deliverable Format
Return the result as:
- Slide 1: Title
- Slide 2: ...
- ...
- Slide 15: ...
For each slide, include:
- Title
- Key bullets
- Suggested visual
- Speaker note

Do not write generic presentation filler. Use the project facts above and maintain internal consistency across all slides.
Professional correctness must be ensured; do not create non-standard technical terms; when specialized terms are necessary, provide explanations and cite sources.
