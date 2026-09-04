#include "fovea/tracker.hpp"

#include <algorithm>
#include <cmath>
#include <string>

namespace fovea {

namespace {

bool labels_compatible(const std::string& a, const std::string& b) {
    if (a == b) {
        return true;
    }
    const bool a_large = (a == "truck" || a == "bus");
    const bool b_large = (b == "truck" || b == "bus");
    if (a_large && b_large) {
        return true;
    }
    const bool a_vehicle = (a == "car" || a == "truck" || a == "bus" || a == "motorcycle");
    const bool b_vehicle = (b == "car" || b == "truck" || b == "bus" || b == "motorcycle");
    return a_vehicle && b_vehicle;
}

std::string stabilize_label(const Detection& previous, const Detection& incoming) {
    if (previous.label == incoming.label) {
        return incoming.label;
    }
    // Strong hysteresis: large vehicles especially should not flip truck↔bus↔car.
    const float margin = (previous.label == "truck" || previous.label == "bus" ||
                          incoming.label == "truck" || incoming.label == "bus")
                             ? 0.28F
                             : 0.18F;
    if (incoming.confidence > previous.confidence + margin && incoming.confidence >= 0.55F) {
        return incoming.label;
    }
    return previous.label;
}

BoundingBox predict_bbox(const Detection& det, float dt) {
    BoundingBox box = det.bbox;
    box.x += det.velocity.dx * dt;
    box.y += det.velocity.dy * dt;
    return box;
}

}  // namespace

void Tracker::reset() {
    tracks_.clear();
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
    const bool has_history = !tracks_.empty() && previous_timestamp_seconds_ >= 0.0;
    const float dt = has_history
                         ? static_cast<float>(std::max(0.001, timestamp_seconds - previous_timestamp_seconds_))
                         : 0.0F;

    std::vector<bool> track_used(tracks_.size(), false);
    std::vector<Detection> output;
    output.reserve(frame.detections.size() + tracks_.size());

    // Associate current detections to existing tracks (IoU vs predicted box).
    for (auto& detection : frame.detections) {
        float best_iou = 0.0F;
        int best_index = -1;

        if (has_history) {
            for (std::size_t i = 0; i < tracks_.size(); ++i) {
                if (track_used[i]) {
                    continue;
                }
                if (!labels_compatible(tracks_[i].detection.label, detection.label)) {
                    continue;
                }

                const BoundingBox predicted = predict_bbox(tracks_[i].detection, dt);
                const float iou = intersection_over_union(detection.bbox, predicted);
                // Also try raw previous box — helps when velocity is noisy.
                const float iou_raw =
                    intersection_over_union(detection.bbox, tracks_[i].detection.bbox);
                const float score = std::max(iou, iou_raw);
                if (score > best_iou) {
                    best_iou = score;
                    best_index = static_cast<int>(i);
                }
            }
        }

        if (best_index >= 0 && best_iou >= kMatchIou) {
            auto& track = tracks_[static_cast<std::size_t>(best_index)];
            track_used[static_cast<std::size_t>(best_index)] = true;

            Detection linked = detection;
            linked.id = track.detection.id;
            linked.label = stabilize_label(track.detection, detection);
            linked.velocity = {
                (center_x(detection.bbox) - center_x(track.detection.bbox)) / std::max(dt, 0.001F),
                (center_y(detection.bbox) - center_y(track.detection.bbox)) / std::max(dt, 0.001F),
            };

            track.detection = linked;
            track.miss_count = 0;
            track.hit_count += 1;
            output.push_back(linked);
        } else {
            detection.id = next_id_++;
            detection.velocity = {0.0F, 0.0F};
            tracks_.push_back(TrackState{detection, 0, 1});
            track_used.push_back(true);
            output.push_back(detection);
        }
    }

    // Coast unmatched tracks so a single YOLO miss doesn't delete the truck.
    for (std::size_t i = 0; i < tracks_.size(); ++i) {
        if (i < track_used.size() && track_used[i]) {
            continue;
        }
        auto& track = tracks_[i];
        track.miss_count += 1;
        if (track.miss_count > kMaxCoastMisses) {
            continue;
        }

        Detection coasted = track.detection;
        coasted.bbox = predict_bbox(track.detection, dt);
        // Soften confidence while coasting so UI can show "tracking" state.
        coasted.confidence = std::max(0.25F, track.detection.confidence * 0.92F);
        track.detection = coasted;
        output.push_back(coasted);
    }

    // Prune dead tracks.
    tracks_.erase(std::remove_if(tracks_.begin(), tracks_.end(),
                                 [](const TrackState& t) {
                                     return t.miss_count > kMaxCoastMisses;
                                 }),
                  tracks_.end());

    frame.detections = std::move(output);
    previous_timestamp_seconds_ = timestamp_seconds;
}

}  // namespace fovea
