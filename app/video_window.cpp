#include "video_window.hpp"

#if defined(FOVEA_HAS_VIDEO)

#include "panel_bridge.hpp"

#include <QHBoxLayout>
#include <QStatusBar>
#include <QVBoxLayout>

#include <cmath>

VideoWindow::VideoWindow(const std::string& video_path,
                         const bool enable_grok,
                         const bool enable_panel,
                         const std::string& scenario_path,
                         const fovea::CameraProfile& camera,
                         QWidget* parent)
    : QMainWindow(parent), enable_panel_(enable_panel), camera_(camera) {
    setWindowTitle(QString("Fovea — %1").arg(QString::fromStdString(camera.label)));
    resize(1280, 860);

    options_.enable_grok = enable_grok;
    options_.scenario_path = scenario_path;
    grok_enabled_ = enable_grok;
    video_.open(video_path);
    pipeline_.reset_tracking();

    auto* central = new QWidget(this);
    auto* layout = new QVBoxLayout(central);

    canvas_ = new SceneCanvas(central);
    slider_ = new QSlider(Qt::Horizontal, central);
    slider_->setRange(0, std::max(0, video_.frame_count() - 1));

    auto* controls = new QHBoxLayout();
    play_button_ = new QPushButton("Play", central);
    controls->addWidget(play_button_);
    controls->addWidget(slider_, 1);

    metrics_label_ = new QLabel(central);
    metrics_label_->setStyleSheet("color: #f5d66d; padding: 8px; font-family: Menlo, monospace;");

    grok_label_ = new QLabel(central);
    grok_label_->setWordWrap(true);
    grok_label_->setStyleSheet(
        "color: #f0f0f0; background: #1a1a1f; padding: 12px; border-top: 1px solid #3a3a44;");
    grok_label_->setVisible(enable_grok);

    layout->addWidget(canvas_, 1);
    layout->addLayout(controls);
    layout->addWidget(metrics_label_);
    layout->addWidget(grok_label_);
    setCentralWidget(central);

    connect(play_button_, &QPushButton::clicked, this, &VideoWindow::on_play_pause);
    connect(slider_, &QSlider::valueChanged, this, &VideoWindow::on_slider_changed);
    connect(&timer_, &QTimer::timeout, this, &VideoWindow::on_tick);

    const int interval_ms = std::max(1, static_cast<int>(1000.0 / video_.fps()));
    timer_.setInterval(interval_ms);

    const QString status = panel_status_hint(enable_panel_);
    statusBar()->showMessage(status.isEmpty() ? QString::fromStdString(video_path) : status);
    show_frame(0);
}

void VideoWindow::show_frame(int frame_index) {
    current_frame_ = frame_index;
    slider_->blockSignals(true);
    slider_->setValue(frame_index);
    slider_->blockSignals(false);

    fovea::PipelineOptions frame_options = options_;
    const int grok_interval_frames =
        std::max(1, static_cast<int>(std::lround(video_.fps() * 3.0)));
    const bool should_refresh_grok =
        grok_enabled_ &&
        (last_grok_frame_ < 0 ||
         std::abs(frame_index - last_grok_frame_) >= grok_interval_frames);
    frame_options.enable_grok = should_refresh_grok;

    auto result = pipeline_.process_video_frame(video_, frame_index, frame_options);

    if (grok_enabled_ && !should_refresh_grok) {
        result.grok = cached_grok_;
    } else if (grok_enabled_ && should_refresh_grok) {
        cached_grok_ = result.grok;
        last_grok_frame_ = frame_index;
    }

    canvas_->set_scene(to_qimage(result.frame.image), result.frame);

    metrics_label_->setText(
        QString("frame: %1/%2 | capture: %3 ms | infer: %4 ms | grok: %5 ms | total: %6 ms | detections: %7")
            .arg(frame_index + 1)
            .arg(video_.frame_count())
            .arg(result.metrics.capture_ms, 0, 'f', 2)
            .arg(result.metrics.infer_ms, 0, 'f', 2)
            .arg(result.metrics.grok_ms, 0, 'f', 0)
            .arg(result.metrics.total_ms, 0, 'f', 2)
            .arg(result.frame.detections.size()));

    if (grok_enabled_) {
        QString grok_text = QString::fromStdString(result.grok.text);
        if (!result.grok.error.empty()) {
            grok_text = QString("[%1] %2")
                            .arg(QString::fromStdString(result.grok.error),
                                 grok_text);
        }
        if (!grok_text.isEmpty()) {
            grok_label_->setText(QString("Grok: %1").arg(grok_text));
        }
    }

    maybe_publish_panel(result, enable_panel_);
}

void VideoWindow::on_play_pause() {
    playing_ = !playing_;
    play_button_->setText(playing_ ? "Pause" : "Play");
    if (playing_) {
        timer_.start();
    } else {
        timer_.stop();
    }
}

void VideoWindow::on_slider_changed(int value) {
    if (!playing_) {
        pipeline_.reset_tracking();
        show_frame(value);
    }
}

void VideoWindow::on_tick() {
    if (current_frame_ + 1 >= video_.frame_count()) {
        pipeline_.reset_tracking();
        show_frame(0);
        return;
    }
    show_frame(current_frame_ + 1);
}

#endif
