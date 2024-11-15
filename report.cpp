#include "report.h"

#include <uv.h>

#include <spdlog/spdlog.h>

namespace ad {

namespace {

auto
to_handle(uv_udp_t* h) -> uv_handle_t*
{
  return reinterpret_cast<uv_handle_t*>(h);
}

} // namespace

[[nodiscard]] auto
report(const double timestamp, const double anomaly, const char* ip, const int udp_port) -> bool
{
  sockaddr_in address{};

  const auto addr_err = uv_ip4_addr(ip, udp_port, &address);
  if (addr_err) {
    SPDLOG_ERROR("Failed to parse '{}:{}': {}", ip, udp_port, uv_strerror(addr_err));
    return false;
  }

  sockaddr_in bind_address{};
  uv_ip4_addr("0.0.0.0", 0, &bind_address);

  uv_loop_t loop{};
  uv_loop_init(&loop);

  uv_udp_t socket{};
  uv_udp_init(&loop, &socket);
  uv_udp_bind(&socket, reinterpret_cast<const sockaddr*>(&bind_address), UV_UDP_REUSEADDR);
  uv_udp_set_broadcast(&socket, 1);

  uv_udp_send_t write_op;

  double data[2]{ timestamp, anomaly };

  uv_buf_t send_buffer;
  send_buffer.base = reinterpret_cast<char*>(&data[0]);
  send_buffer.len = sizeof(data);

  auto on_write = [](uv_udp_send_t*, const int status) {
    if (status == 0) {
      SPDLOG_DEBUG("Sent anomaly report.");
    } else {
      SPDLOG_ERROR("Failed to send anomaly report: {}", uv_strerror(status));
    }
  };

  const auto send_err =
    uv_udp_send(&write_op, &socket, &send_buffer, 1, reinterpret_cast<const sockaddr*>(&address), on_write);
  if (send_err != 0) {
    SPDLOG_ERROR("Failed to send UDP message: {}", uv_strerror(send_err));
  }

  uv_run(&loop, UV_RUN_DEFAULT);

  // shutdown

  uv_close(to_handle(&socket), nullptr);

  uv_run(&loop, UV_RUN_DEFAULT);

  uv_loop_close(&loop);

  return true;
}

} // namespace ad
