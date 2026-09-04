#include "panel_engine.hpp"

#if defined(FOVEA_HAS_VIDEO)

PanelEngine::PanelEngine(const std::string& forward_video,
                         const std::string& reverse_video,
                         const bool enable_grok,
                         const std::string& scenario_path,
                         const QString& gear,
                         QObject* parent)
    : QObject(parent) {
    options_.scenario_path = scenario_path;
    grok_enabled_ = enable_grok;

    forward_video_.open(forward_video);
    reverse_video_.open(reverse_video);
    forward_pipeline_.reset_tracking();
    reverse_pipeline_.reset_tracking();

    PanelBridge::instance().start();
    PanelBridge::instance().set_gear(gear);

    connect(&timer_, &QTimer::timeout, this, &PanelEngine::on_tick);
    timer_.setInterval(kTickMs);
    timer_.setSingleShot(true);
    grok_timer_.start();
    last_grok_ms_ = -kGrokCooldownMs;
}

void PanelEngine::start() {
    on_tick();
}

bool PanelEngine::should_refresh_grok() const {
    if (!grok_enabled_ || grok_auth_failed_) {
        return false;
    }
    return grok_timer_.elapsed() - last_grok_ms_ >= kGrokCooldownMs;
}

void PanelEngine::refresh_grok_narration(const fovea::SceneFrame& frame) {
    cached_grok_ = forward_pipeline_.narrate_scene(frame);
    last_grok_ms_ = grok_timer_.elapsed();

    if (!cached_grok_.error.empty()) {
        if (cached_grok_.error.find("HTTP 400") != std::string::npos ||
            cached_grok_.error.find("HTTP 401") != std::string::npos) {
            grok_auth_failed_ = true;
            cached_grok_.text.clear();
        }
        return;
    }

    grok_auth_failed_ = false;
}

void PanelEngine::process_tick() {
    if (busy_) {
        return;
    }
    busy_ = true;

    const int max_frames = std::min(forward_video_.frame_count(), reverse_video_.frame_count());
    if (max_frames <= 0) {
        busy_ = false;
        return;
    }

    if (frame_index_ >= max_frames) {
        frame_index_ = 0;
        forward_pipeline_.reset_tracking();
        reverse_pipeline_.reset_tracking();
    }

    if (alternate_forward_) {
        cached_forward_ = forward_pipeline_.process_video_frame(forward_video_, frame_index_, options_);
        has_forward_cache_ = true;
    } else {
        cached_reverse_ = reverse_pipeline_.process_video_frame(reverse_video_, frame_index_, options_);
        has_reverse_cache_ = true;
        frame_index_++;
    }
    alternate_forward_ = !alternate_forward_;

    if (has_forward_cache_ && has_reverse_cache_) {
        if (should_refresh_grok()) {
            refresh_grok_narration(cached_forward_.frame);
        }

        CabinSnapshot snapshot{};
        snapshot.forward = cached_forward_;
        snapshot.reverse = cached_reverse_;
        snapshot.grok = cached_grok_;
        if (grok_auth_failed_) {
            snapshot.grok.error.clear();
            snapshot.grok.text.clear();
        }
        PanelBridge::instance().publish_cabin(snapshot);
    }

    busy_ = false;
}

void PanelEngine::on_tick() {
    process_tick();
    timer_.start();
}

#endif
