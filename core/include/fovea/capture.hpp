#pragma once

#include "fovea/types.hpp"

#include <string>

namespace fovea {

struct CaptureRequest {
    std::string image_path;
};

class Capture {
public:
    [[nodiscard]] SceneFrame load_frame(const CaptureRequest& request) const;
};

}  // namespace fovea
