#include "scene_window.hpp"

#include "fovea/pipeline.hpp"

#include <QApplication>

#include <iostream>
#include <string>

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);

    const std::string image_path =
        argc > 1 ? argv[1] : "assets/test/bdd/street.jpg";

    try {
        fovea::Pipeline pipeline;
        const auto result = pipeline.process_image(image_path);

        SceneWindow window(result);
        window.show();

        return app.exec();
    } catch (const std::exception& error) {
        std::cerr << "error: " << error.what() << '\n';
        return 1;
    }
}
