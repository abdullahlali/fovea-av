#pragma once

#include "fovea/camera_view.hpp"
#include "fovea/pipeline.hpp"

#include <QImage>
#include <QLabel>
#include <QMainWindow>
#include <QString>

class SceneCanvas : public QWidget {
    Q_OBJECT

public:
    explicit SceneCanvas(QWidget* parent = nullptr);

    void set_scene(const QImage& image, const fovea::SceneFrame& frame);

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    QImage image_;
    fovea::SceneFrame frame_;
};

class SceneWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit SceneWindow(const fovea::PipelineResult& result,
                         bool enable_panel = false,
                         const fovea::CameraProfile& camera = fovea::camera_profile(fovea::CameraView::Forward),
                         QWidget* parent = nullptr);

private:
    SceneCanvas* canvas_ = nullptr;
    QLabel* metrics_label_ = nullptr;
    QLabel* grok_label_ = nullptr;
    bool enable_panel_ = false;
};

QImage to_qimage(const fovea::ImageBuffer& image);
