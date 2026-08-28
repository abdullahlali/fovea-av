#include "fovea/grok_client.hpp"

#include <cstdlib>

namespace fovea {

GrokClient::GrokClient(GrokConfig config) : config_(std::move(config)) {}

GrokResponse GrokClient::narrate(const GrokRequest& request) const {
    GrokResponse response{};

    const char* api_key = std::getenv(config_.api_key_env.c_str());
    if (api_key == nullptr || api_key[0] == '\0') {
        response.ok = false;
        response.error =
            "XAI_API_KEY is not set. Export your key to enable Grok narration.";
        response.text =
            "[offline mode] Pedestrian ahead in travel lane. Vehicle should "
            "reduce speed and prepare to yield while maintaining lane position.";
        return response;
    }

    (void)request;
    response.ok = true;
    response.text =
        "Grok narration is scaffolded. Next step: wire HTTP client to " +
        config_.base_url + " using model " + config_.model + ".";
    return response;
}

}  // namespace fovea
