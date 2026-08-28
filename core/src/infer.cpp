#include "fovea/infer.hpp"

namespace fovea {

void InferEngine::run_stub(SceneFrame& frame) const {
    Detection pedestrian{};
    pedestrian.id = 1;
    pedestrian.label = "person";
    pedestrian.confidence = 0.94F;
    pedestrian.bbox = {120.0F, 80.0F, 60.0F, 180.0F};
    pedestrian.velocity = {0.4F, 0.0F};

    Detection car{};
    car.id = 2;
    car.label = "car";
    car.confidence = 0.87F;
    car.bbox = {420.0F, 160.0F, 180.0F, 120.0F};
    car.velocity = {-2.5F, 0.0F};

    frame.detections = {pedestrian, car};
}

void InferEngine::run(SceneFrame& frame) const {
#if defined(FOVEA_HAS_ONNX)
    if (frame.image.pixels.empty()) {
        run_stub(frame);
        return;
    }
    run_onnx_inference(*this, frame);
#else
    run_stub(frame);
#endif
}

#if !defined(FOVEA_HAS_ONNX)
InferEngine::InferEngine(InferConfig config)
    : config_(std::move(config)), impl_(nullptr) {}

InferEngine::~InferEngine() = default;
#endif

}  // namespace fovea
