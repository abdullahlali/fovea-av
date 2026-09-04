#pragma once

#include "fovea/pipeline.hpp"

#include <QElapsedTimer>
#include <QMainWindow>
#include <QTimer>

#if defined(FOVEA_HAS_VIDEO)
#include "fovea/video_capture.hpp"
#endif

#if defined(FOVEA_HAS_WEBENGINE)
#include <QWebEngineView>
#endif

#if defined(FOVEA_HAS_VIDEO)

class PanelDisplay : public QMainWindow {
    Q_OBJECT

public:
    PanelDisplay(const std::string& forward_video,
                 const std::string& reverse_video,
                 bool enable_grok,
                 const std::string& scenario_path,
                 const QString& gear = QStringLiteral("D"),
                 QWidget* parent = nullptr);

private slots:
    void on_tick();
    void on_gear_changed(const QString& gear);

private:
    void process_and_publish();
    bool should_refresh_grok() const;
    void refresh_grok_narration(const fovea::SceneFrame& frame);

    fovea::Pipeline forward_pipeline_;
    fovea::Pipeline reverse_pipeline_;
    fovea::VideoCapture forward_video_;
    fovea::VideoCapture reverse_video_;
    fovea::PipelineOptions options_;

    fovea::PipelineResult cached_forward_{};
    fovea::PipelineResult cached_reverse_{};
    bool has_forward_cache_ = false;
    bool has_reverse_cache_ = false;

    QTimer timer_;
    int current_frame_ = 0;
    bool busy_ = false;
    bool alternate_forward_ = true;
    bool grok_enabled_ = false;
    bool grok_auth_failed_ = false;
    fovea::GrokResponse cached_grok_{};
    int ticks_since_grok_ = 0;

    static constexpr int kTickMs = 180;
    static constexpr int kGrokEveryTicks = 28;  // ~5s at 180ms

#if defined(FOVEA_HAS_WEBENGINE)
    QWebEngineView* panel_view_ = nullptr;
#endif
};

#endif
