#include "cabin_window.hpp"

#include "panel_bridge.hpp"
#include "panel_static_server.hpp"

#if defined(FOVEA_HAS_VIDEO)

#include <QFrame>
#include <QGroupBox>
#include <QStatusBar>

#if defined(FOVEA_HAS_WEBENGINE)
#include <QWebEngineSettings>
#endif

#include <cmath>

namespace {

QWidget* make_driver_feed(const QString& title, SceneCanvas*& canvas_out, QLabel*& metrics_out) {
    auto* box = new QGroupBox(title);
    auto* layout = new QVBoxLayout(box);
    layout->setContentsMargins(8, 12, 8, 8);
    layout->setSpacing(6);

    canvas_out = new SceneCanvas(box);
    canvas_out->setMinimumHeight(180);
    metrics_out = new QLabel(box);
    metrics_out->setStyleSheet(
        "color: #9ecfff; padding: 4px 2px; font-family: Menlo, monospace; font-size: 11px;");

    layout->addWidget(canvas_out, 1);
    layout->addWidget(metrics_out);
    return box;
}

}  // namespace

CabinWindow::CabinWindow(const std::string& forward_video,
                         const std::string& reverse_video,
                         const bool enable_grok,
                         const std::string& scenario_path,
                         const QString& gear,
                         QWidget* parent)
    : QMainWindow(parent) {
    setWindowTitle(QStringLiteral("Fovea — Cabin Display"));
    resize(1600, 900);

    options_.enable_grok = enable_grok;
    options_.scenario_path = scenario_path;
    grok_enabled_ = enable_grok;

    forward_video_.open(forward_video);
    reverse_video_.open(reverse_video);
    forward_pipeline_.reset_tracking();
    reverse_pipeline_.reset_tracking();

    PanelBridge::instance().start();
    PanelBridge::instance().set_gear(gear);
    connect(&PanelBridge::instance(), &PanelBridge::gear_changed, this, &CabinWindow::on_gear_changed);

    auto* splitter = new QSplitter(Qt::Horizontal, this);

    auto* driver = new QWidget(splitter);
    auto* driver_layout = new QVBoxLayout(driver);
    driver_layout->setContentsMargins(10, 10, 6, 10);
    driver_layout->setSpacing(8);

    auto* forward_box = make_driver_feed(QStringLiteral("Forward Camera"), forward_canvas_, forward_metrics_);
    auto* reverse_box = make_driver_feed(QStringLiteral("Reverse Camera"), reverse_canvas_, reverse_metrics_);
    driver_layout->addWidget(forward_box, 3);
    driver_layout->addWidget(reverse_box, 2);

    slider_ = new QSlider(Qt::Horizontal, driver);
    slider_->setRange(0, std::max(0, forward_video_.frame_count() - 1));

    auto* controls = new QHBoxLayout();
    play_button_ = new QPushButton(QStringLiteral("Play"), driver);
    controls->addWidget(play_button_);
    controls->addWidget(slider_, 1);

    grok_label_ = new QLabel(driver);
    grok_label_->setWordWrap(true);
    grok_label_->setVisible(enable_grok);
    grok_label_->setStyleSheet(
        "color: #f0f0f0; background: #1a1a1f; padding: 10px; border-radius: 8px; "
        "border: 1px solid #3a3a44; font-size: 13px;");

    driver_layout->addLayout(controls);
    driver_layout->addWidget(grok_label_);

#if defined(FOVEA_HAS_WEBENGINE)
    panel_view_ = new QWebEngineView(splitter);
    panel_view_->setMinimumWidth(420);
    panel_view_->setStyleSheet(QStringLiteral("background: #0d0f14;"));

    auto* settings = panel_view_->settings();
    settings->setAttribute(QWebEngineSettings::LocalContentCanAccessFileUrls, true);
    settings->setAttribute(QWebEngineSettings::LocalContentCanAccessRemoteUrls, true);
    settings->setAttribute(QWebEngineSettings::JavascriptEnabled, true);

    const QString dist_dir = resolve_panel_dist_directory();
    if (!dist_dir.isEmpty() && PanelStaticServer::instance().start(dist_dir)) {
        const QUrl panel_url(PanelStaticServer::instance().base_url());
        panel_view_->load(panel_url);
        qInfo("CabinWindow: passenger panel -> %s", qPrintable(panel_url.toString()));
    } else {
        panel_view_->setHtml(QStringLiteral(
            "<body style='background:#0d0f14;color:#eee;font-family:sans-serif;padding:24px'>"
            "<h2>Passenger panel not built</h2>"
            "<p>Run <code>cd panel && npm run build</code> from the project root.</p>"
            "</body>"));
    }
#else
    auto* panel_placeholder = new QLabel(
        QStringLiteral("Passenger panel requires Qt WebEngine.\n"
                       "Install Qt WebEngine or run: cd panel && npm run dev"),
        splitter);
    panel_placeholder->setAlignment(Qt::AlignCenter);
    panel_placeholder->setStyleSheet("color: #ccc; padding: 24px;");
#endif

    splitter->addWidget(driver);
#if defined(FOVEA_HAS_WEBENGINE)
    splitter->addWidget(panel_view_);
#else
    splitter->addWidget(panel_placeholder);
#endif
    splitter->setStretchFactor(0, 3);
    splitter->setStretchFactor(1, 2);
    splitter->setSizes({960, 640});

    setCentralWidget(splitter);

    connect(play_button_, &QPushButton::clicked, this, &CabinWindow::on_play_pause);
    connect(slider_, &QSlider::valueChanged, this, &CabinWindow::on_slider_changed);
    connect(&timer_, &QTimer::timeout, this, &CabinWindow::on_tick);

    const int interval_ms = std::max(1, static_cast<int>(1000.0 / forward_video_.fps()));
    timer_.setInterval(interval_ms);
    timer_.setSingleShot(true);

    publish_timer_.start();
    last_publish_ms_ = publish_timer_.elapsed();

    statusBar()->showMessage(panel_status_hint(true));
    show_frame(0);
}

bool CabinWindow::should_refresh_grok(const int frame_index) const {
    if (!grok_enabled_ || grok_auth_failed_) {
        return false;
    }

    const int grok_interval_frames =
        std::max(1, static_cast<int>(std::lround(forward_video_.fps() * kGrokCooldownSeconds)));
    return last_grok_frame_ < 0 ||
           std::abs(frame_index - last_grok_frame_) >= grok_interval_frames;
}

void CabinWindow::refresh_grok_narration(const fovea::SceneFrame& frame) {
    cached_grok_ = forward_pipeline_.narrate_scene(frame);
    last_grok_frame_ = current_frame_;

    if (!cached_grok_.error.empty()) {
        if (cached_grok_.error.find("HTTP 400") != std::string::npos ||
            cached_grok_.error.find("HTTP 401") != std::string::npos) {
            grok_auth_failed_ = true;
            cached_grok_.text =
                "[offline mode] Grok unavailable — using local passenger briefing.";
        }
    }
}

void CabinWindow::publish_panel_update(const CabinSnapshot& snapshot) {
    const qint64 now_ms = publish_timer_.elapsed();
    if (playing_ && now_ms - last_publish_ms_ < kPublishIntervalMs) {
        return;
    }
    last_publish_ms_ = now_ms;
    PanelBridge::instance().publish_cabin(snapshot);
}

fovea::PipelineResult CabinWindow::process_view(fovea::Pipeline& pipeline,
                                                fovea::VideoCapture& video,
                                                const int frame_index,
                                                const bool enable_grok_for_frame) {
    fovea::PipelineOptions frame_options = options_;
    frame_options.enable_grok = enable_grok_for_frame;
    return pipeline.process_video_frame(video, frame_index, frame_options);
}

void CabinWindow::show_frame(const int frame_index) {
    if (busy_) {
        return;
    }
    busy_ = true;

    current_frame_ = frame_index;
    slider_->blockSignals(true);
    slider_->setValue(frame_index);
    slider_->blockSignals(false);

    auto forward_result = process_view(forward_pipeline_, forward_video_, frame_index, false);
    auto reverse_result = process_view(reverse_pipeline_, reverse_video_, frame_index, false);

    if (should_refresh_grok(frame_index)) {
        refresh_grok_narration(forward_result.frame);
    } else if (grok_enabled_) {
        forward_result.grok = cached_grok_;
        reverse_result.grok = cached_grok_;
    }

    forward_canvas_->set_scene(to_qimage(forward_result.frame.image), forward_result.frame);
    reverse_canvas_->set_scene(to_qimage(reverse_result.frame.image), reverse_result.frame);

    forward_metrics_->setText(
        QString("fwd · infer %1 ms · %2 objects")
            .arg(forward_result.metrics.infer_ms, 0, 'f', 1)
            .arg(forward_result.frame.detections.size()));
    reverse_metrics_->setText(
        QString("rev · infer %1 ms · %2 objects")
            .arg(reverse_result.metrics.infer_ms, 0, 'f', 1)
            .arg(reverse_result.frame.detections.size()));

    if (grok_enabled_) {
        QString grok_text;
        if (!cached_grok_.text.empty()) {
            grok_text = QString::fromStdString(cached_grok_.text);
        }
        if (!cached_grok_.error.empty() && !grok_auth_failed_) {
            grok_text = QString("[%1] %2")
                            .arg(QString::fromStdString(cached_grok_.error),
                                 grok_text.isEmpty() ? QStringLiteral("Grok unavailable") : grok_text);
        }
        if (!grok_text.isEmpty()) {
            grok_label_->setText(QStringLiteral("Grok: %1").arg(grok_text));
        }
    }

    CabinSnapshot snapshot{};
    snapshot.forward = std::move(forward_result);
    snapshot.reverse = std::move(reverse_result);
    snapshot.grok = cached_grok_;
    if (grok_auth_failed_) {
        snapshot.grok.error.clear();
    }
    publish_panel_update(snapshot);

    busy_ = false;
    schedule_next_tick();
}

void CabinWindow::schedule_next_tick() {
    if (!playing_ || busy_) {
        return;
    }
    timer_.start();
}

void CabinWindow::on_play_pause() {
    playing_ = !playing_;
    play_button_->setText(playing_ ? QStringLiteral("Pause") : QStringLiteral("Play"));
    timer_.stop();
    if (playing_ && !busy_) {
        schedule_next_tick();
    }
}

void CabinWindow::on_slider_changed(const int value) {
    if (!playing_) {
        forward_pipeline_.reset_tracking();
        reverse_pipeline_.reset_tracking();
        show_frame(value);
    }
}

void CabinWindow::on_tick() {
    if (!playing_ || busy_) {
        return;
    }

    const int max_frames = std::min(forward_video_.frame_count(), reverse_video_.frame_count());
    if (current_frame_ + 1 >= max_frames) {
        forward_pipeline_.reset_tracking();
        reverse_pipeline_.reset_tracking();
        show_frame(0);
        return;
    }
    show_frame(current_frame_ + 1);
}

void CabinWindow::on_gear_changed(const QString& gear) {
    statusBar()->showMessage(QStringLiteral("Gear: %1 · %2").arg(gear, panel_status_hint(true)));
}

#endif
