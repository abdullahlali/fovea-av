#pragma once

#include "fovea/pipeline.hpp"
#include "scene_window.hpp"

#include <QImage>
#include <QLabel>
#include <QMainWindow>
#include <QPushButton>
#include <QSlider>
#include <QTimer>

#if defined(FOVEA_HAS_VIDEO)
#include "fovea/video_capture.hpp"
#endif

class VideoWindow : public QMainWindow {
    Q_OBJECT

public:
#if defined(FOVEA_HAS_VIDEO)
    VideoWindow(const std::string& video_path, bool enable_grok, bool enable_panel, QWidget* parent = nullptr);
#endif

private slots:
    void on_play_pause();
    void on_slider_changed(int value);
    void on_tick();

private:
#if defined(FOVEA_HAS_VIDEO)
    void show_frame(int frame_index);

    fovea::Pipeline pipeline_;
    fovea::VideoCapture video_;
    fovea::PipelineOptions options_;

    SceneCanvas* canvas_ = nullptr;
    QLabel* metrics_label_ = nullptr;
    QLabel* grok_label_ = nullptr;
    QSlider* slider_ = nullptr;
    QPushButton* play_button_ = nullptr;
    QTimer timer_;

    int current_frame_ = 0;
    bool playing_ = false;
    bool enable_panel_ = false;

    bool grok_enabled_ = false;
    fovea::GrokResponse cached_grok_{};
    int last_grok_frame_ = -1;
#endif
};
