#include "fovea/tracker.hpp"

#include <algorithm>
#include <cmath>

namespace fovea {

void Tracker::reset() {
    previous_detections_.clear();
    previous_timestamp_seconds_ = -1.0;
    next_id_ = 1;
}

float Tracker::center_x(const BoundingBox& bbox) {
    return bbox.x + bbox.width * 0.5F;
}

float Tracker::center_y(const BoundingBox& bbox) {
    return bbox.y + bbox.height * 0.5F;
}

float Tracker::intersection_over_union(const BoundingBox& a, const BoundingBox& b) {
    const float x1 = std::max(a.x, b.x);
    const float y1 = std::max(a.y, b.y);
    const float x2 = std::min(a.x + a.width, b.x + b.width);
    const float y2 = std::min(a.y + a.height, b.y + b.height);

    const float intersection = std::max(0.0F, x2 - x1) * std::max(0.0F, y2 - y1);
    const float union_area = a.width * a.height + b.width * b.height - intersection;
    return union_area <= 0.0F ? 0.0F : intersection / union_area;
}

void Tracker::update(SceneFrame& frame, double timestamp_seconds) {
    if (frame.detections.empty()) {
        previous_timestamp_seconds_ = timestamp_seconds;
        return;
    }

    const bool has_history =
        !previous_detections_.empty() && previous_timestamp_seconds_ >= 0.0;
    const double delta_seconds =
        has_history ? std::max(0.001, timestamp_seconds - previous_timestamp_seconds_) : 0.0;

    std::vector<bool> previous_used(previous_detections_.size(), false);

    for (auto& detection : frame.detections) {
        float best_iou = 0.0F;
        int best_index = -1;

        if (has_history) {
            for (std::size_t i = 0; i < previous_detections_.size(); ++i) {
                if (previous_used[i]) {
                    continue;
                }
                if (previous_detections_[i].label != detection.label) {
                    continue;
                }

                const float iou =
                    intersection_over_union(detection.bbox, previous_detections_[i].bbox);
                if (iou > best_iou) {
                    best_iou = iou;
                    best_index = static_cast<int>(i);
                }
            }
        }

        if (best_index >= 0 && best_iou >= 0.3F) {
            const auto& previous = previous_detections_[static_cast<std::size_t>(best_index)];
            previous_used[static_cast<std::size_t>(best_index)] = true;
            detection.id = previous.id;

            const float dx =
                (center_x(detection.bbox) - center_x(previous.bbox)) /
                static_cast<float>(delta_seconds);
            const float dy =
                (center_y(detection.bbox) - center_y(previous.bbox)) /
                static_cast<float>(delta_seconds);
            detection.velocity = {dx, dy};
        } else {
            detection.id = next_id_++;
            detection.velocity = {0.0F, 0.0F};
        }
    }

    previous_detections_ = frame.detections;
    previous_timestamp_seconds_ = timestamp_seconds;
}

}  // namespace fovea
