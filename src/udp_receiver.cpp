#include <winsock2.h>
#include <ws2tcpip.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <limits>
#include <string>
#include <string_view>
#include <vector>

#pragma comment(lib, "ws2_32.lib")

namespace {

std::atomic<bool> g_stop_requested{false};

BOOL WINAPI console_ctrl_handler(DWORD ctrl_type) {
  if (ctrl_type == CTRL_C_EVENT || ctrl_type == CTRL_BREAK_EVENT ||
      ctrl_type == CTRL_CLOSE_EVENT) {
    g_stop_requested.store(true);
    return TRUE;
  }
  return FALSE;
}

enum class Mode {
  Structured,
  Raw,
};

constexpr const char* kTransportProfile = "winsock_udp";

struct Config {
  std::string bind_ip = "0.0.0.0";
  uint16_t port = 5001;
  double duration_s = 60.0;
  double report_interval_s = 1.0;
  int recv_buffer_mb = 64;
  Mode mode = Mode::Structured;

  int header_size = 12;
  int sequence_offset = 4;
  int sequence_size = 8;
  bool little_endian = true;

  double target_gbps = 8.0;
  double max_loss_rate = 1.0;
  bool strict_order_fail = false;
  bool strict_invalid_fail = false;
};

struct Stats {
  uint64_t packets = 0;
  uint64_t bytes = 0;
  uint64_t invalid_packets = 0;
  uint64_t out_of_order = 0;
  uint64_t estimated_lost = 0;
  uint64_t min_packet_bytes = std::numeric_limits<uint64_t>::max();
  uint64_t max_packet_bytes = 0;

  bool seen_first_seq = false;
  uint64_t expected_seq = 0;
};

void update_packet_size_stats(int received, Stats* stats) {
  const auto packet_bytes = static_cast<uint64_t>(received);
  stats->min_packet_bytes = std::min(stats->min_packet_bytes, packet_bytes);
  stats->max_packet_bytes = std::max(stats->max_packet_bytes, packet_bytes);
}

void print_usage(const char* exe) {
  std::cerr
      << "Usage: " << exe << " [options]\n"
      << "Options:\n"
      << "  --bind-ip <ip>                 (default: 0.0.0.0)\n"
      << "  --port <1-65535>              (default: 5001)\n"
      << "  --duration-s <seconds>         (default: 60)\n"
      << "  --report-interval-s <seconds>  (default: 1)\n"
      << "  --recv-buffer-mb <MiB>         (default: 64)\n"
      << "  --mode <structured|raw>        (default: structured)\n"
      << "  --header-size <bytes>          (default: 12)\n"
      << "  --seq-offset <bytes>           (default: 4)\n"
      << "  --seq-size <4|8>               (default: 8)\n"
      << "  --endian <little|big>          (default: little)\n"
      << "  --target-gbps <value>          (default: 8.0)\n"
      << "  --max-loss-rate <0..1>         (default: 1.0, disabled)\n"
      << "  --strict-order-fail            (default: off)\n"
      << "  --strict-invalid-fail          (default: off)\n";
}

bool parse_uint16(const std::string& text, uint16_t* out) {
  try {
    const auto value = std::stoul(text);
    if (value == 0 || value > 65535) {
      return false;
    }
    *out = static_cast<uint16_t>(value);
    return true;
  } catch (...) {
    return false;
  }
}

bool parse_int(const std::string& text, int* out) {
  try {
    *out = std::stoi(text);
    return true;
  } catch (...) {
    return false;
  }
}

bool parse_double(const std::string& text, double* out) {
  try {
    *out = std::stod(text);
    return true;
  } catch (...) {
    return false;
  }
}

bool parse_args(int argc, char** argv, Config* cfg) {
  for (int i = 1; i < argc; ++i) {
    const std::string arg = argv[i];

    auto require_value = [&](std::string* value_out) -> bool {
      if (i + 1 >= argc) {
        return false;
      }
      *value_out = argv[++i];
      return true;
    };

    std::string value;
    if (arg == "--bind-ip") {
      if (!require_value(&value)) {
        return false;
      }
      cfg->bind_ip = value;
    } else if (arg == "--port") {
      if (!require_value(&value) || !parse_uint16(value, &cfg->port)) {
        return false;
      }
    } else if (arg == "--duration-s") {
      if (!require_value(&value) || !parse_double(value, &cfg->duration_s) ||
          cfg->duration_s <= 0.0) {
        return false;
      }
    } else if (arg == "--report-interval-s") {
      if (!require_value(&value) || !parse_double(value, &cfg->report_interval_s) ||
          cfg->report_interval_s <= 0.0) {
        return false;
      }
    } else if (arg == "--recv-buffer-mb") {
      if (!require_value(&value) || !parse_int(value, &cfg->recv_buffer_mb) ||
          cfg->recv_buffer_mb <= 0) {
        return false;
      }
    } else if (arg == "--mode") {
      if (!require_value(&value)) {
        return false;
      }
      if (value == "structured") {
        cfg->mode = Mode::Structured;
      } else if (value == "raw") {
        cfg->mode = Mode::Raw;
      } else {
        return false;
      }
    } else if (arg == "--header-size") {
      if (!require_value(&value) || !parse_int(value, &cfg->header_size) ||
          cfg->header_size < 0) {
        return false;
      }
    } else if (arg == "--seq-offset") {
      if (!require_value(&value) || !parse_int(value, &cfg->sequence_offset) ||
          cfg->sequence_offset < 0) {
        return false;
      }
    } else if (arg == "--seq-size") {
      if (!require_value(&value) || !parse_int(value, &cfg->sequence_size) ||
          (cfg->sequence_size != 4 && cfg->sequence_size != 8)) {
        return false;
      }
    } else if (arg == "--endian") {
      if (!require_value(&value)) {
        return false;
      }
      if (value == "little") {
        cfg->little_endian = true;
      } else if (value == "big") {
        cfg->little_endian = false;
      } else {
        return false;
      }
    } else if (arg == "--target-gbps") {
      if (!require_value(&value) || !parse_double(value, &cfg->target_gbps) ||
          cfg->target_gbps <= 0.0) {
        return false;
      }
    } else if (arg == "--max-loss-rate") {
      if (!require_value(&value) || !parse_double(value, &cfg->max_loss_rate) ||
          cfg->max_loss_rate < 0.0 || cfg->max_loss_rate > 1.0) {
        return false;
      }
    } else if (arg == "--strict-order-fail") {
      cfg->strict_order_fail = true;
    } else if (arg == "--strict-invalid-fail") {
      cfg->strict_invalid_fail = true;
    } else if (arg == "--help" || arg == "-h") {
      print_usage(argv[0]);
      return false;
    } else {
      std::cerr << "Unknown argument: " << arg << "\n";
      return false;
    }
  }

  if (cfg->mode == Mode::Structured &&
      cfg->sequence_offset + cfg->sequence_size > cfg->header_size) {
    std::cerr << "Invalid structured settings: seq field exceeds header-size\n";
    return false;
  }

  return true;
}

uint64_t read_uint(const uint8_t* data, int size, bool little_endian) {
  uint64_t value = 0;
  if (little_endian) {
    for (int i = 0; i < size; ++i) {
      value |= static_cast<uint64_t>(data[i]) << (8 * i);
    }
  } else {
    for (int i = 0; i < size; ++i) {
      value = (value << 8) | data[i];
    }
  }
  return value;
}

bool update_sequence_stats(const Config& cfg, const uint8_t* packet, int bytes,
                           Stats* stats) {
  if (bytes < cfg.header_size) {
    return false;
  }

  const uint8_t* seq_ptr = packet + cfg.sequence_offset;
  const uint64_t seq = read_uint(seq_ptr, cfg.sequence_size, cfg.little_endian);

  if (!stats->seen_first_seq) {
    stats->seen_first_seq = true;
    stats->expected_seq = seq + 1;
    return true;
  }

  if (seq == stats->expected_seq) {
    stats->expected_seq += 1;
    return true;
  }

  if (seq > stats->expected_seq) {
    stats->estimated_lost += (seq - stats->expected_seq);
    stats->expected_seq = seq + 1;
    return true;
  }

  stats->out_of_order += 1;
  return true;
}

void print_periodic(const Stats& stats, double elapsed_s) {
  const double gbps = (stats.bytes * 8.0) / std::max(elapsed_s, 1e-9) / 1e9;
  const double pps = stats.packets / std::max(elapsed_s, 1e-9);
  const double avg_packet_bytes =
      stats.packets == 0
          ? 0.0
          : static_cast<double>(stats.bytes) / static_cast<double>(stats.packets);
  std::cout << std::fixed << std::setprecision(3)
            << "[progress] t=" << elapsed_s << "s"
            << " packets=" << stats.packets
            << " pps=" << pps
            << " bytes=" << stats.bytes
            << " avg_pkt_bytes=" << avg_packet_bytes
            << " goodput_gbps=" << gbps
            << " invalid=" << stats.invalid_packets
            << " ooo=" << stats.out_of_order
            << " lost_est=" << stats.estimated_lost << "\n";
}

void print_final(const Config& cfg, const Stats& stats, double elapsed_s, bool pass) {
  const double gbps = (stats.bytes * 8.0) / std::max(elapsed_s, 1e-9) / 1e9;
  const double pps = stats.packets / std::max(elapsed_s, 1e-9);
  const double avg_packet_bytes =
      stats.packets == 0
          ? 0.0
          : static_cast<double>(stats.bytes) / static_cast<double>(stats.packets);
  const uint64_t min_packet_bytes =
      stats.packets == 0 ? 0 : stats.min_packet_bytes;
  const bool jumbo_observed = stats.max_packet_bytes > 1500;
  const uint64_t total_seq_events = stats.packets + stats.estimated_lost;
  const double loss_rate =
      total_seq_events == 0
          ? 0.0
          : static_cast<double>(stats.estimated_lost) /
                static_cast<double>(total_seq_events);
  std::cout << "\nFinal summary\n";
  std::cout << "bind_ip: " << cfg.bind_ip << "\n";
  std::cout << "port: " << cfg.port << "\n";
  std::cout << "transport_profile: " << kTransportProfile << "\n";
  std::cout << "mode: " << (cfg.mode == Mode::Structured ? "structured" : "raw")
            << "\n";
  std::cout << "target_gbps: " << cfg.target_gbps << "\n";
  std::cout << "max_loss_rate: " << cfg.max_loss_rate << "\n";
  std::cout << "strict_order_fail: " << (cfg.strict_order_fail ? "true" : "false")
            << "\n";
  std::cout << "strict_invalid_fail: "
            << (cfg.strict_invalid_fail ? "true" : "false") << "\n";
  std::cout << std::fixed << std::setprecision(6)
            << "elapsed_s: " << elapsed_s << "\n"
            << "packets_per_second: " << pps << "\n"
            << "goodput_gbps: " << gbps << "\n"
            << "avg_packet_bytes: " << avg_packet_bytes << "\n"
            << "min_packet_bytes: " << min_packet_bytes << "\n"
            << "max_packet_bytes: " << stats.max_packet_bytes << "\n"
            << "jumbo_observed: " << (jumbo_observed ? "true" : "false") << "\n"
            << "loss_rate: " << loss_rate << "\n";
  std::cout << "packets: " << stats.packets << "\n";
  std::cout << "bytes: " << stats.bytes << "\n";
  std::cout << "invalid_packets: " << stats.invalid_packets << "\n";
  std::cout << "out_of_order: " << stats.out_of_order << "\n";
  std::cout << "estimated_lost: " << stats.estimated_lost << "\n";
  std::cout << "pass: " << (pass ? "true" : "false") << "\n";
}

}  // namespace

int main(int argc, char** argv) {
  Config cfg;
  if (!parse_args(argc, argv, &cfg)) {
    print_usage(argv[0]);
    return 2;
  }

  if (!SetConsoleCtrlHandler(console_ctrl_handler, TRUE)) {
    std::cerr << "Failed to register console handler\n";
    return 1;
  }

  WSADATA wsa_data;
  const int wsa_result = WSAStartup(MAKEWORD(2, 2), &wsa_data);
  if (wsa_result != 0) {
    std::cerr << "WSAStartup failed with code " << wsa_result << "\n";
    return 1;
  }

  SOCKET sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  if (sock == INVALID_SOCKET) {
    std::cerr << "socket() failed: " << WSAGetLastError() << "\n";
    WSACleanup();
    return 1;
  }

  const int recv_buffer_bytes = cfg.recv_buffer_mb * 1024 * 1024;
  if (setsockopt(sock, SOL_SOCKET, SO_RCVBUF,
                 reinterpret_cast<const char*>(&recv_buffer_bytes),
                 sizeof(recv_buffer_bytes)) != 0) {
    std::cerr << "Warning: failed to set SO_RCVBUF: " << WSAGetLastError() << "\n";
  }

  sockaddr_in addr{};
  addr.sin_family = AF_INET;
  addr.sin_port = htons(cfg.port);

  if (InetPtonA(AF_INET, cfg.bind_ip.c_str(), &addr.sin_addr) != 1) {
    std::cerr << "Invalid bind IP: " << cfg.bind_ip << "\n";
    closesocket(sock);
    WSACleanup();
    return 1;
  }

  if (bind(sock, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) != 0) {
    std::cerr << "bind() failed: " << WSAGetLastError() << "\n";
    closesocket(sock);
    WSACleanup();
    return 1;
  }

  // Use recv timeout to periodically check stop conditions and print reports.
  const DWORD recv_timeout_ms = 200;
  if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO,
                 reinterpret_cast<const char*>(&recv_timeout_ms),
                 sizeof(recv_timeout_ms)) != 0) {
    std::cerr << "Warning: failed to set SO_RCVTIMEO: " << WSAGetLastError() << "\n";
  }

  std::cout << "Receiver started on " << cfg.bind_ip << ":" << cfg.port
            << " transport_profile=" << kTransportProfile
            << " mode=" << (cfg.mode == Mode::Structured ? "structured" : "raw")
            << " target_gbps=" << cfg.target_gbps << "\n";

  constexpr int kMaxPacketSize = 65535;
  std::vector<uint8_t> buffer(static_cast<size_t>(kMaxPacketSize));

  Stats stats;
  const auto start = std::chrono::steady_clock::now();
  auto next_report = start + std::chrono::duration<double>(cfg.report_interval_s);

  while (!g_stop_requested.load()) {
    const int received = recv(sock, reinterpret_cast<char*>(buffer.data()),
                              static_cast<int>(buffer.size()), 0);

    if (received > 0) {
      stats.packets += 1;
      stats.bytes += static_cast<uint64_t>(received);
      update_packet_size_stats(received, &stats);

      if (cfg.mode == Mode::Structured) {
        if (!update_sequence_stats(cfg, buffer.data(), received, &stats)) {
          stats.invalid_packets += 1;
        }
      }
    } else if (received == SOCKET_ERROR) {
      const int err = WSAGetLastError();
      if (err != WSAETIMEDOUT) {
        std::cerr << "recv() failed: " << err << "\n";
        closesocket(sock);
        WSACleanup();
        return 1;
      }
    }

    const auto now = std::chrono::steady_clock::now();
    const double elapsed_s =
        std::chrono::duration<double>(now - start).count();

    if (elapsed_s >= cfg.duration_s) {
      break;
    }

    if (now >= next_report) {
      print_periodic(stats, elapsed_s);
      next_report = now + std::chrono::duration<double>(cfg.report_interval_s);
    }
  }

  const auto end = std::chrono::steady_clock::now();
  const double elapsed_s = std::chrono::duration<double>(end - start).count();
  const double gbps = (stats.bytes * 8.0) / std::max(elapsed_s, 1e-9) / 1e9;
  const uint64_t total_seq_events = stats.packets + stats.estimated_lost;
  const double loss_rate =
      total_seq_events == 0
          ? 0.0
          : static_cast<double>(stats.estimated_lost) /
                static_cast<double>(total_seq_events);

  const bool goodput_ok = (stats.packets > 0) && (gbps >= cfg.target_gbps);
  const bool loss_ok = (loss_rate <= cfg.max_loss_rate);
  const bool order_ok = !cfg.strict_order_fail || (stats.out_of_order == 0);
  const bool invalid_ok = !cfg.strict_invalid_fail || (stats.invalid_packets == 0);

  const bool pass = goodput_ok && loss_ok && order_ok && invalid_ok;
  print_final(cfg, stats, elapsed_s, pass);

  closesocket(sock);
  WSACleanup();
  return pass ? 0 : 3;
}
