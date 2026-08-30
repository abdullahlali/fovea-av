#include "scene_window.hpp"
#include "video_window.hpp"

#include "fovea/pipeline.hpp"
#include "panel_bridge.hpp"

#if defined(FOVEA_HAS_VIDEO)
#include "fovea/video_capture.hpp"
#endif

#include <QApplication>

#include <iostream>
#include <string>

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);

    std::string media_path = "assets/test/bdd/street.jpg";
    bool enable_grok = false;
    bool enable_panel = false;

    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--grok") {
            enable_grok = true;
        } else if (arg == "--panel") {
            enable_panel = true;
        } else if (arg.rfind('-', 0) != 0) {
            media_path = arg;
        }
    }

    if (enable_panel) {
        PanelBridge::instance().start();
    }

    try {
#if defined(FOVEA_HAS_VIDEO)
        if (fovea::is_video_path(media_path)) {
            VideoWindow window(media_path, enable_grok, enable_panel);
            window.show();
            return app.exec();
        }
#endif

        fovea::Pipeline pipeline;
        fovea::PipelineOptions options{};
        options.enable_grok = enable_grok;
        const auto result = pipeline.process_image(media_path, options);

        SceneWindow window(result, enable_panel);
        window.show();
        return app.exec();
    } catch (const std::exception& error) {
        std::cerr << "error: " << error.what() << '\n';
        return 1;
    }
}
