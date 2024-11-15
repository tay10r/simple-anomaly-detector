#include "pipeline.h"

#include <algorithm>

#include <stdint.h>

#include <math.h>

namespace ad {

pipeline::pipeline(std::string video_path)
  : capture_(video_path)
{
}

pipeline::pipeline(const int index, const int w, const int h)
  : capture_(index)
{
  capture_.set(cv::CAP_PROP_FRAME_WIDTH, w);
  capture_.set(cv::CAP_PROP_FRAME_HEIGHT, h);
}

auto
pipeline::good() const -> bool
{
  return capture_.isOpened();
}

auto
pipeline::iterate() -> bool
{
  if (!capture_.read(last_frame_)) {
    return false;
  }

  last_timestamp_ = static_cast<double>(std::chrono::duration_cast<std::chrono::milliseconds>(
                                          std::chrono::system_clock::now().time_since_epoch())
                                          .count()) /
                    1000.0;

  if (background_frames_.size() < max_background_frames_) {
    add_to_background_model(last_frame_);
    return true;
  }

  last_anomaly_level_ = 0.0F;

  const auto num_pixels = last_frame_.total();

  // This protects against pixels that don't change.
  // In cases like those, any pixel with a different value would have a z score of infinity.
  auto safe_stddev = [](const float stddev) -> float {
    constexpr auto threshold{ 1.0F / 256.0F };
    return (stddev < threshold) ? 1.0F : stddev;
  };

  for (auto i = 0; i < num_pixels; i++) {

    const auto p = last_frame_.at<pixel>(i);

    const auto stddev_r = safe_stddev(background_stddev_[i].rgb[0]);
    const auto stddev_g = safe_stddev(background_stddev_[i].rgb[1]);
    const auto stddev_b = safe_stddev(background_stddev_[i].rgb[2]);

    const auto score_r = (p.rgb[0] - background_mean_[i].rgb[0]) / stddev_r;
    const auto score_g = (p.rgb[1] - background_mean_[i].rgb[1]) / stddev_g;
    const auto score_b = (p.rgb[2] - background_mean_[i].rgb[2]) / stddev_b;

    const auto anomaly = score_r + score_g + score_b;

    last_anomaly_level_ += anomaly;
  }

  last_anomaly_level_ /= static_cast<float>(num_pixels * 3);

  auto background_should_update = background_update_counter == 0;

  if (background_should_update || (fabsf(last_anomaly_level_) < background_anomaly_threshold_)) {
    add_to_background_model(last_frame_);
    background_update_counter = 30;
  } else {
    background_update_counter--;
  }

  return true;
}

void
pipeline::add_to_background_model(const cv::Mat& frame)
{
  if (background_frames_.size() >= max_background_frames_) {
    background_frames_.erase(background_frames_.begin());
  }

  background_frames_.emplace_back();

  frame.copyTo(background_frames_.back());

  if (background_frames_.size() < max_background_frames_) {
    return;
  }

  const auto num_pixels = frame.total();

  background_mean_.resize(num_pixels);

  background_stddev_.resize(num_pixels);

  const auto k = 1.0F / static_cast<float>(background_frames_.size());

  for (auto i = 0; i < num_pixels; i++) {

    std::vector<pixel> samples;

    samples.resize(background_frames_.size());

    for (auto j = 0; j < background_frames_.size(); j++) {
      const auto& bg_frame = background_frames_[j];
      samples[j] = bg_frame.at<pixel>(i);
    }

    pixel_f32 mean{};

    for (const auto& s : samples) {
      mean.rgb[0] += s.rgb[0];
      mean.rgb[1] += s.rgb[1];
      mean.rgb[2] += s.rgb[2];
    }

    mean.rgb[0] *= k;
    mean.rgb[1] *= k;
    mean.rgb[2] *= k;

    pixel_f32 stddev{};

    for (const auto& s : samples) {
      const auto delta_r = mean.rgb[0] - s.rgb[0];
      const auto delta_g = mean.rgb[1] - s.rgb[1];
      const auto delta_b = mean.rgb[2] - s.rgb[2];
      stddev.rgb[0] += delta_r * delta_r;
      stddev.rgb[1] += delta_g * delta_g;
      stddev.rgb[2] += delta_b * delta_b;
    }

    stddev.rgb[0] = sqrtf(stddev.rgb[0] * k);
    stddev.rgb[1] = sqrtf(stddev.rgb[1] * k);
    stddev.rgb[2] = sqrtf(stddev.rgb[2] * k);

    background_mean_[i] = mean;

    background_stddev_[i] = stddev;
  }
}

auto
pipeline::get_last_frame() const -> const cv::Mat&
{
  return last_frame_;
}

auto
pipeline::get_last_timestamp() const -> double
{
  return last_timestamp_;
}

auto
pipeline::get_last_anomaly_level() const -> float
{
  return last_anomaly_level_;
}

} // namespace ad
