#pragma once

#include "fovea/types.hpp"

#include <QMainWindow>
#include <QString>

namespace fovea::ui {

class SceneWidget;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(const SceneFrame& frame, const PipelineMetrics& metrics,
                        QWidget* parent = nullptr);

private:
    SceneWidget* scene_widget_ = nullptr;
};

}  // namespace fovea::ui
