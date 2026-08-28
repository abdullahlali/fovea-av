#pragma once

#include "fovea/types.hpp"

#include <memory>
#include <string>

namespace fovea {

struct InferConfig {
    std::string model_path = "models/yolov8n.onnx";
    float confidence_threshold = 0.35F;
    float nms_threshold = 0.45F;
    int input_size = 640;
};

class InferEngine {
public:
    explicit InferEngine(InferConfig config = {});
    ~InferEngine();

    InferEngine(const InferEngine&) = delete;
    InferEngine& operator=(const InferEngine&) = delete;

    void run(SceneFrame& frame) const;

private:
    struct Impl;
    InferConfig config_;
    mutable std::unique_ptr<Impl> impl_;

    void run_stub(SceneFrame& frame) const;

#if defined(FOVEA_HAS_ONNX)
    friend void run_onnx_inference(const InferEngine& engine, SceneFrame& frame);
#endif
};

#if defined(FOVEA_HAS_ONNX)
void run_onnx_inference(const InferEngine& engine, SceneFrame& frame);
#endif

}  // namespace fovea
