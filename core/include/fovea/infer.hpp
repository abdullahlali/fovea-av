#pragma once

#include "fovea/types.hpp"

namespace fovea {

struct InferConfig {
    std::string model_path = "models/yolov8n.onnx";
    float confidence_threshold = 0.5F;
};

class InferEngine {
public:
    explicit InferEngine(InferConfig config = {});

    void run(SceneFrame& frame) const;

private:
    InferConfig config_;
    void run_stub(SceneFrame& frame) const;
};

}  // namespace fovea
