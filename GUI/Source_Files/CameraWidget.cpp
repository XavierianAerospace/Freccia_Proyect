#include "CameraWidget.h"
#include <QPainter>
#include <QDateTime>
#include <QPixmap>
#include <cstring>

CameraWidget::CameraWidget(const QString& cameraName, QWidget* parent)
    : QWidget(parent), m_cameraName(cameraName), m_connectionLost(true) {

    setupUI();
    setCursor(Qt::PointingHandCursor);

    m_watchdogTimer = new QTimer(this);
    m_watchdogTimer->setInterval(3000);
    connect(m_watchdogTimer, &QTimer::timeout, this, &CameraWidget::handleTimeout);

    handleTimeout();
}

void CameraWidget::setupUI() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(5, 5, 5, 5);

    m_videoRenderer = new VideoRenderer(this);
    m_videoRenderer->setMinimumHeight(50);
    m_videoRenderer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    mainLayout->addWidget(m_videoRenderer);

    // Telemetry Panel
    m_telemetryFrame = new QFrame(this);
    m_telemetryFrame->setStyleSheet("background-color: #111; color: white; border-top: 1px solid #444;");
    QGridLayout* teleLayout = new QGridLayout(m_telemetryFrame);

    m_labelImgQuality = new QLabel("Calidad Imagen: --%", m_telemetryFrame);
    m_labelSignalQuality = new QLabel("Calidad Señal: --%", m_telemetryFrame);
    m_labelStatus = new QLabel("Estado: DESCONECTADO", m_telemetryFrame);
    m_labelStatus->setStyleSheet("color: red; font-weight: bold;");

    teleLayout->addWidget(new QLabel("CAM: " + m_cameraName), 0, 0);
    teleLayout->addWidget(m_labelStatus, 0, 1);
    teleLayout->addWidget(m_labelImgQuality, 1, 0);
    teleLayout->addWidget(m_labelSignalQuality, 1, 1);

    mainLayout->addWidget(m_telemetryFrame);

    m_overlayLabel = new QLabel("CONNECTION LOST", m_videoRenderer);
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
    m_overlayLabel->setGeometry(m_videoRenderer->rect());
}

void CameraWidget::mousePressEvent(QMouseEvent* event) {
    Q_UNUSED(event);
    emit clicked(this);
}

void CameraWidget::updateTelemetry(float imgQuality, float signalQuality) {
    m_labelImgQuality->setText(QString("Calidad Imagen: %1%").arg(imgQuality, 0, 'f', 1));
    m_labelSignalQuality->setText(QString("Calidad Señal: %1%").arg(signalQuality, 0, 'f', 1));

}

void CameraWidget::setFrame(const QImage& frame) {
    // Frame reception also counts as a heartbeat
    if (m_connectionLost) {
        setConnectionStatus(true);
    }

    m_videoRenderer->updateFrame(frame);
    m_watchdogTimer->start();
}

void CameraWidget::setConnectionStatus(bool connected) {
    if (connected) {
        m_connectionLost = false;
        m_overlayLabel->setVisible(false);
        m_labelStatus->setText("Estado: CONECTADO");
        m_labelStatus->setStyleSheet("color: #0f0; font-weight: bold;");
    } else {
        handleTimeout();
    }
    update();
}

void CameraWidget::setCompactMode(bool compact) {
    if (compact) {
        m_telemetryFrame->hide();
        layout()->setContentsMargins(0, 0, 0, 0);
        m_overlayLabel->setStyleSheet("background-color: rgba(255, 0, 0, 80); color: white; font-size: 12px; font-weight: bold; border: 1px solid red;");
    } else {
        m_telemetryFrame->show();
        layout()->setContentsMargins(5, 5, 5, 5);
        m_overlayLabel->setStyleSheet("background-color: rgba(255, 0, 0, 80); color: white; font-size: 20px; font-weight: bold; border: 2px solid red;");
    }
}

void CameraWidget::handleTimeout() {
    m_connectionLost = true;
    m_overlayLabel->setGeometry(m_videoRenderer->rect());
    m_overlayLabel->setVisible(true);
    m_labelStatus->setText("Estado: DESCONECTADO");
    m_labelStatus->setStyleSheet("color: red; font-weight: bold;");
    m_labelImgQuality->setText("Calidad Imagen: --%");
    m_labelSignalQuality->setText("Calidad Señal: --%");
    update();
}

// === VideoRenderer Implementation ===

CameraWidget::VideoRenderer::VideoRenderer(QWidget* parent)
    : QOpenGLWidget(parent), m_texture(0), m_hasFrame(false) {
}

void CameraWidget::VideoRenderer::updateFrame(const QImage& frame) {
    QImage rgbFrame = frame.convertToFormat(QImage::Format_RGB888);

    if (rgbFrame.bytesPerLine() != rgbFrame.width() * 3) {
        QImage packedFrame(rgbFrame.width(), rgbFrame.height(), QImage::Format_RGB888);
        const int packedLineSize = packedFrame.width() * 3;
        for (int y = 0; y < rgbFrame.height(); ++y) {
            std::memcpy(packedFrame.scanLine(y), rgbFrame.constScanLine(y), packedLineSize);
        }
        rgbFrame = packedFrame;
    }

    m_currentFrame = rgbFrame;
    m_hasFrame = true;
    update();
}

void CameraWidget::VideoRenderer::initializeGL() {
    initializeOpenGLFunctions();
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glEnable(GL_TEXTURE_2D);
    glGenTextures(1, &m_texture);
    glBindTexture(GL_TEXTURE_2D, m_texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
}

void CameraWidget::VideoRenderer::paintGL() {
    glClear(GL_COLOR_BUFFER_BIT);
    if (!m_hasFrame) return;

    glBindTexture(GL_TEXTURE_2D, m_texture);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, m_currentFrame.width(), m_currentFrame.height(), 0, GL_RGB, GL_UNSIGNED_BYTE, m_currentFrame.bits());

    glBegin(GL_QUADS);
    glTexCoord2f(0, 1); glVertex2f(-1, -1);
    glTexCoord2f(1, 1); glVertex2f(1, -1);
    glTexCoord2f(1, 0); glVertex2f(1, 1);
    glTexCoord2f(0, 0); glVertex2f(-1, 1);
    glEnd();
}

void CameraWidget::VideoRenderer::resizeGL(int w, int h) {
    glViewport(0, 0, w, h);
}
