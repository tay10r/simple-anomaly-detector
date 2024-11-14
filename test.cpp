#include <uikit/main.hpp>

#include <glad/glad.h>
#include <imgui.h>
#include <implot.h>

#include "pipeline.h"

namespace {

class app_impl final : public uikit::app
{
public:
  void setup(uikit::platform&) override
  {
    glGenTextures(1, &texture_);
    glBindTexture(GL_TEXTURE_2D, texture_);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  }

  void teardown(uikit::platform&) override { glDeleteTextures(1, &texture_); }

  void loop(uikit::platform&) override
  {
    update();

    if (ImGui::Begin("Viewport")) {
      render_viewport();
    }

    ImGui::End();
    if (ImGui::Begin("Anomaly Chart")) {
      render_anomaly_chart();
    }
    ImGui::End();
  }

protected:
  void render_anomaly_chart()
  {
    if (!ImPlot::BeginPlot(
          "##AnomalyChart", ImVec2(-1, -1), ImPlotFlags_Crosshairs)) {
      return;
    }

    ImPlot::SetupAxes("", "", ImPlotAxisFlags_AutoFit, ImPlotAxisFlags_AutoFit);

    ImPlot::PlotLine("Anomaly", anomaly_series_.data(), anomaly_series_.size());

    ImPlot::EndPlot();
  }

  void render_viewport()
  {
    if (!ImPlot::BeginPlot("##Viewport", ImVec2(-1, -1), ImPlotFlags_NoFrame)) {
      return;
    }

    ImPlot::PlotImage("##LastFrame",
                      reinterpret_cast<ImTextureID>(texture_),
                      ImPlotPoint(-aspect_, -1),
                      ImPlotPoint(aspect_, 1));

    ImPlot::EndPlot();
  }

  void update()
  {
    if (!pipeline_.iterate()) {
      return;
    }

    anomaly_series_.emplace_back(pipeline_.get_last_anomaly_level());

    const auto& last_frame = pipeline_.get_last_frame();
    const auto w = last_frame.size().width;
    const auto h = last_frame.size().height;

    aspect_ = static_cast<float>(w) / static_cast<float>(h);

    glBindTexture(GL_TEXTURE_2D, texture_);

    glTexImage2D(GL_TEXTURE_2D,
                 0,
                 GL_RGB,
                 w,
                 h,
                 0,
                 GL_RGB,
                 GL_UNSIGNED_BYTE,
                 last_frame.data);
  }

private:
  ad::pipeline pipeline_{ "test.mp4" };
  // ad::pipeline pipeline_{ 0, 640, 480 };

  GLuint texture_{};

  float aspect_{ 1 };

  std::vector<float> anomaly_series_;
};

} // namespace

namespace uikit {

auto
app::create() -> std::unique_ptr<app>
{
  return std::make_unique<app_impl>();
}

} // namespace uikit
