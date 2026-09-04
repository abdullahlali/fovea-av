#pragma once

#include "fovea/pipeline.hpp"
#include "panel_bridge.hpp"
#include "scene_window.hpp"

#include <QHBoxLayout>
#include <QLabel>
#include <QMainWindow>
#include <QPushButton>
#include <QSlider>
#include <QSplitter>
#include <QElapsedTimer>
#include <QTimer>
#include <QVBoxLayout>

#if defined(FOVEA_HAS_WEBENGINE)
#include <QWebEngineView>
#endif

#if defined(FOVEA_HAS_VIDEO)
#include "fovea/camera_view.hpp"
#include "fovea/video_capture.hpp"
#endif

class CabinWindow : public QMainWindow {
    Q_OBJECT

public:
#if defined(FOVEA_HAS_VIDEO)
    CabinWindow(const std::string& forward_video,
                const std::string& reverse_video,
                bool enable_grok,
                const std::string& scenario_path,
                const QString& gear = QStringLiteral("D"),
                QWidget* parent = nullptr);
#endif

private slots:
    void on_play_pause();
    void on_slider_changed(int value);
    void on_tick();
    void on_gear_changed(const QString& gear);

private:
#if defined(FOVEA_HAS_VIDEO)
    void show_frame(int frame_index);
    void schedule_next_tick();
    void publish_panel_update(const CabinSnapshot& snapshot);
    bool should_refresh_grok(int frame_index) const;
    void refresh_grok_narration(const fovea::SceneFrame& frame);
    fovea::PipelineResult process_view(fovea::Pipeline& pipeline,
                                       fovea::VideoCapture& video,
                                       int frame_index,
                                       bool enable_grok_for_frame);

    fovea::Pipeline forward_pipeline_;
    fovea::Pipeline reverse_pipeline_;
    fovea::VideoCapture forward_video_;
    fovea::VideoCapture reverse_video_;
    fovea::PipelineOptions options_;

    SceneCanvas* forward_canvas_ = nullptr;
    SceneCanvas* reverse_canvas_ = nullptr;
    QLabel* forward_metrics_ = nullptr;
    QLabel* reverse_metrics_ = nullptr;
    QLabel* grok_label_ = nullptr;
    QSlider* slider_ = nullptr;
    QPushButton* play_button_ = nullptr;
    QTimer timer_;

#if defined(FOVEA_HAS_WEBENGINE)
    QWebEngineView* panel_view_ = nullptr;
#endif

    int current_frame_ = 0;
    bool playing_ = false;
    bool busy_ = false;
    bool grok_enabled_ = false;
    bool grok_auth_failed_ = false;
    fovea::GrokResponse cached_grok_{};
    int last_grok_frame_ = -1;
    QElapsedTimer publish_timer_;
    qint64 last_publish_ms_ = 0;
    static constexpr int kPublishIntervalMs = 100;
    static constexpr int kGrokCooldownSeconds = 8;
#endif
};
