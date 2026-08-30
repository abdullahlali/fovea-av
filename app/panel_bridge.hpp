#pragma once

#include "fovea/pipeline.hpp"

#include <QObject>
#include <QWebSocket>
#include <QWebSocketServer>

class PanelBridge : public QObject {
    Q_OBJECT

public:
    static PanelBridge& instance();

    void start(quint16 port = 8765);
    void publish(const fovea::PipelineResult& result);

private slots:
    void on_new_connection();
    void on_socket_disconnected();

private:
    explicit PanelBridge(QObject* parent = nullptr);

    QWebSocketServer server_{QStringLiteral("FoveaPanel"), QWebSocketServer::NonSecureMode};
    QList<QWebSocket*> clients_;
};

void maybe_publish_panel(const fovea::PipelineResult& result, bool enabled);
