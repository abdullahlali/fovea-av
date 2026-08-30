#include "scene_window.hpp"

#include <QPainter>
#include <QPen>
#include <QStatusBar>
#include <QVBoxLayout>

namespace {

QColor confidence_color(float confidence) {
    const int alpha = static_cast<int>(80.0F + confidence * 175.0F);
    return QColor(255, 198, 41, alpha);
}

}  // namespace

QImage to_qimage(const fovea::ImageBuffer& image) {
    if (image.pixels.empty() || image.width <= 0 || image.height <= 0) {
        return {};
    }

    QImage qimage(image.width, image.height, QImage::Format_RGB888);
    for (int y = 0; y < image.height; ++y) {
        auto* scan_line = reinterpret_cast<std::uint8_t*>(qimage.scanLine(y));
        const std::size_t offset = static_cast<std::size_t>(y * image.width * image.channels);
        for (int x = 0; x < image.width; ++x) {
            const std::size_t index = offset + static_cast<std::size_t>(x * image.channels);
            scan_line[x * 3 + 0] = image.pixels[index + 0];
            scan_line[x * 3 + 1] = image.pixels[index + 1];
            scan_line[x * 3 + 2] = image.pixels[index + 2];
        }
    }
    return qimage;
}

SceneCanvas::SceneCanvas(QWidget* parent) : QWidget(parent) {
    setMinimumSize(960, 540);
    setAutoFillBackground(true);
    QPalette palette = this->palette();
    palette.setColor(QPalette::Window, QColor(12, 12, 14));
    setPalette(palette);
}

void SceneCanvas::set_scene(const QImage& image, const fovea::SceneFrame& frame) {
    image_ = image;
    frame_ = frame;
    update();
}

void SceneCanvas::paintEvent(QPaintEvent* event) {
    QWidget::paintEvent(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    if (image_.isNull()) {
        painter.setPen(QColor(220, 220, 220));
        painter.drawText(rect(), Qt::AlignCenter, "No image loaded");
        return;
    }

    const QSize target = image_.size().scaled(size(), Qt::KeepAspectRatio);
    const QRect image_rect{
        (width() - target.width()) / 2,
        (height() - target.height()) / 2,
        target.width(),
        target.height(),
    };

    painter.drawImage(image_rect, image_);

    const float scale_x =
        static_cast<float>(image_rect.width()) / static_cast<float>(image_.width());
    const float scale_y =
        static_cast<float>(image_rect.height()) / static_cast<float>(image_.height());

  auto draw_box = [&](const fovea::BoundingBox& bbox, float confidence, bool ghost) {
        const QRectF box{
            image_rect.left() + bbox.x * scale_x,
            image_rect.top() + bbox.y * scale_y,
            bbox.width * scale_x,
            bbox.height * scale_y,
        };

        QPen pen(confidence_color(confidence), ghost ? 2.0 : 3.0);
        if (ghost) {
            pen.setStyle(Qt::DashLine);
            pen.setColor(QColor(255, 255, 255, 140));
        }
        painter.setPen(pen);
        painter.setBrush(Qt::NoBrush);
        painter.drawRect(box);
    };

    for (const auto& prediction : frame_.predictions) {
        draw_box(prediction.bbox, 0.5F, true);
    }

    for (const auto& detection : frame_.detections) {
        draw_box(detection.bbox, detection.confidence, false);

        const QRectF box{
            image_rect.left() + detection.bbox.x * scale_x,
            image_rect.top() + detection.bbox.y * scale_y,
            detection.bbox.width * scale_x,
            detection.bbox.height * scale_y,
        };

        const QString label = QString::fromStdString(detection.label) +
                              QString(" %1%").arg(static_cast<int>(detection.confidence * 100.0F));
        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(0, 0, 0, 170));
        const QRectF text_rect = painter.boundingRect(box, Qt::AlignLeft | Qt::AlignTop, label);
        painter.drawRect(text_rect.adjusted(-4, -2, 4, 2));
        painter.setPen(QColor(255, 220, 120));
        painter.drawText(text_rect, Qt::AlignLeft | Qt::AlignTop, label);
    }
}

SceneWindow::SceneWindow(const fovea::PipelineResult& result, QWidget* parent)
    : QMainWindow(parent) {
    setWindowTitle("Fovea — Autonomy Visualization");
    resize(1280, 800);

    auto* central = new QWidget(this);
    auto* layout = new QVBoxLayout(central);

    canvas_ = new SceneCanvas(central);
    metrics_label_ = new QLabel(central);
    metrics_label_->setStyleSheet("color: #f5d66d; padding: 8px; font-family: Menlo, monospace;");
    metrics_label_->setText(
        QString("capture: %1 ms | infer: %2 ms | predict: %3 ms | grok: %4 ms | total: %5 ms | detections: %6")
            .arg(result.metrics.capture_ms, 0, 'f', 2)
            .arg(result.metrics.infer_ms, 0, 'f', 2)
            .arg(result.metrics.predict_ms, 0, 'f', 3)
            .arg(result.metrics.grok_ms, 0, 'f', 2)
            .arg(result.metrics.total_ms, 0, 'f', 2)
            .arg(result.frame.detections.size()));

    grok_label_ = new QLabel(central);
    grok_label_->setWordWrap(true);
    grok_label_->setStyleSheet(
        "color: #f0f0f0; background: #1a1a1f; padding: 12px; border-top: 1px solid #3a3a44; "
        "font-size: 14px;");
    grok_label_->setVisible(result.grok.text.size() > 0);
    if (!result.grok.text.empty()) {
        QString grok_text = QString::fromStdString(result.grok.text);
        if (!result.grok.error.empty()) {
            grok_text = QString("[warning] %1\n\n%2")
                            .arg(QString::fromStdString(result.grok.error),
                                 grok_text);
        }
        grok_label_->setText(QString("Grok: %1").arg(grok_text));
    }

    layout->addWidget(canvas_, 1);
    layout->addWidget(metrics_label_);
    layout->addWidget(grok_label_);
    setCentralWidget(central);

    canvas_->set_scene(to_qimage(result.frame.image), result.frame);

    statusBar()->showMessage(QString::fromStdString(result.frame.source_path));
}
