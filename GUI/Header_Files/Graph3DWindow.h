#ifndef GRAPH3DWINDOW_H
#define GRAPH3DWINDOW_H

#include "SensorManager.h"
#include "TopToolbar.h"
#include "CameraWidget.h"
#include "VideoManager.h"
#include "VideoDecoder.h"

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
    SensorManager* m_sensorManager = nullptr;
    TopToolbar* m_topToolbar = nullptr;
    CameraWidget* m_camera1 = nullptr;
    CameraWidget* m_camera2 = nullptr;
    VideoManager* m_videoManager = nullptr;
    VideoDecoder* m_videoDecoder = nullptr;

    // === Layout principal ===
    QGridLayout* mainLayout;

    // === Contenedores de visualización ===
    QWidget* containerGeneral2D;
    QWidget* container3D;
    

    // === Gráfico 3D ===
    Q3DScatter* scatterGraph;
    QScatter3DSeries* mainSeries;
    QScatter3DSeries* trailSeries;
    QVector<QVector3D> pointHistory;
    int xIndex3D = 0;


    // === Series y ejes para gráfica 3D ===
    Qt3DCore::QEntity* rootEntity;
    Qt3DCore::QTransform* rocketTransform;
    Qt3DExtras::Qt3DWindow* view3D;
    QWidget* container3DModel;

    // === Métodos ===


};

#endif // GRAPH3DWINDOW_H
