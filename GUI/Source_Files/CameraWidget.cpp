#include "CameraWidget.h"
#include <QPainter>
#include <QDateTime>
#include <QPixmap>

CameraWidget::CameraWidget(const QString& cameraName, QWidget* parent)
    : QWidget(parent), m_cameraName(cameraName), m_connectionLost(true) {

    setupUI();

    m_watchdogTimer = new QTimer(this);
    m_watchdogTimer->setInterval(3000);
    connect(m_watchdogTimer, &QTimer::timeout, this, &CameraWidget::handleTimeout);

    handleTimeout();
}

void CameraWidget::setupUI() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(5, 5, 5, 5);

    m_videoPlaceholder = new QLabel("VIDEO STREAM: " + m_cameraName, this);
    m_videoPlaceholder->setAlignment(Qt::AlignCenter);
    m_videoPlaceholder->setStyleSheet("background-color: #000; color: #444; font-size: 16px; border: 1px solid #333;");
    m_videoPlaceholder->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_videoPlaceholder->setMinimumHeight(200);

    mainLayout->addWidget(m_videoPlaceholder);

    // Telemetry Panel
    QFrame* telemetryFrame = new QFrame(this);
    telemetryFrame->setStyleSheet("background-color: #111; color: white; border-top: 1px solid #444;");
    QGridLayout* teleLayout = new QGridLayout(telemetryFrame);

    m_labelImgQuality = new QLabel("Calidad Imagen: --%", telemetryFrame);
    m_labelSignalQuality = new QLabel("Calidad Señal: --%", telemetryFrame);
    m_labelStatus = new QLabel("Estado: DESCONECTADO", telemetryFrame);
    m_labelStatus->setStyleSheet("color: red; font-weight: bold;");

    teleLayout->addWidget(new QLabel("CAM: " + m_cameraName), 0, 0);
    teleLayout->addWidget(m_labelStatus, 0, 1);
    teleLayout->addWidget(m_labelImgQuality, 1, 0);
    teleLayout->addWidget(m_labelSignalQuality, 1, 1);

    mainLayout->addWidget(telemetryFrame);

    m_overlayLabel = new QLabel("CONNECTION LOST", m_videoPlaceholder);
    m_overlayLabel->setAlignment(Qt::AlignCenter);
    m_overlayLabel->setStyleSheet("background-color: rgba(255, 0, 0, 80); color: white; font-size: 20px; font-weight: bold; border: 2px solid red;");
    m_overlayLabel->setVisible(false);
}

void CameraWidget::paintEvent(QPaintEvent* event) {
    QWidget::paintEvent(event);
    if (m_connectionLost) {
        QPainter painter(this);
        painter.setPen(QPen(Qt::red, 4));
        painter.drawRect(rect().adjusted(2, 2, -2, -2));
    }
}

void CameraWidget::resizeEvent(QResizeEvent* event) {
    QWidget::resizeEvent(event);
    m_overlayLabel->setGeometry(m_videoPlaceholder->rect());
}

void CameraWidget::updateTelemetry(float imgQuality, float signalQuality) {
    if (m_connectionLost) {
        m_connectionLost = false;
        m_overlayLabel->setVisible(false);
        m_labelStatus->setText("Estado: CONECTADO");
        m_labelStatus->setStyleSheet("color: #0f0; font-weight: bold;");
        m_videoPlaceholder->setStyleSheet("background-color: #000; color: #0f0; font-size: 16px; border: 1px solid #0f0;");
        m_videoPlaceholder->setText("");
        update();
    }

    m_labelImgQuality->setText(QString("Calidad Imagen: %1%").arg(imgQuality, 0, 'f', 1));
    m_labelSignalQuality->setText(QString("Calidad Señal: %1%").arg(signalQuality, 0, 'f', 1));

    m_watchdogTimer->start();
}

void CameraWidget::setFrame(const QImage& frame) {
    // Frame reception also counts as a heartbeat
    if (m_connectionLost) {
        setConnectionStatus(true);
    }

    m_videoPlaceholder->setPixmap(QPixmap::fromImage(frame).scaled(m_videoPlaceholder->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
    m_watchdogTimer->start();
}

void CameraWidget::setConnectionStatus(bool connected) {
    if (connected) {
        m_connectionLost = false;
        m_overlayLabel->setVisible(false);
        m_labelStatus->setText("Estado: CONECTADO");
        m_labelStatus->setStyleSheet("color: #0f0; font-weight: bold;");
        m_videoPlaceholder->setStyleSheet("background-color: #000; color: #0f0; font-size: 16px; border: 1px solid #0f0;");
        m_videoPlaceholder->setText("");
    } else {
        handleTimeout();
    }
    update();
}

void CameraWidget::handleTimeout() {
    m_connectionLost = true;
    m_overlayLabel->setGeometry(m_videoPlaceholder->rect());
    m_overlayLabel->setVisible(true);
    m_labelStatus->setText("Estado: DESCONECTADO");
    m_labelStatus->setStyleSheet("color: red; font-weight: bold;");
    m_videoPlaceholder->setStyleSheet("background-color: #1a1a1a; color: #444; font-size: 16px; border: 1px solid red;");
    m_labelImgQuality->setText("Calidad Imagen: --%");
    m_labelSignalQuality->setText("Calidad Señal: --%");
    update();
}
