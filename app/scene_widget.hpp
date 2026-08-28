#pragma once

#include "fovea/types.hpp"

#include <QWidget>

namespace fovea::ui {

class SceneWidget : public QWidget {
    Q_OBJECT

public:
    explicit SceneWidget(QWidget* parent = nullptr);

    void set_frame(const SceneFrame& frame);

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    SceneFrame frame_;
    QImage image_;
};

}  // namespace fovea::ui
