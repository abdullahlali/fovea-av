#pragma once

#include "fovea/pipeline.hpp"
#include "panel_bridge.hpp"

#include <QElapsedTimer>
#include <QObject>
#include <QTimer>

#if defined(FOVEA_HAS_VIDEO)
#include "fovea/video_capture.hpp"
#endif

class PanelEngine : public QObject {
    Q_OBJECT

public:
#if defined(FOVEA_HAS_VIDEO)
    PanelEngine(const std::string& forward_video,
                const std::string& reverse_video,
                bool enable_grok,
                const std::string& scenario_path,
                const QString& gear = QStringLiteral("D"),
                QObject* parent = nullptr);

    void start();
#endif

private slots:
    void on_tick();

private:
#if defined(FOVEA_HAS_VIDEO)
    void process_tick();
    bool should_refresh_grok() const;
    void refresh_grok_narration(const fovea::SceneFrame& frame);

    fovea::Pipeline forward_pipeline_;
    fovea::Pipeline reverse_pipeline_;
    fovea::VideoCapture forward_video_;
    fovea::VideoCapture reverse_video_;
    fovea::PipelineOptions options_;

    QTimer timer_;
    QElapsedTimer grok_timer_;

    fovea::PipelineResult cached_forward_{};
    fovea::PipelineResult cached_reverse_{};
    bool has_forward_cache_ = false;
    bool has_reverse_cache_ = false;

    fovea::GrokResponse cached_grok_{};
    bool grok_enabled_ = false;
    bool grok_auth_failed_ = false;
    qint64 last_grok_ms_ = 0;

    int frame_index_ = 0;
    bool busy_ = false;
    bool alternate_forward_ = true;

    static constexpr int kTickMs = 280;
    static constexpr int kGrokCooldownMs = 5000;
#endif
};
