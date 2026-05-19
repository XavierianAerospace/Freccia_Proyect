#ifndef WIDGET_H
#define WIDGET_H

#include "SensorManager.h"
#include "data/FileHelper.h"
#include "WindowManager.h"
#include "TopToolbar.h"

class Graph3DWindow;

#include <QWidget>
#include <QtCharts/QChart>
#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <QtCharts/QValueAxis>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QTimer>
#include <QSlider>
#include <QDateTime>
#include <QVector3D>
#include <QtDataVisualization/Q3DScatter>
#include <QtDataVisualization/QScatter3DSeries>
#include <QtDataVisualization/QScatterDataProxy>
#include <QtDataVisualization/QScatterDataItem>
#include <QMouseEvent>
#include <QGraphicsLineItem>
#include <QGraphicsEllipseItem>
#include <QGraphicsSimpleTextItem>
#include <QPainter>
#include <QBrush>
#include <QColor>
#include <QRectF>

extern Graph3DWindow* ventanaGraph3D;

class HoverChartView : public QChartView {
    Q_OBJECT
public:
    explicit HoverChartView(QWidget* parent=nullptr)
        : QChartView(parent),
          hover_line_(nullptr), hover_point_(nullptr), hover_text_(nullptr),
          m_isPanning(false), m_autoFollow(true), m_alertLevel(0), m_blinkState(false)
    {
        setMouseTracking(true);
        setRenderHint(QPainter::Antialiasing, true);

        m_blinkTimer = new QTimer(this);
        connect(m_blinkTimer, &QTimer::timeout, this, &HoverChartView::onBlinkTimeout);
    }

    void clearHoverElements() {
        if (!scene()) return;
        if (hover_line_)  { scene()->removeItem(hover_line_);  delete hover_line_;  hover_line_  = nullptr; }
        if (hover_point_) { scene()->removeItem(hover_point_); delete hover_point_; hover_point_ = nullptr; }
        if (hover_text_)  { scene()->removeItem(hover_text_);  delete hover_text_;  hover_text_  = nullptr; }
    }

    void setAlertLevel(int level) {
        if (m_alertLevel == level) return;
        m_alertLevel = level;
        m_blinkTimer->stop();

        if (m_alertLevel == 1) { // Warning: Yellow fixed
            updateVisuals(QColor(100, 100, 0));
        } else if (m_alertLevel == 2) { // Critical: Red blinking
            m_blinkTimer->start(500);
            m_blinkState = true;
            updateVisuals(QColor(120, 0, 0));
        } else {
            updateVisuals(Qt::black);
        }
    }

    bool isPanning() const { return m_isPanning; }
    bool autoFollow() const { return m_autoFollow; }
    void setAutoFollow(bool f) { m_autoFollow = f; }

private slots:
    void onBlinkTimeout() {
        m_blinkState = !m_blinkState;
        updateVisuals(m_blinkState ? QColor(120, 0, 0) : Qt::black);
    }

    void updateVisuals(const QColor& color) {
        setBackgroundBrush(QBrush(color));
        if (chart()) chart()->setBackgroundBrush(QBrush(color));
    }

signals:
    void hoverUpdate(QPointF chartPos, QPoint scenePos, bool insidePlot);

protected:
    void mousePressEvent(QMouseEvent* e) override {
        if (e->button() == Qt::LeftButton) {
            m_isPanning = true;
            m_autoFollow = false;
            m_lastMousePos = e->pos();
            setCursor(Qt::ClosedHandCursor);
        }
        QChartView::mousePressEvent(e);
    }

    void mouseReleaseEvent(QMouseEvent* e) override {
        if (e->button() == Qt::LeftButton) {
            m_isPanning = false;
            setCursor(Qt::ArrowCursor);
        }
        QChartView::mouseReleaseEvent(e);
    }

    void mouseDoubleClickEvent(QMouseEvent* e) override {
        m_autoFollow = true;
        QChartView::mouseDoubleClickEvent(e);
    }

    void mouseMoveEvent(QMouseEvent* e) override {
        if (!chart()) { QChartView::mouseMoveEvent(e); return; }

        if (m_isPanning) {
            QPoint delta = e->pos() - m_lastMousePos;
            chart()->scroll(-delta.x(), -delta.y());
            m_lastMousePos = e->pos();
        }

        const QRectF pa = chart()->plotArea();
        const QPoint p  = e->pos();
        emit hoverUpdate(chart()->mapToValue(p), p, pa.contains(p));
        QChartView::mouseMoveEvent(e);
    }
    void leaveEvent(QEvent* e) override {
        emit hoverUpdate(QPointF(), QPoint(), false);
        QChartView::leaveEvent(e);
    }

public:
    // Punteros a los elementos dibujados (línea/punto/tooltip) para
    QGraphicsLineItem*       hover_line_;
    QGraphicsEllipseItem*    hover_point_;
    QGraphicsSimpleTextItem* hover_text_;

private:
    bool m_isPanning;
    bool m_autoFollow;
    QPoint m_lastMousePos;
    int m_alertLevel;
    QTimer* m_blinkTimer;
    bool m_blinkState;
};

class Widget : public QWidget {
    Q_OBJECT

public:
    explicit Widget(SensorManager* manager, QWidget* parent = nullptr);
    ~Widget();

    void abrirVentana3DDesdeExterno();
    void procesarDatos(const SensorData& data);

public slots:
    void abrirDialogoSerial();
    void resetCharts();

private:
    int xIndex = 0;

    // Referencias a SensorManager y Graph3DWindow
    SensorManager* m_sensorManager = nullptr;
    Graph3DWindow* m_graph3DWindow = nullptr;

    // === Menu and Toolbar ===
    TopToolbar* m_topToolbar = nullptr;

    QTimer* timer;
    QTime tiempoInicio;
    bool tiempoIniciado = false;
    QTimer* timeoutTimer;

    // --- contador global de muestras para X ---
    int t_ = 0;

    bool resetTimeBase_ = false; 

    // === Gráficas individuales ===
    QChart *chartRoll, *chartPitch, *chartYaw;
    QChart *chartSats, *chartLat, *chartLon;
    QChart *chartAlt, *chartHdop;
    QChart *chartPresion, *chartTemp;

    // === Series ===
    QLineSeries *seriesRoll, *seriesPitch, *seriesYaw;
    QLineSeries *seriesSats, *seriesLat, *seriesLon;
    QLineSeries *seriesAlt, *seriesHdop;
    QLineSeries *seriesPressure, *seriesTemp;

    // === Vistas ===
    HoverChartView *viewRoll, *viewPitch, *viewYaw;
    HoverChartView *viewSats, *viewLat, *viewLon;
    HoverChartView *viewAlt, *viewHdop;
    HoverChartView *viewPressure, *viewTemp;

    // === Labels ===
    QLabel *labelRoll, *labelPitch, *labelYaw;
    QLabel *labelSats, *labelLat, *labelLon;
    QLabel *labelAlt, *labelHdop;
    QLabel *labelPressure, *labelTemp;

    // === Labels de servo y estado ===
    QLabel* labelServos[6];   // <- ahora 6 servos
    QLabel* labelStatus[8];   // Ampliado para incluir Puerto y Velocidad

    // === Ejes dinámicos para cada gráfica ===
    QValueAxis *axisX_Roll, *axisY_Roll;
    QValueAxis *axisX_Pitch, *axisY_Pitch;
    QValueAxis *axisX_Yaw, *axisY_Yaw;
    QValueAxis *axisX_Sats, *axisY_Sats;
    QValueAxis *axisX_Lat, *axisY_Lat;
    QValueAxis *axisX_Lon, *axisY_Lon;
    QValueAxis *axisX_Alt, *axisY_Alt;
    QValueAxis *axisX_Hdop, *axisY_Hdop;
    QValueAxis *axisX_Pressure, *axisY_Pressure;
    QValueAxis *axisX_Temp, *axisY_Temp;

    // === Pie de estado ===
    QLabel* labelCom = nullptr;
    QLabel* labelBaud = nullptr;
    QLabel* labelRaw = nullptr;

    // === Módulo "ventana de tiempo" ===
    QSlider* winSlider = nullptr;
    QLabel*  winText   = nullptr;
    int      windowSec = 0;

    // Actualizacion de las gráficas con el slider
    QVector<std::pair<QLineSeries*, QValueAxis*>> seriesAndXAxis;

    bool modoArchivo_ = false;
};

#endif // WIDGET_H
