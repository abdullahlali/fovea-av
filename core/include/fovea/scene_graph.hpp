#pragma once

#include "fovea/types.hpp"

#include <vector>

namespace fovea {

class SceneGraph {
public:
    void update(SceneFrame& frame) const;
    [[nodiscard]] std::string to_json(const SceneFrame& frame) const;
};

}  // namespace fovea
