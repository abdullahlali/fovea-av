#include "cabin_window.hpp"
#include "panel_engine.hpp"
#include "panel_window.hpp"
#include "scene_window.hpp"
#include "video_window.hpp"

#include "fovea/camera_view.hpp"
#include "fovea/env.hpp"
#include "fovea/pipeline.hpp"
#include "panel_bridge.hpp"

#if defined(FOVEA_HAS_VIDEO)
#include "fovea/video_capture.hpp"
#endif

#include <QApplication>

#include <iostream>
#include <string>

namespace {

bool ends_with(const std::string& value, const std::string& suffix) {
    return value.size() >= suffix.size() &&
           value.compare(value.size() - suffix.size(), suffix.size(), suffix) == 0;
}

bool looks_like_video(const std::string& path) {
    return ends_with(path, ".mp4") || ends_with(path, ".mov") || ends_with(path, ".avi") ||
           ends_with(path, ".mkv") || ends_with(path, ".webm");
}

void print_usage() {
    std::cout << "Usage: fovea_app [options]\n\n";
    std::cout << "Options:\n";
    std::cout << "  --grok              Enable Grok passenger narration\n";
    std::cout << "  --gear <P|R|N|D>    Simulated gear (default: D)\n";
    std::cout << "  --forward <path>    Forward camera video (default: street.mp4)\n";
    std::cout << "  --reverse <path>    Reverse camera video (default: reverse.mp4)\n";
    std::cout << "  --scenario <path>   Inject synthetic hazards from JSON\n";
    std::cout << "  --cabin             Legacy dual-camera + panel window\n";
    std::cout << "  --driver-only       Single-camera driver window (legacy)\n";
    std::cout << "  --camera <view>     forward | reverse (driver-only mode)\n";
    std::cout << "  --image <path>      Process a still image instead of video\n";
}

}  // namespace

int main(int argc, char* argv[]) {
    fovea::load_project_dotenv();

    QApplication app(argc, argv);

    bool enable_grok = false;
    bool driver_only = false;
    bool cabin_mode = false;
    std::string scenario_path;
    std::string gear = "D";
    std::string forward_video = fovea::default_forward_video();
    std::string reverse_video = fovea::default_reverse_video();
    std::string image_path;
    fovea::CameraView driver_camera = fovea::CameraView::Forward;

    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--grok") {
            enable_grok = true;
        } else if (arg == "--driver-only") {
            driver_only = true;
        } else if (arg == "--cabin") {
            cabin_mode = true;
        } else if (arg == "--gear" && i + 1 < argc) {
            gear = argv[++i];
        } else if (arg == "--forward" && i + 1 < argc) {
            forward_video = argv[++i];
        } else if (arg == "--reverse" && i + 1 < argc) {
            reverse_video = argv[++i];
        } else if (arg == "--scenario" && i + 1 < argc) {
            scenario_path = argv[++i];
        } else if (arg == "--camera" && i + 1 < argc) {
            driver_camera = fovea::parse_camera_view(argv[++i]);
        } else if (arg == "--image" && i + 1 < argc) {
            image_path = argv[++i];
        } else if (arg == "--help" || arg == "-h") {
            print_usage();
            return 0;
        } else if (arg.rfind('-', 0) != 0) {
            if (looks_like_video(arg)) {
                forward_video = arg;
            } else {
                image_path = arg;
            }
        } else {
            std::cerr << "Unknown option: " << arg << '\n';
            print_usage();
            return 1;
        }
    }

    try {
        if (!image_path.empty()) {
            fovea::Pipeline pipeline;
            fovea::PipelineOptions options{};
            options.enable_grok = enable_grok;
            options.scenario_path = scenario_path;
            const auto result = pipeline.process_image(image_path, options);

            PanelBridge::instance().start();
            PanelBridge::instance().set_camera_profile("forward", "Forward Camera");
            SceneWindow window(result, true, fovea::camera_profile(fovea::CameraView::Forward));
            window.show();
            return app.exec();
        }

#if defined(FOVEA_HAS_VIDEO)
        if (driver_only) {
            const fovea::CameraProfile active_camera = fovea::camera_profile(driver_camera);
            const std::string media_path = active_camera.default_video == fovea::default_reverse_video()
                                               ? reverse_video
                                               : forward_video;

            PanelBridge::instance().start();
            PanelBridge::instance().set_camera_profile(active_camera.mode_id, active_camera.label);
            VideoWindow window(media_path, enable_grok, true, scenario_path, active_camera);
            window.show();
            return app.exec();
        }

        if (cabin_mode) {
            CabinWindow window(forward_video, reverse_video, enable_grok, scenario_path,
                               QString::fromStdString(gear));
            window.show();
            return app.exec();
        }

#if defined(FOVEA_HAS_WEBENGINE)
        PanelWindow panel;
        panel.show();

        PanelEngine engine(forward_video, reverse_video, enable_grok, scenario_path,
                           QString::fromStdString(gear));
        engine.start();
        return app.exec();
#else
        PanelEngine engine(forward_video, reverse_video, enable_grok, scenario_path,
                           QString::fromStdString(gear));
        engine.start();
        std::cerr << "Panel UI requires Qt WebEngine. Perception engine is running on ws://127.0.0.1:8765\n";
        return app.exec();
#endif
#else
        std::cerr << "Video support not built. Rebuild with -DFOVEA_ENABLE_VIDEO=ON\n";
        return 1;
#endif
    } catch (const std::exception& error) {
        std::cerr << "error: " << error.what() << '\n';
        return 1;
    }
}
