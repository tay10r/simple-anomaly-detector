#include <chrono>
#include <iostream>
#include <sstream>
#include <string>

#include <math.h>
#include <stdlib.h>

#include "pipeline.h"
#include "report.h"

#include <opencv2/imgcodecs.hpp>

#include <spdlog/spdlog.h>

#include <cxxopts.hpp>

namespace {

struct options final
{
  int device_index{};

  std::string file;

  std::string report_ip{ "127.0.0.1" };

  float threshold{ 500.0F };

  bool debug{ false };

  [[nodiscard]] auto parse(int argc, char** argv) -> bool
  {
    cxxopts::Options options(argv[0], "Records anomalous images to disk.");

    options.add_options()                                                                                        //
      ("debug", "Enable debug logging.", cxxopts::value<bool>()->default_value("false")->implicit_value("true")) //
      ("device",
       "Which video device to open.",
       cxxopts::value<int>()->default_value(std::to_string(device_index))) //
      ("file",
       "If specified, will instead run the pipeline on a file instead of a real video capture device.",
       cxxopts::value<std::string>()->default_value("")) //
      ("help",
       "Print this help message.",
       cxxopts::value<bool>()->implicit_value("true")->default_value("false")) //
      ("threshold",
       "The anomaly threshold that triggers a report.",
       cxxopts::value<float>()->default_value(std::to_string(threshold))) //
      ("report-ip",
       "The IPv4 address to report anomaly levels on.",
       cxxopts::value<std::string>()->default_value(report_ip)) //
      ;

    try {

      const auto result = options.parse(argc, argv);
      if (result["help"].as<bool>()) {
        std::cout << options.help() << std::endl;
        return false;
      }

      debug = result["debug"].as<bool>();
      device_index = result["device"].as<int>();
      file = result["file"].as<std::string>();
      threshold = result["threshold"].as<float>();
      report_ip = result["report-ip"].as<std::string>();

    } catch (const cxxopts::exceptions::exception& e) {
      SPDLOG_ERROR(e.what());
      return false;
    }

    return true;
  }
};

[[nodiscard]] auto
run_pipeline(ad::pipeline& pipeline, const options& opts) -> bool
{
  while (pipeline.good()) {

    SPDLOG_DEBUG("Running pipeline.");

    if (!pipeline.iterate()) {
      SPDLOG_ERROR("Pipeline failed.");
      return false;
    }

    const auto anomaly = fabsf(pipeline.get_last_anomaly_level());

    SPDLOG_DEBUG("Anomaly level is {}", anomaly);

    if (!ad::report(pipeline.get_last_timestamp(), anomaly, opts.report_ip.c_str())) {
      SPDLOG_ERROR("Failed to publish anomaly report.");
    }

    if (anomaly < opts.threshold) {
      // No need to save the data.
      continue;
    }

    std::ostringstream name_stream;
    name_stream << std::chrono::system_clock::now().time_since_epoch().count();
    name_stream << '_';
    name_stream << static_cast<int>(anomaly);
    name_stream << ".png";
    const auto name = name_stream.str();

    SPDLOG_INFO("Reporting anomaly of level {} to path '{}'.", anomaly, name);

    cv::imwrite(name, pipeline.get_last_frame());
  }

  return true;
}

} // namespace

auto
main(int argc, char** argv) -> int
{
  options opts;

  if (!opts.parse(argc, argv)) {
    return EXIT_FAILURE;
  }

  if (opts.debug) {
    spdlog::set_level(spdlog::level::debug);
  }

  if (opts.file.empty()) {
    SPDLOG_INFO("Starting pipeline with device '{}' and anomaly threshold of '{}'.", opts.device_index, opts.threshold);
    ad::pipeline pipeline(opts.device_index, 640, 480);
    return run_pipeline(pipeline, opts) ? EXIT_SUCCESS : EXIT_FAILURE;
  } else {
    SPDLOG_INFO("Starting pipeline with video file '{}' and anomaly threshold of '{}'.", opts.file, opts.threshold);
    ad::pipeline pipeline(opts.file);
    return run_pipeline(pipeline, opts) ? EXIT_SUCCESS : EXIT_FAILURE;
  }
}
