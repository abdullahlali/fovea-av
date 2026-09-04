#pragma once

#include "fovea/types.hpp"

#include <cstdint>
#include <vector>

namespace fovea {

struct TrackState {
    Detection detection;
    int miss_count = 0;
    int hit_count = 1;
};

class Tracker {
public:
    void reset();
    void update(SceneFrame& frame, double timestamp_seconds);

private:
    std::vector<TrackState> tracks_;
    double previous_timestamp_seconds_ = -1.0;
    std::uint32_t next_id_ = 1;

    static constexpr int kMaxCoastMisses = 12;  // keep ghosting ~1–2s across missed frames
    static constexpr float kMatchIou = 0.18F;

    static float intersection_over_union(const BoundingBox& a, const BoundingBox& b);
    static float center_x(const BoundingBox& bbox);
    static float center_y(const BoundingBox& bbox);
};

}  // namespace fovea
