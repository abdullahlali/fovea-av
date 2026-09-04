#include "panel_window.hpp"

#include "panel_static_server.hpp"

#if defined(FOVEA_HAS_WEBENGINE)

#include <QWebEngineSettings>
#include <QWebEngineView>

PanelWindow::PanelWindow(QWidget* parent) : QMainWindow(parent) {
    setWindowTitle(QStringLiteral("Fovea — Driver Display"));
    resize(1100, 640);
    setMinimumSize(900, 520);

    auto* panel = new QWebEngineView(this);
    panel->setStyleSheet(QStringLiteral("background: #070708;"));

    auto* settings = panel->settings();
    settings->setAttribute(QWebEngineSettings::JavascriptEnabled, true);
    settings->setAttribute(QWebEngineSettings::LocalContentCanAccessRemoteUrls, true);

    const QString dist_dir = resolve_panel_dist_directory();
    if (!dist_dir.isEmpty() && PanelStaticServer::instance().start(dist_dir)) {
        panel->load(QUrl(PanelStaticServer::instance().base_url()));
    } else {
        panel->setHtml(QStringLiteral(
            "<body style='background:#070708;color:#eee;font-family:sans-serif;padding:32px'>"
            "<h2>Panel not built</h2><p>Run <code>cd panel && npm run build</code></p></body>"));
    }

    setCentralWidget(panel);
}

#else

PanelWindow::PanelWindow(QWidget* parent) : QMainWindow(parent) {
    setWindowTitle(QStringLiteral("Fovea — Driver Display"));
}

#endif
