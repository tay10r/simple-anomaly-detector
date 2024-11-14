#pragma once

namespace ad {

[[nodiscard]] auto
report(float anomaly, const char* ip, const int udp_port = 5205) -> bool;

} // namespace ad
