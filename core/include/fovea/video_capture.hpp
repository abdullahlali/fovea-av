#pragma once

#include "fovea/types.hpp"

#include <memory>
#include <string>

namespace fovea {

class VideoCapture {
public:
    VideoCapture();
    ~VideoCapture();

    VideoCapture(const VideoCapture&) = delete;
    VideoCapture& operator=(const VideoCapture&) = delete;
    VideoCapture(VideoCapture&&) noexcept;
    VideoCapture& operator=(VideoCapture&&) noexcept;

    void open(const std::string& video_path);
    [[nodiscard]] SceneFrame read_frame_at(int frame_index);
    [[nodiscard]] int frame_count() const;
    [[nodiscard]] double fps() const;
    [[nodiscard]] const std::string& path() const;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

bool is_video_path(const std::string& path);

}  // namespace fovea
