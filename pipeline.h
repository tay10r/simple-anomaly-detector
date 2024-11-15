#pragma once

#include <opencv2/videoio.hpp>

#include <chrono>

#include <stdint.h>

namespace ad {

template<typename Scalar>
struct basic_pixel final
{
  Scalar rgb[3];

  template<typename OtherScalar>
  void copy_from(const basic_pixel<OtherScalar>& other)
  {
    rgb[0] = static_cast<OtherScalar>(other.rgb[0]);
    rgb[1] = static_cast<OtherScalar>(other.rgb[1]);
    rgb[2] = static_cast<OtherScalar>(other.rgb[2]);
  }
};

using pixel = basic_pixel<uint8_t>;

using pixel_f32 = basic_pixel<float>;

class timer final
{
public:
  timer(const std::chrono::milliseconds interval)
    : interval_(interval)
  {
  }

  void reset() { start_time_ = std::chrono::high_resolution_clock::now(); }

  auto check() const -> bool
  {
    const auto t = std::chrono::high_resolution_clock::now();

    const auto dt = t - start_time_;

    return dt >= interval_;
  }

private:
  std::chrono::milliseconds interval_;

  std::chrono::high_resolution_clock::time_point start_time_;
};

class pipeline final
{
public:
  pipeline(std::string video_path);

  pipeline(int device_index, int w, int h);

  [[nodiscard]] auto good() const -> bool;

  [[nodiscard]] auto iterate() -> bool;

  [[nodiscard]] auto get_last_frame() const -> const cv::Mat&;

  [[nodiscard]] auto get_last_timestamp() const -> double;

  [[nodiscard]] auto get_last_anomaly_level() const -> float;

protected:
  void add_to_background_model(const cv::Mat& frame);

private:
  cv::VideoCapture capture_;

  cv::Mat last_frame_;

  double last_timestamp_{};

  /**
   * @brief Only non-anomaly frames get put here.
   */
  std::vector<cv::Mat> background_frames_;

  /**
   * @brief The anomaly level of the last frame.
   */
  float last_anomaly_level_{};

  /**
   * @brief The maximum number of background frames.
   */
  int max_background_frames_{ 8 };

  /**
   * @brief If the anomaly threshold is above this value, do not use it to
   * update the background model.
   */
  float background_anomaly_threshold_{ 0.5F };

  std::vector<pixel_f32> background_mean_;

  std::vector<pixel_f32> background_stddev_;

  int background_update_counter{ 0 };
};

} // namespace ad
