#include "fovea/capture.hpp"

namespace fovea {

SceneFrame Capture::load_frame(const CaptureRequest& request) const {
    SceneFrame frame{};
    frame.frame_index = 0;
    frame.timestamp_seconds = 0.0;
    frame.source_path = request.image_path;
    return frame;
}

}  // namespace fovea
