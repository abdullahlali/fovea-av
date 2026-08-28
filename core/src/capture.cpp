#define STB_IMAGE_IMPLEMENTATION
#include "fovea/capture.hpp"

#include <fstream>
#include <stdexcept>

#include "stb_image.h"

namespace fovea {

namespace {

bool file_exists(const std::string& path) {
    std::ifstream file(path);
    return file.good();
}

}  // namespace

SceneFrame Capture::load_frame(const CaptureRequest& request) const {
    if (!file_exists(request.image_path)) {
        throw std::runtime_error("Image not found: " + request.image_path);
    }

    int width = 0;
    int height = 0;
    int channels = 0;
    unsigned char* data =
        stbi_load(request.image_path.c_str(), &width, &height, &channels, STBI_rgb);

    if (data == nullptr) {
        throw std::runtime_error("Failed to decode image: " + request.image_path);
    }

    SceneFrame frame{};
    frame.frame_index = 0;
    frame.timestamp_seconds = 0.0;
    frame.source_path = request.image_path;
    frame.image.width = width;
    frame.image.height = height;
    frame.image.channels = 3;
    frame.image.pixels.assign(data, data + (width * height * 3));

    stbi_image_free(data);
    return frame;
}

}  // namespace fovea
