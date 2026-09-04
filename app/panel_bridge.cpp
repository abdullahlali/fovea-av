#include "panel_bridge.hpp"
#include "panel_static_server.hpp"

#include "fovea/scene_graph.hpp"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QHostAddress>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QUrl>

namespace {

QJsonObject metrics_to_json(const fovea::PipelineMetrics& metrics) {
    QJsonObject object;
    object.insert("capture_ms", metrics.capture_ms);
    object.insert("infer_ms", metrics.infer_ms);
    object.insert("predict_ms", metrics.predict_ms);
    object.insert("grok_ms", metrics.grok_ms);
    object.insert("total_ms", metrics.total_ms);
    return object;
}

QJsonObject scene_to_json(const fovea::SceneFrame& frame) {
    static const fovea::SceneGraph scene_graph;
    return QJsonDocument::fromJson(QByteArray::fromStdString(scene_graph.to_json(frame))).object();
}

QJsonObject camera_view_json(const fovea::PipelineResult& result, const QString& mode) {
    QJsonObject view;
    view.insert("mode", mode);
    view.insert("scene", scene_to_json(result.frame));
    view.insert("metrics", metrics_to_json(result.metrics));
    return view;
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

void PanelBridge::set_gear(const QString& gear) {
    if (gear_ == gear) {
        return;
    }
    gear_ = gear;
    emit gear_changed(gear_);
}

QString PanelBridge::gear() const {
    return gear_;
}

void PanelBridge::set_camera_profile(const std::string& mode, const std::string& label) {
    cabin_mode_ = false;
    camera_mode_ = QString::fromStdString(mode);
    camera_label_ = QString::fromStdString(label);
}

QString PanelBridge::build_legacy_message(const fovea::PipelineResult& result) const {
    QJsonObject grok;
    grok.insert("text", QString::fromStdString(result.grok.text));
    grok.insert("error", QString::fromStdString(result.grok.error));

    QJsonObject camera;
    camera.insert("mode", camera_mode_);
    camera.insert("label", camera_label_);

    QJsonObject payload;
    payload.insert("type", QStringLiteral("scene"));
    payload.insert("scene", scene_to_json(result.frame));
    payload.insert("metrics", metrics_to_json(result.metrics));
    payload.insert("grok", grok);
    payload.insert("camera", camera);
    payload.insert("gear", gear_);
    payload.insert("focus", focus_);

    return QString::fromUtf8(QJsonDocument(payload).toJson(QJsonDocument::Compact));
}

QString PanelBridge::build_cabin_message(const CabinSnapshot& snapshot) const {
    QJsonObject grok;
    grok.insert("text", QString::fromStdString(snapshot.grok.text));
    grok.insert("error", QString::fromStdString(snapshot.grok.error));

    QJsonObject payload;
    payload.insert("type", QStringLiteral("cabin"));
    payload.insert("gear", gear_);
    payload.insert("focus", focus_);
    payload.insert("forward", camera_view_json(snapshot.forward, QStringLiteral("forward")));
    payload.insert("reverse", camera_view_json(snapshot.reverse, QStringLiteral("reverse")));
    payload.insert("grok", grok);

    return QString::fromUtf8(QJsonDocument(payload).toJson(QJsonDocument::Compact));
}

void PanelBridge::publish_cabin(const CabinSnapshot& snapshot) {
    cabin_mode_ = true;
    last_cabin_ = snapshot;

    if (clients_.isEmpty()) {
        return;
    }

    const QString message = build_cabin_message(snapshot);
    for (QWebSocket* client : clients_) {
        client->sendTextMessage(message);
    }
}

void PanelBridge::publish(const fovea::PipelineResult& result) {
    cabin_mode_ = false;
    last_legacy_ = result;

    if (clients_.isEmpty()) {
        return;
    }

    const QString message = build_legacy_message(result);
    for (QWebSocket* client : clients_) {
        client->sendTextMessage(message);
    }
}

void PanelBridge::send_snapshot(QWebSocket* client) const {
    if (client == nullptr) {
        return;
    }
    if (cabin_mode_ && last_cabin_.has_value()) {
        client->sendTextMessage(build_cabin_message(*last_cabin_));
        return;
    }
    if (last_legacy_.has_value()) {
        client->sendTextMessage(build_legacy_message(*last_legacy_));
    }
}

void PanelBridge::on_new_connection() {
    QWebSocket* socket = server_.nextPendingConnection();
    clients_.append(socket);

    connect(socket, &QWebSocket::textMessageReceived, this, &PanelBridge::on_text_message);
    connect(socket, &QWebSocket::disconnected, this, &PanelBridge::on_socket_disconnected);
    connect(socket, &QWebSocket::destroyed, this, [this, socket]() { clients_.removeAll(socket); });

    send_snapshot(socket);
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

void PanelBridge::on_text_message(const QString& message) {
    const QJsonDocument doc = QJsonDocument::fromJson(message.toUtf8());
    if (!doc.isObject()) {
        return;
    }

    const QJsonObject payload = doc.object();
    if (payload.value(QStringLiteral("type")).toString() != QStringLiteral("control")) {
        return;
    }

    if (payload.contains(QStringLiteral("gear"))) {
        const QString gear = payload.value(QStringLiteral("gear")).toString().toUpper();
        if (!gear.isEmpty()) {
            set_gear(gear);
        }
    }

    if (payload.contains(QStringLiteral("focus"))) {
        const QString focus = payload.value(QStringLiteral("focus")).toString();
        if (!focus.isEmpty() && focus_ != focus) {
            focus_ = focus;
            emit focus_changed(focus_);
        }
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
    return QStringLiteral("Fovea cabin · embedded passenger display");
}

QString panel_dist_url() {
    const QString dist_dir = resolve_panel_dist_directory();
    if (!dist_dir.isEmpty() && PanelStaticServer::instance().start(dist_dir)) {
        return PanelStaticServer::instance().base_url();
    }

    return QStringLiteral("http://127.0.0.1:5173");
}
