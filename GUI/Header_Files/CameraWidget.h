#ifndef CAMERAWIDGET_H
#define CAMERAWIDGET_H

#include <QWidget>
#include <QLabel>
#include <QTimer>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QFrame>

class CameraWidget : public QWidget {
    Q_OBJECT
public:
    explicit CameraWidget(const QString& cameraName, QWidget* parent = nullptr);
    void updateTelemetry(float imgQuality, float signalQuality); // Heartbeat and data update

protected:
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

private slots:
    void handleTimeout();

private:
    QString m_cameraName;
    QLabel* m_videoPlaceholder;
    QLabel* m_overlayLabel;

    QLabel* m_labelImgQuality;
    QLabel* m_labelSignalQuality;
    QLabel* m_labelStatus;

    QTimer* m_watchdogTimer;
    bool m_connectionLost;

    void setupUI();
};

#endif // CAMERAWIDGET_H
