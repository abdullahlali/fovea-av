#pragma once

#include "fovea/types.hpp"

#include <string>

namespace fovea {

class VideoCapture {
public:
    void open(const std::string& video_path);
    [[nodiscard]] SceneFrame read_frame_at(int frame_index) const;
    [[nodiscard]] int frame_count() const;
    [[nodiscard]] double fps() const;
    [[nodiscard]] const std::string& path() const;

private:
    std::string video_path_;
    int frame_count_ = 0;
    double fps_ = 30.0;
};

bool is_video_path(const std::string& path);

}  // namespace fovea
