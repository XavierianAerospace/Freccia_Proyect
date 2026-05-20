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

    m_videoRenderer = new VideoRenderer(this);
    m_videoRenderer->setMinimumHeight(200);
    m_videoRenderer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    mainLayout->addWidget(m_videoRenderer);

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

void CameraWidget::updateTelemetry(float imgQuality, float signalQuality) {
    if (m_connectionLost) {
        m_connectionLost = false;
        m_overlayLabel->setVisible(false);
        m_labelStatus->setText("Estado: CONECTADO");
        m_labelStatus->setStyleSheet("color: #0f0; font-weight: bold;");
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
    m_currentFrame = frame;
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
