# EthernetPerf Phase 1 Requirement

## 1. Objective
Build and validate a Windows receiver that can ingest high-rate UDP data sent from an existing FPGA sender over 10GbE.

Phase 1 objective:
- deliver a C++ command-line receiver for Windows,
- measure sustained goodput and packet behavior,
- pass an early bring-up baseline for functional convergence and a later qualification baseline based on per-scan confidence evidence aligned to CT operation assumptions.

## 2. Scope
In scope:
- Windows receiver implementation in C++.
- UDP data ingest over IPv4 on 10GbE network.
- Runtime statistics and final pass/fail output.
- Validation procedure for functional and performance baseline.

Out of scope:
- FPGA sender development or modifications.
- GUI receiver.
- Encryption, authentication, or security hardening.
- Production-grade retransmission and reliability protocol.
- RDMA transport implementation.

## 3. System Boundary
Sender side:
- FPGA sender is already implemented and treated as fixed for Phase 1.

Receiver side:
- runs on Windows PC,
- binds UDP socket to configured local IP/port,
- parses incoming packets,
- reports metrics and acceptance result.

Network:
- controlled lab environment,
- 10GbE link between FPGA and PC (direct or through switch).

RDMA stance:
- Phase 1 receiver is explicitly based on standard Windows UDP socket receive path.
- RDMA evaluation is deferred to a later phase and should only be introduced if measured CPU or copy overhead becomes a limiting factor for higher-speed targets.

## 4. Protocol Contract
Transport:
- UDP over IPv4.

Packet format:
- primary mode: custom fixed header + payload from FPGA sender,
- fallback mode: raw UDP payload-only counting for bring-up diagnostics.

Header contract for primary mode:
- sender and receiver must agree on field layout, field sizes, and byte order,
- receiver must at minimum support extracting session identity and packet sequence index,
- exact field map will be stored in implementation constants and must match FPGA sender wire format.

MTU and packet size:
- sender packet size is assumed to avoid IP fragmentation for target lab configuration,
- receiver must tolerate variable payload length up to configured UDP datagram limit.
- Jumbo Frames are optional for Phase 1 functional bring-up but should be benchmarked explicitly because they are the preferred path for scaling beyond 10GbE.

## 5. Functional Requirements
FR-1 Socket binding:
- receiver shall accept CLI arguments for bind IP and UDP port and fail with clear message if bind fails.

FR-2 Data path:
- receiver shall continuously receive UDP packets until stop condition is met.

FR-3 Stop conditions:
- receiver shall support stop-by-duration and manual stop.

FR-4 Statistics:
- receiver shall maintain counters for:
	- total packets,
	- total bytes,
	- invalid packets (parse failures in primary mode),
	- out-of-order packets (when sequence field is available),
	- lost packet estimate (derived from sequence gaps, when sequence field is available).

FR-5 Reporting:
- receiver shall print periodic report at configurable interval,
- receiver shall print final summary including elapsed time and computed goodput,
- receiver shall report packet-size observability sufficient to determine whether jumbo-sized traffic is being received in practice.

FR-6 Exit status:
- receiver shall return process exit code 0 on pass and non-zero on fail/error.

## 6. Performance Requirements
PR-1 Baseline threshold:
- sustained goodput shall be >= 8.0 Gbps during acceptance run.

PR-2 Bring-up run duration:
- bring-up acceptance run duration shall be at least 60 seconds.

PR-3 Bring-up stability:
- receiver shall complete at least 3 consecutive bring-up runs without crash or hang.

PR-4 Loss visibility:
- receiver shall report packet loss estimate (if sequence info exists) and invalid packet count.

PR-5 Lifecycle reliability workload model:
- lifecycle reliability validation shall use CT operation assumptions of 10-year service life, about 300 patients per day, and about 10-second raw-data burst per scan.
- the model-equivalent active capture load is about 3000 seconds per day and shall be represented in accelerated test windows.

PR-6 Lifecycle reliability evidence:
- under modeled windows, the receiver shall show no unexplained loss trend and no crash/hang.

PR-7 Later qualification per-scan confidence criterion:
- define one statistical window as one 10-second scan-equivalent raw-data burst.
- later Phase 0.5 qualification and later 100G readiness shall use a per-scan success-rate confidence method as the primary statistical gate.
- Phase 0.5 later-stage target is per-scan success rate p0 = 99.50%.
- Phase 0.5 later-stage validation gate uses one-sided confidence C = 95% with zero-failure evidence threshold n >= 598 windows.
- 100G qualification gate uses one-sided confidence C = 95% with zero-failure evidence threshold n >= 29956 windows.
- optional sensitivity buckets for Phase 0.5 later-stage target p0 = 99.50% are:
	- 90% confidence: n >= 460 windows,
	- 95% confidence: n >= 598 windows,
	- 99% confidence: n >= 919 windows.
- optional sensitivity buckets for 100G target p0 = 99.99% are:
	- 90% confidence: n >= 23025 windows,
	- 95% confidence: n >= 29956 windows,
	- 99% confidence: n >= 46050 windows.

PR-9 Supplemental monitoring windows (optional):
- one-day monitoring horizon: 300 windows.
- one-week monitoring horizon: 2100 windows.
- deep monitoring horizon: 10000 windows.
- these windows are retained for engineering trending and debug efficiency, but are not the primary later-phase qualification gate.

PR-8 CPU efficiency criterion:
- during bring-up and lifecycle acceptance windows, average CPU usage (Processor(_Total) % Processor Time) shall be <= 30%.

## 7. Observability and Output
Required final summary fields:
- bind IP,
- port,
- transport profile,
- mode (primary/fallback),
- elapsed seconds,
- packets received,
- packets per second,
- bytes received,
- average packet bytes,
- minimum packet bytes,
- maximum packet bytes,
- jumbo observed flag,
- goodput in Gbps,
- invalid packet count,
- out-of-order count,
- estimated lost packets,
- pass/fail decision.

## 8. Acceptance Criteria
AC-1 Functional:
- receiver starts, binds socket, receives packets from FPGA sender, and exits cleanly.

AC-2 Bring-up throughput:
- measured sustained goodput >= 8.0 Gbps for at least 60 seconds.

AC-3 Bring-up repeatability:
- AC-1 and AC-2 pass on 3 consecutive runs.

AC-4 Lifecycle reliability:
- modeled CT workload windows complete without unexplained loss trend, crash, or hang.

AC-5 Later qualification confidence evidence:
- later Phase 0.5 qualification shall meet per-scan target p0 = 99.50% at one-sided 95% confidence.
- later Phase 0.5 acceptance requires zero observed failed windows over at least 598 windows.
- 100G qualification shall meet per-scan target p0 = 99.99% at one-sided 95% confidence with zero observed failed windows over at least 29956 windows.
- stronger optional release evidence may use 99% confidence with zero observed failed windows over at least 46050 windows.

AC-8 Supplemental monitoring reporting:
- if one-day, one-week, or 10000-window monitoring campaigns are run, their results shall be reported as engineering trend data only and shall not replace AC-5.

AC-6 CPU ceiling:
- average CPU usage shall be <= 30% in the assessed acceptance windows.

AC-7 Reporting completeness:
- final summary includes all required fields in Section 7.

## 9. Test Matrix
T-1 Bring-up test:
- short run (10 seconds), verify connectivity and non-zero packet/byte counters.

T-2 Baseline performance test:
- 60-second run under target sender load, verify AC-2.

T-3 Bring-up repeatability test:
- execute T-2 three times, verify AC-3.

T-4 Robustness test:
- run with malformed or partial packets (if available), verify invalid packet accounting and process stability.

T-5 Lifecycle reliability campaign:
- execute accelerated workload windows mapped to the CT model (10-year, about 300 patients/day, about 10-second burst/scan) and verify AC-4.

T-6 Later qualification per-scan confidence campaign:
- run scan-equivalent windows (10 seconds each) until the selected zero-failure confidence bucket is reached.
- Phase 0.5 later-stage default bucket is p0 = 99.50%, C = 95%, requiring at least 598 windows with zero observed failed windows.
- 100G qualification default bucket is p0 = 99.99%, C = 95%, requiring at least 29956 windows with zero observed failed windows.
- each window passes only when no packet loss, no invalid packet, no crash/hang, and throughput and CPU gates are met.
- report target p0, confidence C, tested windows n, observed failed windows k, and whether the selected confidence bucket is satisfied.

T-7 CPU ceiling verification:
- collect CPU counters during T-2/T-3/T-6 windows and compute average CPU usage.
- pass criterion: average CPU usage <= 30% for each assessed acceptance window set.

T-8 One-day monitoring test:
- run 300 windows and report failures, but treat result as supplemental engineering monitoring only.

T-9 One-week monitoring test:
- run 2100 windows and report failures, but treat result as supplemental engineering monitoring only.

T-10 Deep monitoring test:
- run 10000 windows and report failures for trend comparison, but do not use this test alone as a substitute for T-6.

## 10. Risks and Mitigations
Risk R-1:
- exact FPGA header map not synchronized with receiver implementation.
Mitigation:
- keep header constants isolated in one source file and validate against FPGA document before baseline test.

Risk R-2:
- OS/NIC settings prevent reaching 8 Gbps.
Mitigation:
- document socket buffer and NIC tuning settings used during acceptance tests.

Risk R-3:
- packet loss at high line rate.
Mitigation:
- expose counters and run in controlled network first; optimize buffer sizes in later phase if needed.

## 11. Deliverables
- D-1: this requirement document.
- D-2: Windows C++ UDP receiver CLI source code.
- D-3: execution instructions and acceptance test commands.

## 12. Open Items
- O-1: confirm exact FPGA header field layout (names, offsets, endian).
- O-2: finalize strict policy for out-of-order handling in pass/fail (count-only vs fail-condition).
- O-3: define explicit trigger criteria for considering RDMA in future phases.