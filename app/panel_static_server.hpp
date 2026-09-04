#pragma once

#include <QtGlobal>

#include <QObject>
#include <QString>

class PanelStaticServer : public QObject {
    Q_OBJECT

public:
    static PanelStaticServer& instance();

    bool start(const QString& root_directory, quint16 port = 8766);
    [[nodiscard]] QString base_url() const;

private:
    explicit PanelStaticServer(QObject* parent = nullptr);

    QString root_;
    quint16 port_ = 8766;
    bool running_ = false;
};

QString resolve_panel_dist_directory();
