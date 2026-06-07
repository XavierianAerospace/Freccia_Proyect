#include "Graph3DWindow.h"
#include "SensorData.h"
#include "data/DataTopic.h"
#include "WindowManager.h"
#include "VideoSubsystem.h"

#include <QVBoxLayout>
#include <cstdlib>
#include <QGridLayout>
#include <QtDataVisualization/Q3DTheme>
#include <QtDataVisualization/Q3DScatter>
#include <QtDataVisualization/QScatter3DSeries>
#include <QtDataVisualization/QScatterDataProxy>
#include <QtDataVisualization/QScatterDataItem>

#include <Qt3DExtras/Qt3DWindow>
#include <Qt3DCore/QEntity>
#include <Qt3DExtras/QOrbitCameraController>
#include <Qt3DExtras/QPhongMaterial>
#include <Qt3DRender/QCamera>
#include <Qt3DRender/QMesh>
#include <Qt3DExtras/QForwardRenderer>
#include <Qt3DExtras/QPhongAlphaMaterial>
#include <Qt3DCore/QTransform>
#include <Qt3DRender/QPointLight>

Graph3DWindow::Graph3DWindow(SensorManager* manager, QWidget* parent)
    : QWidget(parent), m_sensorManager(manager) {
    setWindowIcon(QIcon("./assets/logo_xae.png"));
    setWindowTitle("FRECCIA_XAE - Gráficas 3D y OSM");
    WindowManager::instance()->registerGraph3D(this);

    QVBoxLayout* globalLayout = new QVBoxLayout(this);
    globalLayout->setContentsMargins(0, 0, 0, 0);
    globalLayout->setSpacing(0);

    m_topToolbar = new TopToolbar(this);
    globalLayout->addWidget(m_topToolbar);

    m_centralWidget = new QWidget();
    mainLayout = new QGridLayout(m_centralWidget);
    mainLayout->setSpacing(4);
    mainLayout->setContentsMargins(4, 4, 4, 4);
    globalLayout->addWidget(m_centralWidget);

    this->setAttribute(Qt::WA_DeleteOnClose);

    // === Cámaras ===
    containerGeneral2D = new QWidget();
    m_camLayout = new QGridLayout(containerGeneral2D);
    m_camLayout->setContentsMargins(0, 0, 0, 0);
    m_camLayout->setSpacing(4);

    m_camera1 = new CameraWidget("Cámara Principal", containerGeneral2D);
    m_camera2 = new CameraWidget("Cámara Secundaria", containerGeneral2D);

    m_camLayout->addWidget(m_camera1, 0, 0);
    m_camLayout->addWidget(m_camera2, 1, 0);

    m_cameras << m_camera1 << m_camera2;

    for (CameraWidget* cam : m_cameras) {
        connect(cam, &CameraWidget::clicked, this, &Graph3DWindow::toggleCameraMaximize);
    }

    // === Conexión a VideoSubsystem central ===
    connect(VideoSubsystem::instance(), &VideoSubsystem::frameReady, this, [=](int camId, const QImage& frame) {
        if (camId == 1 && m_camera1) m_camera1->setFrame(frame);
        if (camId == 2 && m_camera2) m_camera2->setFrame(frame);
    });

    // Iniciar canales independientes
    VideoSubsystem::instance()->start(1, 5600);
    VideoSubsystem::instance()->start(2, 5601);

    // === Gráfico 3D ===
    scatterGraph = new Q3DScatter();
    auto theme = scatterGraph->activeTheme();
    theme->setType(Q3DTheme::ThemeRetro);
    theme->setBackgroundColor(Qt::black);
    theme->setLabelTextColor(Qt::white);
    theme->setGridLineColor(Qt::gray);
    theme->setColorStyle(Q3DTheme::ColorStyleUniform);

    mainSeries = new QScatter3DSeries();
    trailSeries = new QScatter3DSeries();
    mainSeries->setItemSize(0.2f);
    trailSeries->setItemSize(0.5f);
    mainSeries->setBaseColor(Qt::blue);
    trailSeries->setBaseColor(Qt::red);

    scatterGraph->addSeries(mainSeries);
    scatterGraph->addSeries(trailSeries);

    scatterGraph->axisX()->setTitle("Longitud");
    scatterGraph->axisY()->setTitle("Tiempo");
    scatterGraph->axisZ()->setTitle("Latitud");
    scatterGraph->axisX()->setTitleVisible(true);
    scatterGraph->axisY()->setTitleVisible(true);
    scatterGraph->axisZ()->setTitleVisible(true);

    container3D = QWidget::createWindowContainer(scatterGraph);
    container3D->setMinimumSize(QSize(460, 500));
    container3D->installEventFilter(this);
    container3D->setAutoFillBackground(true);
    QPalette pal = container3D->palette();
    pal.setColor(QPalette::Window, Qt::white);
    container3D->setPalette(pal);

    // === Layout general ===
    mainLayout->addWidget(containerGeneral2D, 0, 0);
    mainLayout->addWidget(container3D,         0, 1);
    mainLayout->setColumnStretch(0, 1);
    mainLayout->setColumnStretch(1, 1);

    // === Suscripción a DataTopic ===
    connect(DataTopic::instance(), &DataTopic::dataPublished, this, [=](const QString& line) {
        SensorData d = SensorData::deserialize(line);

        // Actualizar cámaras (mock telemetry based on altitude for variety)
        float mockQual = 95.0f + (rand() % 50) / 10.0f;
        if (m_camera1) m_camera1->updateTelemetry(mockQual, 98.2f);
        if (m_camera2) m_camera2->updateTelemetry(mockQual - 2.0f, 96.5f);

        QVector3D pos(d.longitude, xIndex3D, d.latitude);
        pointHistory.append(pos);

        const int N = 2;
        int startTrail = std::max(0, int(pointHistory.size() - N));

        QScatterDataArray* mainArray = new QScatterDataArray();
        mainArray->resize(startTrail);
        for (int i = 0; i < startTrail; ++i)
            (*mainArray)[i].setPosition(pointHistory[i]);
        mainSeries->dataProxy()->resetArray(mainArray);

        QScatterDataArray* trailArray = new QScatterDataArray();
        trailArray->resize(pointHistory.size() - startTrail);
        for (int i = startTrail; i < pointHistory.size(); ++i) {
            float t = float(i - startTrail) / float(N - 1);
            QColor c; c.setRgbF(t, 0.0, 1.0 - t);
            QScatterDataItem item;
            item.setPosition(pointHistory[i]);
            trailSeries->setBaseColor(c);
            (*trailArray)[i - startTrail] = item;
        }
        trailSeries->dataProxy()->resetArray(trailArray);

        xIndex3D++;
    });

    connect(WindowManager::instance(), &WindowManager::dataResetRequested, this, &Graph3DWindow::resetData);
}

void Graph3DWindow::resetData()
{
    pointHistory.clear();
    xIndex3D = 0;

    if (mainSeries) { scatterGraph->removeSeries(mainSeries); delete mainSeries; mainSeries = nullptr; }
    if (trailSeries){ scatterGraph->removeSeries(trailSeries); delete trailSeries; trailSeries = nullptr; }

    mainSeries  = new QScatter3DSeries();
    trailSeries = new QScatter3DSeries();
    mainSeries->setItemSize(0.2f);
    trailSeries->setItemSize(0.5f);
    mainSeries->setBaseColor(Qt::blue);
    trailSeries->setBaseColor(Qt::red);

    scatterGraph->addSeries(mainSeries);
    scatterGraph->addSeries(trailSeries);

    mainSeries->dataProxy()->resetArray(new QScatterDataArray());
    trailSeries->dataProxy()->resetArray(new QScatterDataArray());

    if (scatterGraph) {
        scatterGraph->axisX()->setAutoAdjustRange(true);
        scatterGraph->axisY()->setAutoAdjustRange(true);
        scatterGraph->axisZ()->setAutoAdjustRange(true);
    }
}

bool Graph3DWindow::eventFilter(QObject* watched, QEvent* event) {
    if (watched == container3D && event->type() == QEvent::MouseButtonDblClick) {
        if (m_maximizeMode == Graph3D) m_maximizeMode = Grid;
        else m_maximizeMode = Graph3D;
        applyLayout();
        return true;
    }
    return QWidget::eventFilter(watched, event);
}

void Graph3DWindow::toggleCameraMaximize(CameraWidget* clickedCamera) {
    if (clickedCamera == m_camera1) {
        if (m_maximizeMode == Cam1) m_maximizeMode = Grid;
        else m_maximizeMode = Cam1;
    } else if (clickedCamera == m_camera2) {
        if (m_maximizeMode == Cam2) m_maximizeMode = Grid;
        else m_maximizeMode = Cam2;
    }
    applyLayout();
}

void Graph3DWindow::applyLayout() {
    // 1. Clear everything from layouts
    mainLayout->removeWidget(containerGeneral2D);
    mainLayout->removeWidget(container3D);
    m_camLayout->removeWidget(m_camera1);
    m_camLayout->removeWidget(m_camera2);

    // 2. Reset visibility and size constraints
    containerGeneral2D->show();
    containerGeneral2D->setMinimumSize(0, 0);
    containerGeneral2D->setMaximumSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);

    container3D->show();
    for (CameraWidget* cam : m_cameras) {
        cam->show();
        cam->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        cam->setMinimumSize(0, 0);
        cam->setMaximumSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);
    }
    container3D->setMinimumSize(0, 0);
    container3D->setMaximumSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);

    mainLayout->setColumnStretch(0, 1);
    mainLayout->setColumnStretch(1, 1);

    // 3. Apply layout based on mode
    switch (m_maximizeMode) {
        case Grid:
            mainLayout->addWidget(containerGeneral2D, 0, 0);
            mainLayout->addWidget(container3D, 0, 1);
            m_camLayout->addWidget(m_camera1, 0, 0);
            m_camLayout->addWidget(m_camera2, 1, 0);
            container3D->setMinimumSize(460, 500);
            break;

        case Cam1:
        case Cam2: {
            CameraWidget* mainCam = (m_maximizeMode == Cam1) ? m_camera1 : m_camera2;
            CameraWidget* pipCam = (m_maximizeMode == Cam1) ? m_camera2 : m_camera1;

            // Standard columns: Cameras (left), 3D Graph (right)
            mainLayout->addWidget(containerGeneral2D, 0, 0);
            mainLayout->addWidget(container3D, 0, 1);
            mainLayout->setColumnStretch(0, 2); // Give more space to cameras
            mainLayout->setColumnStretch(1, 1);

            // Main camera fills its container
            m_camLayout->addWidget(mainCam, 0, 0);

            // Other camera as PiP (Top Right of the camera container)
            m_camLayout->addWidget(pipCam, 0, 0, Qt::AlignTop | Qt::AlignRight);
            pipCam->setFixedSize(240, 180);
            pipCam->raise();

            container3D->setMinimumSize(460, 500);
            break;
        }

        case Graph3D:
            // 3D Graph spans the whole window area
            mainLayout->addWidget(container3D, 0, 0, 1, 2);
            mainLayout->setColumnStretch(0, 0);
            mainLayout->setColumnStretch(1, 1);

            // Cameras as PiP overlay on top of 3D graph
            mainLayout->addWidget(containerGeneral2D, 0, 0, 1, 2, Qt::AlignTop | Qt::AlignRight);
            containerGeneral2D->setFixedSize(240, 360);
            m_camLayout->addWidget(m_camera1, 0, 0);
            m_camLayout->addWidget(m_camera2, 1, 0);
            containerGeneral2D->raise();
            break;
    }
}
