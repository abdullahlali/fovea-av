#pragma once

#include <QMainWindow>

#if defined(FOVEA_HAS_WEBENGINE)
class QWebEngineView;
#endif

class PanelWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit PanelWindow(QWidget* parent = nullptr);
};
