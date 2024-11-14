#pragma once

namespace ad {

[[nodiscard]] auto
report(double timestamp, double anomaly, const char* ip, const int udp_port = 5205) -> bool;

} // namespace ad
