#include "fovea/video_capture.hpp"

#include <opencv2/opencv.hpp>

#include <algorithm>
#include <cctype>
#include <stdexcept>

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

bool is_video_path(const std::string& path) {
    const std::string extension = lowercase_extension(path);
    return extension == ".mp4" || extension == ".mov" || extension == ".avi" ||
           extension == ".mkv" || extension == ".webm";
}

void VideoCapture::open(const std::string& video_path) {
    video_path_ = video_path;
    cv::VideoCapture probe(video_path);
    if (!probe.isOpened()) {
        throw std::runtime_error("Failed to open video: " + video_path);
    }

    frame_count_ = static_cast<int>(probe.get(cv::CAP_PROP_FRAME_COUNT));
    fps_ = probe.get(cv::CAP_PROP_FPS);
    if (fps_ <= 1.0) {
        fps_ = 30.0;
    }
    if (frame_count_ <= 0) {
        frame_count_ = 1;
    }
}

SceneFrame VideoCapture::read_frame_at(int frame_index) const {
    cv::VideoCapture capture(video_path_);
    if (!capture.isOpened()) {
        throw std::runtime_error("Failed to open video: " + video_path_);
    }

    const int clamped_index = std::max(0, std::min(frame_index, frame_count_ - 1));
    capture.set(cv::CAP_PROP_POS_FRAMES, clamped_index);

    cv::Mat frame_mat;
    if (!capture.read(frame_mat) || frame_mat.empty()) {
        throw std::runtime_error("Failed to read video frame: " + std::to_string(clamped_index));
    }

    SceneFrame frame{};
    frame.frame_index = static_cast<std::uint64_t>(clamped_index);
    frame.timestamp_seconds = static_cast<double>(clamped_index) / fps_;
    frame.source_path = video_path_;
    frame.image = mat_to_image_buffer(frame_mat);
    return frame;
}

int VideoCapture::frame_count() const {
    return frame_count_;
}

double VideoCapture::fps() const {
    return fps_;
}

const std::string& VideoCapture::path() const {
    return video_path_;
}

}  // namespace fovea
