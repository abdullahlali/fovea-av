#pragma once

#include "fovea/capture.hpp"
#include "fovea/grok_client.hpp"
#include "fovea/infer.hpp"
#include "fovea/predictor.hpp"
#include "fovea/scene_graph.hpp"
#include "fovea/tracker.hpp"
#include "fovea/types.hpp"

#if defined(FOVEA_HAS_VIDEO)
#include "fovea/video_capture.hpp"
#endif

#include <string>

namespace fovea {

struct PipelineOptions {
    bool enable_grok = false;
    bool print_json = true;
    bool enable_tracking = true;
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

#if defined(FOVEA_HAS_VIDEO)
    [[nodiscard]] PipelineResult process_video_frame(
        const VideoCapture& video,
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
};

}  // namespace fovea
