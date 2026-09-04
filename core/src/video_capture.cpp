#include "fovea/video_capture.hpp"

#include <opencv2/opencv.hpp>

#include <algorithm>
#include <cctype>
#include <stdexcept>
#include <utility>

namespace fovea {

namespace {

std::string lowercase_extension(const std::string& path) {
    const auto dot = path.find_last_of('.');
    if (dot == std::string::npos) {
        return {};
    }
    std::string extension = path.substr(dot);
    std::transform(extension.begin(), extension.end(), extension.begin(),
                   [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
    return extension;
}

ImageBuffer mat_to_image_buffer(const cv::Mat& bgr) {
    cv::Mat rgb;
    cv::cvtColor(bgr, rgb, cv::COLOR_BGR2RGB);

    ImageBuffer image{};
    image.width = rgb.cols;
    image.height = rgb.rows;
    image.channels = 3;
    image.pixels.assign(rgb.data, rgb.data + (rgb.total() * rgb.channels()));
    return image;
}

}  // namespace

struct VideoCapture::Impl {
    std::string video_path;
    cv::VideoCapture capture;
    int frame_count = 0;
    double fps = 30.0;
    int last_index = -1;
};

VideoCapture::VideoCapture() : impl_(std::make_unique<Impl>()) {}

VideoCapture::~VideoCapture() = default;

VideoCapture::VideoCapture(VideoCapture&&) noexcept = default;

VideoCapture& VideoCapture::operator=(VideoCapture&&) noexcept = default;

bool is_video_path(const std::string& path) {
    const std::string extension = lowercase_extension(path);
    return extension == ".mp4" || extension == ".mov" || extension == ".avi" ||
           extension == ".mkv" || extension == ".webm";
}

void VideoCapture::open(const std::string& video_path) {
    impl_->video_path = video_path;
    impl_->capture.open(video_path);
    if (!impl_->capture.isOpened()) {
        throw std::runtime_error("Failed to open video: " + video_path);
    }

    impl_->frame_count = static_cast<int>(impl_->capture.get(cv::CAP_PROP_FRAME_COUNT));
    impl_->fps = impl_->capture.get(cv::CAP_PROP_FPS);
    if (impl_->fps <= 1.0) {
        impl_->fps = 30.0;
    }
    if (impl_->frame_count <= 0) {
        impl_->frame_count = 1;
    }
    impl_->last_index = -1;
}

SceneFrame VideoCapture::read_frame_at(int frame_index) {
    if (!impl_->capture.isOpened()) {
        throw std::runtime_error("Video not open: " + impl_->video_path);
    }

    const int clamped_index = std::max(0, std::min(frame_index, impl_->frame_count - 1));
    if (clamped_index != impl_->last_index) {
        impl_->capture.set(cv::CAP_PROP_POS_FRAMES, clamped_index);
        impl_->last_index = clamped_index;
    }

    cv::Mat frame_mat;
    if (!impl_->capture.read(frame_mat) || frame_mat.empty()) {
        throw std::runtime_error("Failed to read video frame: " + std::to_string(clamped_index));
    }
    impl_->last_index = clamped_index + 1;

    SceneFrame frame{};
    frame.frame_index = static_cast<std::uint64_t>(clamped_index);
    frame.timestamp_seconds = static_cast<double>(clamped_index) / impl_->fps;
    frame.source_path = impl_->video_path;
    frame.image = mat_to_image_buffer(frame_mat);
    return frame;
}

int VideoCapture::frame_count() const {
    return impl_->frame_count;
}

double VideoCapture::fps() const {
    return impl_->fps;
}

const std::string& VideoCapture::path() const {
    return impl_->video_path;
}

}  // namespace fovea
