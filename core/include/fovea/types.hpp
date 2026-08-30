#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace fovea {

struct BoundingBox {
    float x = 0.0F;
    float y = 0.0F;
    float width = 0.0F;
    float height = 0.0F;
};

struct Velocity2D {
    float dx = 0.0F;
    float dy = 0.0F;
};

struct Detection {
    std::uint32_t id = 0;
    std::string label;
    float confidence = 0.0F;
    BoundingBox bbox{};
    Velocity2D velocity{};
};

struct PredictedState {
    std::uint32_t detection_id = 0;
    BoundingBox bbox{};
    float horizon_seconds = 2.0F;
};

struct ImageBuffer {
    int width = 0;
    int height = 0;
    int channels = 0;
    std::vector<std::uint8_t> pixels;
};

struct SceneFrame {
    std::uint64_t frame_index = 0;
    double timestamp_seconds = 0.0;
    std::string source_path;
    ImageBuffer image;
    std::vector<Detection> detections;
    std::vector<PredictedState> predictions;
};

struct PipelineMetrics {
    double capture_ms = 0.0;
    double infer_ms = 0.0;
    double predict_ms = 0.0;
    double grok_ms = 0.0;
    double total_ms = 0.0;
};

}  // namespace fovea
