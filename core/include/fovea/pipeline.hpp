#pragma once

#include "fovea/capture.hpp"
#include "fovea/grok_client.hpp"
#include "fovea/infer.hpp"
#include "fovea/predictor.hpp"
#include "fovea/scene_graph.hpp"
#include "fovea/types.hpp"

#include <string>

namespace fovea {

struct PipelineOptions {
    bool enable_grok = false;
    bool print_json = true;
};

struct PipelineResult {
    SceneFrame frame;
    PipelineMetrics metrics;
    GrokResponse grok;
};

class Pipeline {
public:
    Pipeline();

    [[nodiscard]] PipelineResult process_image(
        const std::string& image_path,
        const PipelineOptions& options = {}) const;

private:
    Capture capture_;
    InferEngine infer_;
    Predictor predictor_;
    SceneGraph scene_graph_;
    GrokClient grok_;
};

}  // namespace fovea
