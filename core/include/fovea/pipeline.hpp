#pragma once

#include "fovea/capture.hpp"
#include "fovea/grok_client.hpp"
#include "fovea/infer.hpp"
#include "fovea/predictor.hpp"
#include "fovea/scene_graph.hpp"
#include "fovea/scenario.hpp"
#include "fovea/tracker.hpp"
#include "fovea/types.hpp"

#include <optional>
#include <string>

#if defined(FOVEA_HAS_VIDEO)
#include "fovea/video_capture.hpp"
#endif

namespace fovea {

struct PipelineOptions {
    bool enable_grok = false;
    bool print_json = true;
    bool enable_tracking = true;
    std::string scenario_path;
};

struct PipelineResult {
    SceneFrame frame;
    PipelineMetrics metrics;
    GrokResponse grok;
};

class Pipeline {
public:
    Pipeline();

    void reset_tracking() const;

    [[nodiscard]] PipelineResult process_image(
        const std::string& image_path,
        const PipelineOptions& options = {}) const;

    [[nodiscard]] PipelineResult process_frame(
        SceneFrame frame,
        const PipelineOptions& options = {}) const;

    // Grok narration only — does not re-run inference on the frame.
    [[nodiscard]] GrokResponse narrate_scene(const SceneFrame& frame) const;

#if defined(FOVEA_HAS_VIDEO)
    [[nodiscard]] PipelineResult process_video_frame(
        VideoCapture& video,
        int frame_index,
        const PipelineOptions& options = {}) const;
#endif

private:
    mutable Capture capture_;
    mutable InferEngine infer_;
    mutable Predictor predictor_;
    mutable SceneGraph scene_graph_;
    mutable GrokClient grok_;
    mutable Tracker tracker_;
    mutable std::optional<Scenario> loaded_scenario_;
    mutable std::string loaded_scenario_path_;
};

}  // namespace fovea
