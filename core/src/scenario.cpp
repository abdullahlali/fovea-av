#include "fovea/scenario.hpp"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>
#include <stdexcept>

namespace fovea {

namespace {

std::string read_file(const std::string& path) {
    std::ifstream input(path);
    if (!input) {
        throw std::runtime_error("Could not open scenario file: " + path);
    }
    std::ostringstream buffer;
    buffer << input.rdbuf();
    return buffer.str();
}

std::string extract_string(const std::string& json, const std::string& key) {
    const std::string needle = "\"" + key + "\"";
    const auto key_pos = json.find(needle);
    if (key_pos == std::string::npos) {
        return {};
    }

    const auto colon = json.find(':', key_pos + needle.size());
    const auto quote_start = json.find('"', colon + 1);
    const auto quote_end = json.find('"', quote_start + 1);
    if (quote_start == std::string::npos || quote_end == std::string::npos) {
        return {};
    }
    return json.substr(quote_start + 1, quote_end - quote_start - 1);
}

float extract_number_after(const std::string& json, const std::string& key) {
    const std::string needle = "\"" + key + "\"";
    const auto key_pos = json.find(needle);
    if (key_pos == std::string::npos) {
        return 0.0F;
    }
    const auto colon = json.find(':', key_pos + needle.size());
    if (colon == std::string::npos) {
        return 0.0F;
    }

    std::size_t index = colon + 1;
    while (index < json.size() && std::isspace(static_cast<unsigned char>(json[index]))) {
        ++index;
    }

    std::size_t end = index;
    while (end < json.size() &&
           (std::isdigit(static_cast<unsigned char>(json[end])) || json[end] == '.' ||
            json[end] == '-')) {
        ++end;
    }
    if (end == index) {
        return 0.0F;
    }
    return std::stof(json.substr(index, end - index));
}

int extract_int_after(const std::string& json, const std::string& key, int default_value) {
    const std::string needle = "\"" + key + "\"";
    if (json.find(needle) == std::string::npos) {
        return default_value;
    }
    return static_cast<int>(extract_number_after(json, key));
}

std::vector<ScenarioHazard> parse_hazards(const std::string& json) {
    std::vector<ScenarioHazard> hazards;
    const std::string hazards_key = "\"hazards\"";
    const auto hazards_pos = json.find(hazards_key);
    if (hazards_pos == std::string::npos) {
        return hazards;
    }

    const auto array_start = json.find('[', hazards_pos);
    const auto array_end = json.find(']', array_start);
    if (array_start == std::string::npos || array_end == std::string::npos) {
        return hazards;
    }

    std::string cursor = json.substr(array_start, array_end - array_start);
    std::size_t search_from = 0;
    while (true) {
        const auto object_start = cursor.find('{', search_from);
        if (object_start == std::string::npos) {
            break;
        }
        const auto object_end = cursor.find('}', object_start);
        if (object_end == std::string::npos) {
            break;
        }

        const std::string object = cursor.substr(object_start, object_end - object_start + 1);
        ScenarioHazard hazard{};
        hazard.label = extract_string(object, "label");
        hazard.confidence = extract_number_after(object, "confidence");
        hazard.trigger_frame = extract_int_after(object, "trigger_frame", -1);

        const auto bbox_pos = object.find("\"bbox\"");
        if (bbox_pos != std::string::npos) {
            const auto bbox_object_start = object.find('{', bbox_pos);
            const auto bbox_object_end = object.find('}', bbox_object_start);
            if (bbox_object_start != std::string::npos && bbox_object_end != std::string::npos) {
                const std::string bbox_object =
                    object.substr(bbox_object_start, bbox_object_end - bbox_object_start + 1);
                hazard.bbox.x = extract_number_after(bbox_object, "x");
                hazard.bbox.y = extract_number_after(bbox_object, "y");
                hazard.bbox.width = extract_number_after(bbox_object, "width");
                hazard.bbox.height = extract_number_after(bbox_object, "height");
            }
        } else {
            hazard.bbox.x = extract_number_after(object, "x");
            hazard.bbox.y = extract_number_after(object, "y");
            hazard.bbox.width = extract_number_after(object, "width");
            hazard.bbox.height = extract_number_after(object, "height");
        }

        if (!hazard.label.empty()) {
            hazards.push_back(hazard);
        }

        search_from = object_end + 1;
    }

    return hazards;
}

}  // namespace

Scenario load_scenario(const std::string& path) {
    const std::string json = read_file(path);
    Scenario scenario{};
    scenario.name = extract_string(json, "name");
    scenario.description = extract_string(json, "description");
    scenario.hazards = parse_hazards(json);

    if (scenario.hazards.empty()) {
        throw std::runtime_error("Scenario has no hazards: " + path);
    }

    return scenario;
}

void apply_scenario(SceneFrame& frame, const Scenario& scenario) {
    std::uint32_t next_id = 9000;
    for (const auto& existing : frame.detections) {
        next_id = std::max(next_id, existing.id + 1);
    }

    const int frame_index = static_cast<int>(frame.frame_index);
    for (const ScenarioHazard& hazard : scenario.hazards) {
        if (hazard.trigger_frame >= 0 && hazard.trigger_frame != frame_index) {
            continue;
        }

        Detection injected{};
        injected.id = next_id++;
        injected.label = hazard.label;
        injected.confidence = hazard.confidence;
        injected.bbox = hazard.bbox;
        frame.detections.push_back(injected);
    }
}

}  // namespace fovea
