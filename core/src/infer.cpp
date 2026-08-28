#include "fovea/infer.hpp"

namespace fovea {

InferEngine::InferEngine(InferConfig config) : config_(std::move(config)) {}

void InferEngine::run_stub(SceneFrame& frame) const {
    Detection pedestrian{};
    pedestrian.id = 1;
    pedestrian.label = "pedestrian";
    pedestrian.confidence = 0.94F;
    pedestrian.bbox = {120.0F, 80.0F, 60.0F, 180.0F};
    pedestrian.velocity = {0.4F, 0.0F};

    Detection car{};
    car.id = 2;
    car.label = "car";
    car.confidence = 0.87F;
    car.bbox = {420.0F, 160.0F, 180.0F, 120.0F};
    car.velocity = {-2.5F, 0.0F};

    Detection traffic_light{};
    traffic_light.id = 3;
    traffic_light.label = "traffic_light";
    traffic_light.confidence = 0.76F;
    traffic_light.bbox = {300.0F, 40.0F, 30.0F, 80.0F};
    traffic_light.velocity = {0.0F, 0.0F};

    frame.detections = {pedestrian, car, traffic_light};
}

void InferEngine::run(SceneFrame& frame) const {
#if defined(FOVEA_HAS_ONNX)
    (void)config_;
    run_stub(frame);
#else
    (void)config_;
    run_stub(frame);
#endif
}

}  // namespace fovea
