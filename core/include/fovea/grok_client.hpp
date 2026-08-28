#pragma once

#include <string>

namespace fovea {

struct GrokConfig {
    std::string api_key_env = "XAI_API_KEY";
    std::string model = "grok-4.6";
    std::string base_url = "https://api.x.ai/v1";
};

struct GrokRequest {
    std::string scene_json;
    std::string prompt =
        "You are an in-vehicle autonomy copilot. Given the scene graph JSON, "
        "explain in 2-3 sentences what the vehicle should do. Use a calm, "
        "passenger-facing tone.";
};

struct GrokResponse {
    bool ok = false;
    std::string text;
    std::string error;
};

class GrokClient {
public:
    explicit GrokClient(GrokConfig config = {});

    [[nodiscard]] GrokResponse narrate(const GrokRequest& request) const;

private:
    GrokConfig config_;
};

}  // namespace fovea
