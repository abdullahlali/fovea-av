#include "panel_static_server.hpp"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QHostAddress>
#include <QTcpServer>
#include <QTcpSocket>

namespace {

QString content_type_for(const QString& path) {
    if (path.endsWith(QStringLiteral(".html"))) return QStringLiteral("text/html; charset=utf-8");
    if (path.endsWith(QStringLiteral(".js"))) return QStringLiteral("application/javascript; charset=utf-8");
    if (path.endsWith(QStringLiteral(".css"))) return QStringLiteral("text/css; charset=utf-8");
    if (path.endsWith(QStringLiteral(".svg"))) return QStringLiteral("image/svg+xml");
    if (path.endsWith(QStringLiteral(".png"))) return QStringLiteral("image/png");
    if (path.endsWith(QStringLiteral(".woff2"))) return QStringLiteral("font/woff2");
    return QStringLiteral("application/octet-stream");
}

void write_response(QTcpSocket* socket, int status, const QString& status_text, const QByteArray& body,
                    const QString& content_type) {
    QByteArray response;
    response += "HTTP/1.1 " + QByteArray::number(status) + ' ' + status_text.toUtf8() + "\r\n";
    response += "Content-Type: " + content_type.toUtf8() + "\r\n";
    response += "Content-Length: " + QByteArray::number(body.size()) + "\r\n";
    response += "Access-Control-Allow-Origin: *\r\n";
    response += "Connection: close\r\n\r\n";
    response += body;
    socket->write(response);
    socket->flush();
    socket->disconnectFromHost();
}

}  // namespace

PanelStaticServer& PanelStaticServer::instance() {
    static PanelStaticServer server;
    return server;
}

PanelStaticServer::PanelStaticServer(QObject* parent) : QObject(parent) {}

bool PanelStaticServer::start(const QString& root_directory, const quint16 port) {
    if (running_) {
        return true;
    }

    const QFileInfo root_info(root_directory);
    if (!root_info.exists() || !root_info.isDir()) {
        qWarning("PanelStaticServer: dist directory not found: %s", qPrintable(root_directory));
        return false;
    }

    root_ = root_info.absoluteFilePath();
    port_ = port;

    auto* server = new QTcpServer(this);
    if (!server->listen(QHostAddress::LocalHost, port_)) {
        qWarning("PanelStaticServer: failed to listen on port %u", port_);
        server->deleteLater();
        return false;
    }

    connect(server, &QTcpServer::newConnection, this, [this, server]() {
        while (server->hasPendingConnections()) {
            QTcpSocket* socket = server->nextPendingConnection();
            connect(socket, &QTcpSocket::readyRead, this, [this, socket]() {
                const QByteArray request = socket->readAll();
                const QList<QByteArray> lines = request.split('\n');
                if (lines.isEmpty()) {
                    write_response(socket, 400, QStringLiteral("Bad Request"), {}, QStringLiteral("text/plain"));
                    return;
                }

                const QList<QByteArray> parts = lines[0].trimmed().split(' ');
                if (parts.size() < 2 || parts[0] != "GET") {
                    write_response(socket, 405, QStringLiteral("Method Not Allowed"), {}, QStringLiteral("text/plain"));
                    return;
                }

                QString path = QString::fromUtf8(parts[1]);
                if (path == QStringLiteral("/")) {
                    path = QStringLiteral("/index.html");
                }

                const QString local_path = QDir(root_).absoluteFilePath(path.mid(1));
                const QFileInfo file(local_path);
                if (!file.exists() || !file.isFile() || !file.absoluteFilePath().startsWith(root_)) {
                    write_response(socket, 404, QStringLiteral("Not Found"), QByteArray("Not found"),
                                     QStringLiteral("text/plain"));
                    return;
                }

                QFile file_handle(file.absoluteFilePath());
                if (!file_handle.open(QIODevice::ReadOnly)) {
                    write_response(socket, 500, QStringLiteral("Internal Server Error"), {}, QStringLiteral("text/plain"));
                    return;
                }

                write_response(socket, 200, QStringLiteral("OK"), file_handle.readAll(),
                                 content_type_for(file.absoluteFilePath()));
            });
        }
    });

    running_ = true;
    qInfo("PanelStaticServer: http://127.0.0.1:%u", port_);
    return true;
}

QString PanelStaticServer::base_url() const {
    return QStringLiteral("http://127.0.0.1:%1/").arg(port_);
}

QString resolve_panel_dist_directory() {
    const QByteArray env = qgetenv("FOVEA_PANEL_DIST");
    if (!env.isEmpty()) {
        const QFileInfo file(QString::fromLocal8Bit(env));
        if (file.exists()) {
            return file.absoluteDir().absolutePath();
        }
    }

    const QDir app_dir(QCoreApplication::applicationDirPath());
    const QStringList candidates = {
        app_dir.absoluteFilePath(QStringLiteral("../panel/dist")),
        app_dir.absoluteFilePath(QStringLiteral("../../panel/dist")),
        app_dir.absoluteFilePath(QStringLiteral("../../../panel/dist")),
        QDir(QCoreApplication::applicationDirPath()).absoluteFilePath(QStringLiteral("../../../../panel/dist")),
        QStringLiteral("%1/panel/dist").arg(QDir::currentPath()),
    };

    for (const QString& candidate : candidates) {
        const QFileInfo index_file(QDir(candidate).absoluteFilePath(QStringLiteral("index.html")));
        if (index_file.exists()) {
            return index_file.absoluteDir().absolutePath();
        }
    }

    return {};
}
