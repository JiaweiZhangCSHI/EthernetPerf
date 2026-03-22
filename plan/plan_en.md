# Execution Plan (EN): CT Raw Data Receiver Scale-Up

## 1. Objective
Drive follow-up work from current Phase 1 (10GbE) toward near-term 100G capability, while preserving a controlled escalation path to dedicated acquisition-card solutions and future 200G planning.

## 2. Scope
In scope:
- 10G early bring-up stabilization and later qualification closure.
- 100G option evaluation and proof campaign.
- Quantitative comparison and go/no-go decision gates.
- Vendor capability and integration risk assessment.

Out of scope (this planning cycle):
- Immediate RDMA implementation.
- Full custom production driver development.
- Security hardening for production release.

Current topology assumption:
- The present validation scene uses direct connect only between FPGA and recon PC NIC.
- Switch-based topology is not part of the current baseline and should only be treated as a future extension or comparison scenario.

## 3. Background: Key Factors and Technical Discussion
- Packet-rate and PPS pressure: At 100G, small-packet traffic can hit packet-per-second limits before line-rate throughput is reached.
- CPU and NUMA locality: Core affinity and NIC queue placement must align with NUMA topology to avoid cross-socket penalties.
- Windows socket-path overhead: WinSock UDP processing adds per-packet overhead that can consume CPU headroom at high PPS.
- MTU and jumbo frames: Jumbo frames reduce PPS load and per-byte overhead, but only if end-to-end path support is validated.
- Observability depth: Queue-level drop visibility, timestamp quality, and reorder tracking are required for accurate bottleneck diagnosis.
- Integration and driver maturity risk: Driver/SDK quality varies across vendors and directly affects schedule and stability risk.
- Cost and vendor lock-in: SmartNIC/custom-card paths can increase performance headroom but also increase long-term switching cost.
- 200G scalability path: 100G architecture choices should preserve a migration path to 200G without full redesign.
- NIC DMA capability: Direct memory transfer behavior strongly influences CPU utilization and sustained receive goodput.
- NIC queue and interrupt behavior: RSS/multi-queue, ring depth, and interrupt moderation settings are first-order tuning levers.
- NIC hardware offload set: Checksum offload, RSC/LRO, and hardware timestamp support affect throughput efficiency and observability quality.

## 4. Roadmap and Phases

### Phase 0.5: Current 10G Early Bring-up and Later Qualification (Immediate)
1. Lock stable baseline on Windows UDP receive path over 10GbE.
2. Meet two-stage acceptance baseline:
- early bring-up gate: sustained goodput >= 8 Gbps, run duration >= 60 s, and 3 consecutive pass runs;
- later qualification gate: validate with per-scan confidence evidence under CT operation model assumptions (10-year service life, about 300 patients/day, about 10-second raw-data burst per scan; equivalent active-capture budget about 3000 seconds/day).
3. Freeze baseline artifacts:
- final KPI output template,
- CPU baseline under acceptance load,
- packet-size distribution and jumbo-observed flag,
- tuning record (NIC settings, socket buffer, core affinity).
4. Add staged loss localization workflow and evidence pack:
- synchronized 1-second counters across sender/direct-link/NIC/UDP/app,
- staged load ramps (2G -> 4G -> 6G -> 8G),
- A/B isolation runs on direct connect (baseline driver vs tuned driver, baseline NIC settings vs tuned NIC settings),
- fault-domain conclusion report for first divergence point.
5. Add Windows command checklist execution record for each acceptance run:
- `Get-NetAdapterStatistics -Name "<NIC_NAME>"`,
- `Get-NetAdapterAdvancedProperty -Name "<NIC_NAME>"`,
- `netstat -s -p udp`,
- `netstat -s -p ip`,
- `Get-Counter '\Network Interface(*)\Packets Received Discarded','\Network Interface(*)\Packets Received Errors','\Processor(_Total)\% Processor Time' -SampleInterval 1 -MaxSamples 60`.

Exit criteria to Phase 1:
- 10G bring-up gate is repeatable,
- later qualification confidence evidence is complete for the 95% confidence bucket at p0 = 99.50%,
- measurement data is trusted,
- baseline tuning is documented,
- loss localization evidence is complete and the first divergence stage is identified whenever mismatch appears,
- no unexplained counter mismatch remains open at 8 Gbps target run,
- CPU <= 30% is demonstrated in later qualification evidence.

### Phase 1: 100G Planning and Procurement Readiness
1. Build shortlist for three lanes:
- Standard high-speed Ethernet NIC,
- SmartNIC/capture card,
- Dedicated FPGA/custom acquisition card.
2. Collect vendor matrix:
- Windows driver maturity,
- SDK/API stability,
- timestamp/observability capability,
- lead time and support model,
- DMA and RSS/multi-queue capability,
- RX/TX ring buffer tunability and interrupt moderation controls,
- checksum offload, RSC/LRO, jumbo-frame support, and hardware timestamp support.

### Phase 2: 100G Proof and A/B Benchmark Campaign
1. Execute in prioritized order:
- Lane A: Ethernet NIC first,
- Lane B: SmartNIC/capture on escalation,
- Lane C: dedicated FPGA/custom only for roadmap-grade determinism.
2. Run workloads:
- large payload profile,
- mixed payload profile,
- small-packet stress profile.
3. Track metrics:
- sustained goodput,
- estimated loss and reorder,
- CPU utilization and headroom,
- pps and queue-level observability,
- per-scan confidence evidence using the same method selected in late Phase 0.5.

### Phase 3: Decision and Escalation
1. Keep Ethernet-first if gates pass consistently.
2. Escalate to SmartNIC/capture if repeated gate failure persists after tuning.
3. Consider dedicated FPGA/custom when deterministic high-throughput requirements justify higher cost and integration effort.
4. Keep Linux as an allowed escalation platform when Windows optimization headroom is insufficient.

## 5. 100G Comparison Table

| Option | Expected Throughput (100G target) | Packet-Rate Tolerance | CPU Overhead Risk | Windows Integration Risk | Implementation Effort | Cost Level | Recommendation |
|---|---|---|---|---|---|---|---|
| Standard high-speed Ethernet NIC + tuned UDP | Mid-High to High | Medium (small-packet PPS sensitive) | Medium-High | Low-Mid | Low-Mid | Low-Mid | Primary path for near-term 100G proof |
| SmartNIC / capture acceleration card | High | High | Medium | Mid-High (SDK dependent) | Mid-High | High | First escalation if Ethernet lane repeatedly fails |
| Dedicated FPGA/custom acquisition card | Very High | Very High | Low-Medium (if co-designed well) | High | Very High | Very High | Use when deterministic throughput is mandatory |

## 6. 100G Weighted Scoring (Decision Support)

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

Interpretation:
- Ethernet-first is preferred for near-term 100G because integration speed and Windows maturity dominate delivery risk.
- Escalate to SmartNIC/capture only after repeated post-tuning gate failures.

## 7. Acceptance Gates (Template)
1. Throughput gate: sustained goodput target for full acceptance window.
2. Quality gate: loss/reorder under defined threshold.
3. Lifecycle statistical gate: use the per-scan confidence method as the primary later-stage gate.
- Phase 0.5 later-stage target: per-scan success rate p0 = 99.50%,
- Phase 0.5 later-stage default confidence: 95% with at least 598 zero-failure windows,
- 100G qualification default confidence: 95% with at least 29956 zero-failure windows,
- optional stronger release evidence: 99% confidence with at least 46050 zero-failure windows.
Supplemental day/week/deep monitoring windows may still be used for engineering trend tracking, but they do not replace the primary confidence gate.
4. Efficiency gate: average CPU usage <= 30%.
5. Stability gate: repeatable pass over consecutive runs.

## 8. Governance and Deliverables
1. Weekly checkpoint package:
- benchmark status,
- blockers and mitigation,
- vendor/procurement updates,
- next-week plan.
2. Milestone review package:
- lane comparison summary,
- weighted score update,
- go/no-go recommendation.
3. Final package:
- selected path,
- risk register,
- timeline and resource estimate for implementation.
