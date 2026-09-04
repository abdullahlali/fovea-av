#pragma once

#include "fovea/pipeline.hpp"

#include <QObject>
#include <QWebSocket>
#include <QWebSocketServer>
#include <QString>

#include <optional>

struct CabinSnapshot {
    fovea::PipelineResult forward;
    fovea::PipelineResult reverse;
    fovea::GrokResponse grok;
};

class PanelBridge : public QObject {
    Q_OBJECT

public:
    static PanelBridge& instance();

    void start(quint16 port = 8765);
    void set_gear(const QString& gear);
    [[nodiscard]] QString gear() const;
    void publish_cabin(const CabinSnapshot& snapshot);

    // Legacy single-camera publish (still images / driver-only mode).
    void set_camera_profile(const std::string& mode, const std::string& label);
    void publish(const fovea::PipelineResult& result);

signals:
    void gear_changed(const QString& gear);
    void focus_changed(const QString& focus);

private slots:
    void on_new_connection();
    void on_socket_disconnected();
    void on_text_message(const QString& message);

private:
    explicit PanelBridge(QObject* parent = nullptr);
    QString build_cabin_message(const CabinSnapshot& snapshot) const;
    QString build_legacy_message(const fovea::PipelineResult& result) const;
    void send_snapshot(QWebSocket* client) const;

    QWebSocketServer server_{QStringLiteral("FoveaPanel"), QWebSocketServer::NonSecureMode};
    QList<QWebSocket*> clients_;
    QString gear_{QStringLiteral("D")};
    QString focus_{QStringLiteral("both")};
    QString camera_mode_{QStringLiteral("forward")};
    QString camera_label_{QStringLiteral("Forward Camera")};
    std::optional<fovea::PipelineResult> last_legacy_;
    std::optional<CabinSnapshot> last_cabin_;
    bool cabin_mode_ = false;
};

void maybe_publish_panel(const fovea::PipelineResult& result, bool enabled);

QString panel_status_hint(bool enabled);

QString panel_dist_url();
