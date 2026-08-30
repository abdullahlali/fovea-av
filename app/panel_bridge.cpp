#include "panel_bridge.hpp"

#include "fovea/scene_graph.hpp"

#include <QHostAddress>
#include <QJsonDocument>
#include <QJsonObject>

namespace {

QString build_panel_message(const fovea::PipelineResult& result) {
    static const fovea::SceneGraph scene_graph;

    QJsonObject metrics;
    metrics.insert("capture_ms", result.metrics.capture_ms);
    metrics.insert("infer_ms", result.metrics.infer_ms);
    metrics.insert("predict_ms", result.metrics.predict_ms);
    metrics.insert("grok_ms", result.metrics.grok_ms);
    metrics.insert("total_ms", result.metrics.total_ms);

    QJsonObject grok;
    grok.insert("text", QString::fromStdString(result.grok.text));
    grok.insert("error", QString::fromStdString(result.grok.error));

    QJsonObject camera;
    camera.insert("mode", QStringLiteral("forward"));
    camera.insert("label", QStringLiteral("Forward Camera"));

    QJsonObject payload;
    payload.insert("type", QStringLiteral("scene"));
    payload.insert(
        "scene",
        QJsonDocument::fromJson(QByteArray::fromStdString(scene_graph.to_json(result.frame))).object());
    payload.insert("metrics", metrics);
    payload.insert("grok", grok);
    payload.insert("camera", camera);

    return QString::fromUtf8(QJsonDocument(payload).toJson(QJsonDocument::Compact));
}

}  // namespace

PanelBridge& PanelBridge::instance() {
    static PanelBridge bridge;
    return bridge;
}

PanelBridge::PanelBridge(QObject* parent) : QObject(parent) {
    connect(&server_, &QWebSocketServer::newConnection, this, &PanelBridge::on_new_connection);
}

void PanelBridge::start(const quint16 port) {
    if (server_.isListening()) {
        return;
    }

    if (!server_.listen(QHostAddress::LocalHost, port)) {
        qWarning("PanelBridge: failed to listen on port %u", port);
        return;
    }

    qInfo("PanelBridge: ws://127.0.0.1:%u", port);
}

void PanelBridge::publish(const fovea::PipelineResult& result) {
    if (clients_.isEmpty()) {
        return;
    }

    const QString message = build_panel_message(result);
    for (QWebSocket* client : clients_) {
        client->sendTextMessage(message);
    }
}

void PanelBridge::on_new_connection() {
    QWebSocket* socket = server_.nextPendingConnection();
    clients_.append(socket);

    connect(socket, &QWebSocket::disconnected, this, &PanelBridge::on_socket_disconnected);
    connect(socket, &QWebSocket::destroyed, this, [this, socket]() { clients_.removeAll(socket); });

    qInfo("PanelBridge: passenger panel connected (%d)", static_cast<int>(clients_.size()));
}

void PanelBridge::on_socket_disconnected() {
    auto* socket = qobject_cast<QWebSocket*>(sender());
    if (socket != nullptr) {
        clients_.removeAll(socket);
        socket->deleteLater();
        qInfo("PanelBridge: passenger panel disconnected (%d)", static_cast<int>(clients_.size()));
    }
}

void maybe_publish_panel(const fovea::PipelineResult& result, const bool enabled) {
    if (enabled) {
        PanelBridge::instance().publish(result);
    }
}

QString panel_status_hint(const bool enabled) {
    if (!enabled) {
        return {};
    }
    return QStringLiteral(
        "Driver display · Passenger panel: http://localhost:5173 (ws://127.0.0.1:8765)");
}
