#pragma once

#include "fovea/types.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace fovea {

struct ScenarioHazard {
    std::string label;
    float confidence = 0.0F;
    BoundingBox bbox{};
    int trigger_frame = -1;  // -1 = active on every frame
};

struct Scenario {
    std::string name;
    std::string description;
    std::vector<ScenarioHazard> hazards;
};

[[nodiscard]] Scenario load_scenario(const std::string& path);

void apply_scenario(SceneFrame& frame, const Scenario& scenario);

}  // namespace fovea
