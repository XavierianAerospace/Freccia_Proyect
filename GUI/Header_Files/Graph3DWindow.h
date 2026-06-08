#ifndef GRAPH3DWINDOW_H
#define GRAPH3DWINDOW_H

#include "SensorManager.h"
#include "TopToolbar.h"
#include "CameraWidget.h"

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

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    enum MaximizeMode { Grid, Cam1, Cam2, Cam3, Cam4, Graph3D };
    MaximizeMode m_maximizeMode = Grid;

    SensorManager* m_sensorManager = nullptr;
    TopToolbar* m_topToolbar = nullptr;
    CameraWidget* m_camera1 = nullptr;
    CameraWidget* m_camera2 = nullptr;
    CameraWidget* m_camera3 = nullptr;
    CameraWidget* m_camera4 = nullptr;
    QList<CameraWidget*> m_cameras;
    QGridLayout* m_camLayout = nullptr;

    // === Layout principal ===
    QGridLayout* mainLayout;
    QWidget* m_centralWidget;

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
    void applyLayout();

private slots:
    void toggleCameraMaximize(CameraWidget* clickedCamera);

};

#endif // GRAPH3DWINDOW_H
