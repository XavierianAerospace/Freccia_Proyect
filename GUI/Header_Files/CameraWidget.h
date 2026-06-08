#ifndef CAMERAWIDGET_H
#define CAMERAWIDGET_H

#include <QWidget>
#include <QLabel>
#include <QTimer>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QFrame>
#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QMouseEvent>

class CameraWidget : public QWidget {
    Q_OBJECT
public:
    explicit CameraWidget(const QString& cameraName, QWidget* parent = nullptr);
    void updateTelemetry(float imgQuality, float signalQuality); // Heartbeat and data update
    void setFrame(const QImage& frame);
    void setConnectionStatus(bool connected);
    void setCompactMode(bool compact);

signals:
    void clicked(CameraWidget* widget);

protected:
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;

private slots:
    void handleTimeout();

private:
    class VideoRenderer : public QOpenGLWidget, protected QOpenGLFunctions {
    public:
        VideoRenderer(QWidget* parent = nullptr);
        void updateFrame(const QImage& frame);
    protected:
        void initializeGL() override;
        void paintGL() override;
        void resizeGL(int w, int h) override;
    private:
        QImage m_currentFrame;
        GLuint m_texture;
        bool m_hasFrame;
    };

    QString m_cameraName;
    VideoRenderer* m_videoRenderer;
    QLabel* m_overlayLabel;
    QWidget* m_telemetryFrame;

    QLabel* m_labelImgQuality;
    QLabel* m_labelSignalQuality;
    QLabel* m_labelStatus;

    QTimer* m_watchdogTimer;
    bool m_connectionLost;

    void setupUI();
};

#endif // CAMERAWIDGET_H
