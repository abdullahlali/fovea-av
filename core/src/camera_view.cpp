#include "fovea/camera_view.hpp"

#include <cctype>
#include <stdexcept>

namespace fovea {

namespace {

std::string lower_copy(std::string value) {
    for (char& ch : value) {
        ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
    }
    return value;
}

}  // namespace

CameraProfile camera_profile(const CameraView view) {
    switch (view) {
        case CameraView::Reverse: {
            CameraProfile profile{};
            profile.view = CameraView::Reverse;
            profile.mode_id = "reverse";
            profile.label = "Reverse Camera";
            profile.default_video = default_reverse_video();
            profile.default_image = "assets/test/bdd/street.jpg";
            return profile;
        }
        case CameraView::Forward:
        default: {
            CameraProfile profile{};
            profile.view = CameraView::Forward;
            profile.mode_id = "forward";
            profile.label = "Forward Camera";
            profile.default_video = default_forward_video();
            profile.default_image = "assets/test/bdd/street.jpg";
            return profile;
        }
    }
}

CameraView parse_camera_view(const std::string& value) {
    const std::string normalized = lower_copy(value);
    if (normalized == "reverse" || normalized == "r" || normalized == "backup" || normalized == "rear") {
        return CameraView::Reverse;
    }
    if (normalized == "forward" || normalized == "f" || normalized == "front") {
        return CameraView::Forward;
    }
    throw std::invalid_argument("Unknown camera view: " + value + " (use forward or reverse)");
}

std::string default_media_for(const CameraProfile& profile) {
    return profile.default_video;
}

std::string default_forward_video() {
    return "assets/test/dashcam/street.mp4";
}

std::string default_reverse_video() {
    return "assets/test/dashcam/reverse.mp4";
}

}  // namespace fovea
