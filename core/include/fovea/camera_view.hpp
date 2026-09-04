#pragma once

#include <string>

namespace fovea {

enum class CameraView { Forward, Reverse };

struct CameraProfile {
    CameraView view = CameraView::Forward;
    std::string mode_id;
    std::string label;
    std::string default_video;
    std::string default_image;
};

[[nodiscard]] CameraProfile camera_profile(CameraView view);

[[nodiscard]] CameraView parse_camera_view(const std::string& value);

[[nodiscard]] std::string default_media_for(const CameraProfile& profile);

[[nodiscard]] std::string default_forward_video();
[[nodiscard]] std::string default_reverse_video();

}  // namespace fovea
