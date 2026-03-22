# EthernetPerf Phase 1 Windows Receiver (C++)

This folder contains a UDP receiver CLI for Windows.

## Build (Pure Visual Studio Solution)

Open the solution in Visual Studio 2022:

- `src\\EthernetPerfReceiver.sln`

Then build `Release | x64`.

### CLI build with MSBuild (Developer PowerShell)

From the repository root:

```powershell
msbuild .\src\EthernetPerfReceiver.sln /t:Build /p:Configuration=Release /p:Platform=x64
```

Output binary:
- `src\bin\Release\udp_receiver.exe`

## Run

Example run (structured mode):

```powershell
src\bin\Release\udp_receiver.exe --bind-ip 0.0.0.0 --port 5001 --duration-s 60 --target-gbps 8.0 --mode structured --header-size 12 --seq-offset 4 --seq-size 8 --endian little
```

Example run (strict benchmark gate for product comparison):

```powershell
src\bin\Release\udp_receiver.exe --bind-ip 0.0.0.0 --port 5001 --duration-s 60 --target-gbps 8.0 --mode structured --header-size 12 --seq-offset 4 --seq-size 8 --endian little --max-loss-rate 0.001 --strict-order-fail --strict-invalid-fail
```

Example run (raw mode fallback):

```powershell
src\bin\Release\udp_receiver.exe --bind-ip 0.0.0.0 --port 5001 --duration-s 60 --target-gbps 8.0 --mode raw
```

Example run for MTU comparison:

```powershell
src\bin\Release\udp_receiver.exe --bind-ip 0.0.0.0 --port 5001 --duration-s 30 --mode raw
```

## Notes

- Structured mode assumes custom FPGA header and extracts sequence to estimate loss/out-of-order.
- If header settings do not match FPGA sender, use raw mode for basic throughput bring-up.
- Current implementation uses the standard Windows UDP socket path and reports `transport_profile: winsock_udp` in its final summary.
- RDMA is not part of Phase 1; if a later RDMA-based receiver is added, keep the same summary fields so UDP and RDMA runs remain directly comparable.
- Final summary includes packet-rate and packet-size statistics so standard MTU and jumbo-frame runs can be compared directly.
- `jumbo_observed: true` means at least one received UDP datagram exceeded 1500 bytes.
- Pass condition includes:
	- received packets > 0,
	- measured goodput >= `--target-gbps`,
	- estimated loss rate <= `--max-loss-rate`,
	- if enabled, zero out-of-order packets (`--strict-order-fail`),
	- if enabled, zero invalid packets (`--strict-invalid-fail`).
