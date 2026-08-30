#include "fovea/scene_graph.hpp"

#include <sstream>

namespace fovea {

namespace {

std::string escape_json(const std::string& value) {
    std::string escaped;
    escaped.reserve(value.size());
    for (const char ch : value) {
        if (ch == '"') {
            escaped += "\\\"";
        } else if (ch == '\\') {
            escaped += "\\\\";
        } else {
            escaped += ch;
        }
    }
    return escaped;
}

}  // namespace

void SceneGraph::update(SceneFrame& frame) const {
    for (std::size_t i = 0; i < frame.detections.size(); ++i) {
        if (frame.detections[i].id == 0) {
            frame.detections[i].id = static_cast<std::uint32_t>(i + 1);
        }
    }
}

std::string SceneGraph::to_json(const SceneFrame& frame) const {
    std::ostringstream json;
    json << "{\n";
    json << "  \"source\": \"" << escape_json(frame.source_path) << "\",\n";
    json << "  \"image\": {\"width\": " << frame.image.width
         << ", \"height\": " << frame.image.height << "},\n";
    json << "  \"frame_index\": " << frame.frame_index << ",\n";
    json << "  \"detections\": [\n";

    for (std::size_t i = 0; i < frame.detections.size(); ++i) {
        const auto& detection = frame.detections[i];
        json << "    {\n";
        json << "      \"id\": " << detection.id << ",\n";
        json << "      \"label\": \"" << escape_json(detection.label) << "\",\n";
        json << "      \"confidence\": " << detection.confidence << ",\n";
        json << "      \"bbox\": {"
             << "\"x\": " << detection.bbox.x << ", "
             << "\"y\": " << detection.bbox.y << ", "
             << "\"width\": " << detection.bbox.width << ", "
             << "\"height\": " << detection.bbox.height << "},\n";
        json << "      \"velocity\": {"
             << "\"dx\": " << detection.velocity.dx << ", "
             << "\"dy\": " << detection.velocity.dy << "}\n";
        json << "    }";
        if (i + 1 < frame.detections.size()) {
            json << ",";
        }
        json << "\n";
    }

    json << "  ],\n";
    json << "  \"predictions\": [\n";

    for (std::size_t i = 0; i < frame.predictions.size(); ++i) {
        const auto& prediction = frame.predictions[i];
        json << "    {\n";
        json << "      \"detection_id\": " << prediction.detection_id << ",\n";
        json << "      \"horizon_seconds\": " << prediction.horizon_seconds << ",\n";
        json << "      \"bbox\": {"
             << "\"x\": " << prediction.bbox.x << ", "
             << "\"y\": " << prediction.bbox.y << ", "
             << "\"width\": " << prediction.bbox.width << ", "
             << "\"height\": " << prediction.bbox.height << "}\n";
        json << "    }";
        if (i + 1 < frame.predictions.size()) {
            json << ",";
        }
        json << "\n";
    }

    json << "  ]\n";
    json << "}\n";
    return json.str();
}

}  // namespace fovea
