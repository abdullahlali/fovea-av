#pragma once

#include "fovea/types.hpp"

namespace fovea {

class Tracker {
public:
    void reset();
    void update(SceneFrame& frame, double timestamp_seconds);

private:
    std::vector<Detection> previous_detections_;
    double previous_timestamp_seconds_ = -1.0;
    std::uint32_t next_id_ = 1;

    static float intersection_over_union(const BoundingBox& a, const BoundingBox& b);
    static float center_x(const BoundingBox& bbox);
    static float center_y(const BoundingBox& bbox);
};

}  // namespace fovea
