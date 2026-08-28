#include "fovea/pipeline.hpp"
#include "fovea/scene_graph.hpp"

#include <iostream>
#include <stdexcept>
#include <string>

namespace {

void print_usage() {
    std::cout << "Fovea — Interactive Autonomy Visualization\n\n";
    std::cout << "Usage:\n";
    std::cout << "  fovea_cli <image_path> [--grok]\n\n";
    std::cout << "Options:\n";
    std::cout << "  --grok    Request Grok narration (requires XAI_API_KEY)\n";
}

}  // namespace

int main(int argc, char* argv[]) {
    if (argc < 2) {
        print_usage();
        return 1;
    }

    const std::string image_path = argv[1];
    bool enable_grok = false;

    for (int i = 2; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--grok") {
            enable_grok = true;
        }
    }

    try {
        fovea::Pipeline pipeline;
        fovea::PipelineOptions options{};
        options.enable_grok = enable_grok;
        options.print_json = true;

        const auto result = pipeline.process_image(image_path, options);
        fovea::SceneGraph scene_graph;

        std::cout << scene_graph.to_json(result.frame);
        std::cout << "\n--- metrics ---\n";
        std::cout << "capture_ms: " << result.metrics.capture_ms << "\n";
        std::cout << "infer_ms: " << result.metrics.infer_ms << "\n";
        std::cout << "predict_ms: " << result.metrics.predict_ms << "\n";
        std::cout << "total_ms: " << result.metrics.total_ms << "\n";

        if (enable_grok) {
            std::cout << "\n--- grok ---\n";
            if (!result.grok.error.empty()) {
                std::cout << "warning: " << result.grok.error << "\n";
            }
            std::cout << result.grok.text << "\n";
        }
    } catch (const std::exception& error) {
        std::cerr << "error: " << error.what() << "\n";
        return 1;
    }

    return 0;
}
