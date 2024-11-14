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
report(float anomaly, const char* ip, const int udp_port) -> bool
{
  sockaddr_in address{};

  const auto addr_err = uv_ip4_addr(ip, udp_port, &address);
  if (addr_err) {
    SPDLOG_ERROR("Failed to parse '{}:{}': {}", ip, udp_port, uv_strerror(addr_err));
    return false;
  }

  uv_loop_t loop{};
  uv_loop_init(&loop);

  uv_udp_t socket{};
  uv_udp_init(&loop, &socket);
  uv_udp_set_broadcast(&socket, 1);

  uv_udp_send_t write_op;

  uv_buf_t send_buffer;
  send_buffer.base = reinterpret_cast<char*>(&anomaly);
  send_buffer.len = sizeof(anomaly);

  const auto send_err =
    uv_udp_send(&write_op, &socket, &send_buffer, 1, reinterpret_cast<const sockaddr*>(&address), nullptr);
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
