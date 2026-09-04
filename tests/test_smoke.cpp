#include "fovea/camera_view.hpp"
#include "fovea/scenario.hpp"

#include <cassert>
#include <cstdlib>
#include <iostream>

int main() {
    using namespace fovea;

    const auto forward = camera_profile(CameraView::Forward);
    assert(forward.mode_id == "forward");
    assert(forward.default_video.find("street.mp4") != std::string::npos);

    const auto reverse = camera_profile(CameraView::Reverse);
    assert(reverse.mode_id == "reverse");

    assert(parse_camera_view("forward") == CameraView::Forward);
    assert(parse_camera_view("r") == CameraView::Reverse);

    SceneFrame frame{};
    frame.frame_index = 60;
    Scenario scenario{};
    scenario.hazards.push_back(
        ScenarioHazard{.label = "person", .confidence = 0.9F, .trigger_frame = 60});
    scenario.hazards.push_back(
        ScenarioHazard{.label = "car", .confidence = 0.8F, .trigger_frame = 10});

    apply_scenario(frame, scenario);
    assert(frame.detections.size() == 1);
    assert(frame.detections[0].label == "person");

    frame.frame_index = 10;
    frame.detections.clear();
    apply_scenario(frame, scenario);
    assert(frame.detections.size() == 1);
    assert(frame.detections[0].label == "car");

    std::cout << "fovea_tests: ok\n";
    return EXIT_SUCCESS;
}
