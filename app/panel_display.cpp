#include "panel_display.hpp"

#include "panel_bridge.hpp"
#include "panel_static_server.hpp"

#if defined(FOVEA_HAS_VIDEO)

#if defined(FOVEA_HAS_WEBENGINE)
#include <QWebEngineSettings>
#endif

#include <QVBoxLayout>

PanelDisplay::PanelDisplay(const std::string& forward_video,
                           const std::string& reverse_video,
                           const bool enable_grok,
                           const std::string& scenario_path,
                           const QString& gear,
                           QWidget* parent)
    : QMainWindow(parent) {
    setWindowTitle(QStringLiteral("Fovea — Driver Display"));
    resize(1280, 800);

    options_.enable_grok = false;
    options_.scenario_path = scenario_path;
    grok_enabled_ = enable_grok;

    forward_video_.open(forward_video);
    reverse_video_.open(reverse_video);
    forward_pipeline_.reset_tracking();
    reverse_pipeline_.reset_tracking();

    PanelBridge::instance().start();
    PanelBridge::instance().set_gear(gear);
    connect(&PanelBridge::instance(), &PanelBridge::gear_changed, this, &PanelDisplay::on_gear_changed);

    auto* central = new QWidget(this);
    auto* layout = new QVBoxLayout(central);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

#if defined(FOVEA_HAS_WEBENGINE)
    panel_view_ = new QWebEngineView(central);
    panel_view_->setStyleSheet(QStringLiteral("background: #08080a;"));

    auto* settings = panel_view_->settings();
    settings->setAttribute(QWebEngineSettings::LocalContentCanAccessRemoteUrls, true);
    settings->setAttribute(QWebEngineSettings::JavascriptEnabled, true);

    const QString dist_dir = resolve_panel_dist_directory();
    if (!dist_dir.isEmpty() && PanelStaticServer::instance().start(dist_dir)) {
        panel_view_->load(QUrl(PanelStaticServer::instance().base_url()));
    } else {
        panel_view_->setHtml(QStringLiteral(
            "<body style='background:#08080a;color:#eee;font-family:sans-serif;padding:32px'>"
            "<h2>Panel not built</h2><p>Run: cd panel && npm run build</p></body>"));
    }
    layout->addWidget(panel_view_);
#else
    layout->addWidget(new QLabel(QStringLiteral("Install Qt WebEngine for the driver display."), central));
#endif

    setCentralWidget(central);

    connect(&timer_, &QTimer::timeout, this, &PanelDisplay::on_tick);
    timer_.setInterval(kTickMs);
    timer_.setSingleShot(true);

    process_and_publish();
    timer_.start();
}

bool PanelDisplay::should_refresh_grok() const {
    if (!grok_enabled_ || grok_auth_failed_) {
        return false;
    }
    return ticks_since_grok_ >= kGrokEveryTicks || ticks_since_grok_ == 0;
}

void PanelDisplay::refresh_grok_narration(const fovea::SceneFrame& frame) {
    cached_grok_ = forward_pipeline_.narrate_scene(frame);
    ticks_since_grok_ = 0;

    if (!cached_grok_.error.empty()) {
        if (cached_grok_.error.find("HTTP 400") != std::string::npos ||
            cached_grok_.error.find("HTTP 401") != std::string::npos) {
            grok_auth_failed_ = true;
            cached_grok_.text.clear();
        }
    }
}

void PanelDisplay::process_and_publish() {
    if (busy_) {
        return;
    }
    busy_ = true;

    const int max_frames = std::min(forward_video_.frame_count(), reverse_video_.frame_count());
    if (max_frames <= 0) {
        busy_ = false;
        return;
    }

    if (alternate_forward_) {
        cached_forward_ = forward_pipeline_.process_video_frame(forward_video_, current_frame_, options_);
        has_forward_cache_ = true;
    } else {
        cached_reverse_ = reverse_pipeline_.process_video_frame(reverse_video_, current_frame_, options_);
        has_reverse_cache_ = true;
    }
    alternate_forward_ = !alternate_forward_;

    fovea::PipelineResult forward_result = has_forward_cache_ ? cached_forward_ : fovea::PipelineResult{};
    fovea::PipelineResult reverse_result = has_reverse_cache_ ? cached_reverse_ : fovea::PipelineResult{};

    ++ticks_since_grok_;
    if (should_refresh_grok() && has_forward_cache_) {
        refresh_grok_narration(forward_result.frame);
    }

    CabinSnapshot snapshot{};
    snapshot.forward = std::move(forward_result);
    snapshot.reverse = std::move(reverse_result);
    snapshot.grok = cached_grok_;
    if (grok_auth_failed_) {
        snapshot.grok.error.clear();
    }

    if (has_forward_cache_) {
        cached_forward_ = snapshot.forward;
    }
    if (has_reverse_cache_) {
        cached_reverse_ = snapshot.reverse;
    }

    PanelBridge::instance().publish_cabin(snapshot);

    current_frame_ = (current_frame_ + 1) % max_frames;
    busy_ = false;
}

void PanelDisplay::on_tick() {
    if (!busy_) {
        process_and_publish();
    }
    timer_.start();
}

void PanelDisplay::on_gear_changed(const QString& gear) {
    setWindowTitle(QStringLiteral("Fovea — Driver Display · Gear %1").arg(gear));
}

#endif
