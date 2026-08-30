#include "scene_window.hpp"

#include "fovea/pipeline.hpp"

#include <QApplication>

#include <iostream>
#include <string>

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);

    std::string image_path = "assets/test/bdd/street.jpg";
    bool enable_grok = false;

    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--grok") {
            enable_grok = true;
        } else if (arg.rfind('-', 0) != 0) {
            image_path = arg;
        }
    }

    try {
        fovea::Pipeline pipeline;
        fovea::PipelineOptions options{};
        options.enable_grok = enable_grok;
        const auto result = pipeline.process_image(image_path, options);

        SceneWindow window(result);
        window.show();

        return app.exec();
    } catch (const std::exception& error) {
        std::cerr << "error: " << error.what() << '\n';
        return 1;
    }
}
