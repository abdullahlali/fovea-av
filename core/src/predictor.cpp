#include "fovea/predictor.hpp"

namespace fovea {

Predictor::Predictor(float horizon_seconds) : horizon_seconds_(horizon_seconds) {}

void Predictor::apply(SceneFrame& frame) const {
    frame.predictions.clear();
    frame.predictions.reserve(frame.detections.size());

    for (const auto& detection : frame.detections) {
        PredictedState predicted{};
        predicted.detection_id = detection.id;
        predicted.horizon_seconds = horizon_seconds_;
        predicted.bbox.x =
            detection.bbox.x + (detection.velocity.dx * horizon_seconds_);
        predicted.bbox.y =
            detection.bbox.y + (detection.velocity.dy * horizon_seconds_);
        predicted.bbox.width = detection.bbox.width;
        predicted.bbox.height = detection.bbox.height;
        frame.predictions.push_back(predicted);
    }
}

}  // namespace fovea
