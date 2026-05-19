#ifndef GRAPH3DWINDOW_H
#define GRAPH3DWINDOW_H

#include "SensorManager.h"
#include "TopToolbar.h"

// Qt Widgets y Layouts
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QTimer>
#include <QPushButton>
#include <QVector3D>
#include <QList>

// Qt Charts
#include <QtCharts/QChart>
#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <QtCharts/QValueAxis>

// Qt Data Visualization
#include <QtDataVisualization/Q3DScatter>
#include <QtDataVisualization/QScatter3DSeries>
#include <QtDataVisualization/QScatterDataProxy>
#include <QtDataVisualization/QScatterDataItem>
#include <QGridLayout>

// Qt 3D
#include <Qt3DCore/QEntity>
#include <Qt3DCore/QTransform>
#include <Qt3DExtras/Qt3DWindow>
#include <QCoreApplication>
#include <Qt3DExtras/QOrbitCameraController>
#include <Qt3DRender/QCamera>

class Graph3DWindow : public QWidget {
    Q_OBJECT

public:
    explicit Graph3DWindow(SensorManager* manager, QWidget* parent = nullptr);

public slots:
    void resetData();

private:
    void aplicarEstiloGrafico(QChart* chart, QValueAxis* axisX, QValueAxis* axisY);
    SensorManager* m_sensorManager = nullptr;
    TopToolbar* m_topToolbar = nullptr;

    // === Layout principal ===
    QGridLayout* mainLayout;

    // === Contenedores de visualización ===
    QWidget* containerGeneral2D;
    QWidget* container3D;
    

    // === Series para gráfica 2D (nuevas variables) ===
    QChart* chartAll;
    QChartView* chartAllView;
    QValueAxis* axisX1;
    QValueAxis* axisY1;
    int xIndex2D = 0;

    QLineSeries* seriesLat;
    QLineSeries* seriesLon;
    QLineSeries* seriesRoll;
    QLineSeries* seriesPitch;
    QLineSeries* seriesYaw;
    QLineSeries* seriesAlt;
    QLineSeries* seriesSats;
    QLineSeries* seriesHDOP;

    // === Gráfico 3D ===
    Q3DScatter* scatterGraph;
    QScatter3DSeries* mainSeries;
    QScatter3DSeries* trailSeries;
    QVector<QVector3D> pointHistory;
    int xIndex3D = 0;

    // === LLMAP ===

    QGraphicsView* llmap;
    QGraphicsScene* llmapScene;
    QGraphicsEllipseItem* mapDot = nullptr;
    QPainterPath trayecto;
    QGraphicsPathItem* pathItem = nullptr;

    // === Series y ejes para gráfica 3D ===
    Qt3DCore::QEntity* rootEntity;
    Qt3DCore::QTransform* rocketTransform;
    Qt3DExtras::Qt3DWindow* view3D;
    QWidget* container3DModel;

    // === Métodos ===
    void setupGeneral2DChart();

    QList<QGraphicsItem*> tooltipTexts;
    QList<QGraphicsEllipseItem*> tooltipDots;
    QList<QGraphicsLineItem*> tooltipLines;
    QGridLayout* contenedorValoresArriba;
    QList<QLabel*> etiquetasValores;

};

#endif // GRAPH3DWINDOW_H
