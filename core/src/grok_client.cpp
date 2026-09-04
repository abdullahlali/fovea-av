#include "fovea/grok_client.hpp"

#include <curl/curl.h>

#include <cstdlib>
#include <sstream>
#include <stdexcept>

namespace fovea {

namespace {

std::string escape_json(const std::string& value) {
    std::string escaped;
    escaped.reserve(value.size() + 16);
    for (const char ch : value) {
        switch (ch) {
            case '"':
                escaped += "\\\"";
                break;
            case '\\':
                escaped += "\\\\";
                break;
            case '\n':
                escaped += "\\n";
                break;
            case '\r':
                escaped += "\\r";
                break;
            case '\t':
                escaped += "\\t";
                break;
            default:
                escaped += ch;
                break;
        }
    }
    return escaped;
}

size_t write_callback(char* contents, size_t size, size_t num_members, void* user_data) {
    auto* buffer = static_cast<std::string*>(user_data);
    const size_t total = size * num_members;
    buffer->append(contents, total);
    return total;
}

std::string extract_message_content(const std::string& response_body) {
    const std::string marker = "\"content\":";
    const auto marker_pos = response_body.find(marker);
    if (marker_pos == std::string::npos) {
        throw std::runtime_error("Grok response missing content field");
    }

    auto start = response_body.find('"', marker_pos + marker.size());
    if (start == std::string::npos) {
        throw std::runtime_error("Grok response malformed");
    }
    ++start;

    std::string content;
    bool escaping = false;
    for (size_t i = start; i < response_body.size(); ++i) {
        const char ch = response_body[i];
        if (escaping) {
            if (ch == 'n') {
                content += '\n';
            } else if (ch == 't') {
                content += '\t';
            } else if (ch == 'r') {
                content += '\r';
            } else {
                content += ch;
            }
            escaping = false;
            continue;
        }
        if (ch == '\\') {
            escaping = true;
            continue;
        }
        if (ch == '"') {
            return content;
        }
        content += ch;
    }

    throw std::runtime_error("Grok response content not terminated");
}

std::string build_payload(const GrokConfig& config, const GrokRequest& request) {
    std::ostringstream payload;
    payload << "{"
            << "\"model\":\"" << escape_json(config.model) << "\","
            << "\"messages\":["
            << "{"
            << "\"role\":\"system\","
            << "\"content\":\"" << escape_json(request.prompt) << "\""
            << "},"
            << "{"
            << "\"role\":\"user\","
            << "\"content\":\"Scene graph JSON:\\n" << escape_json(request.scene_json) << "\""
            << "}"
            << "],"
            << "\"temperature\":0.2,"
            << "\"max_tokens\":" << config.max_tokens
            << "}";
    return payload.str();
}

}  // namespace

GrokClient::GrokClient(GrokConfig config) : config_(std::move(config)) {}

GrokResponse GrokClient::narrate(const GrokRequest& request) const {
    GrokResponse response{};
    GrokConfig config = config_;

    if (const char* model_override = std::getenv("GROK_MODEL");
        model_override != nullptr && model_override[0] != '\0') {
        config.model = model_override;
    }

    const char* api_key = std::getenv(config.api_key_env.c_str());
    if (api_key == nullptr || api_key[0] == '\0') {
        response.ok = false;
        response.error =
            "XAI_API_KEY is not set. Export your key to enable Grok narration.";
        response.text =
            "[offline mode] Multiple road users detected ahead. Vehicle should "
            "maintain lane position, reduce speed, and be prepared to yield.";
        return response;
    }

    CURL* curl = curl_easy_init();
    if (curl == nullptr) {
        response.ok = false;
        response.error = "Failed to initialize HTTP client";
        return response;
    }

    const std::string url = config.base_url + "/chat/completions";
    const std::string payload = build_payload(config, request);
    std::string response_body;

    struct curl_slist* headers = nullptr;
    const std::string auth_header = std::string("Authorization: Bearer ") + api_key;
    headers = curl_slist_append(headers, auth_header.c_str());
    headers = curl_slist_append(headers, "Content-Type: application/json");

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, payload.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_body);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);

    const CURLcode result = curl_easy_perform(curl);
    long http_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    if (result != CURLE_OK) {
        response.ok = false;
        response.error = std::string("HTTP request failed: ") + curl_easy_strerror(result);
        return response;
    }

    if (http_code < 200 || http_code >= 300) {
        response.ok = false;
        response.error = "Grok API returned HTTP " + std::to_string(http_code);
        if (response_body.find("Incorrect API key") != std::string::npos) {
            response.error += " — check XAI_API_KEY in .env";
        }
        return response;
    }

    try {
        response.ok = true;
        response.text = extract_message_content(response_body);
    } catch (const std::exception& error) {
        response.ok = false;
        response.error = error.what();
    }

    return response;
}

}  // namespace fovea
