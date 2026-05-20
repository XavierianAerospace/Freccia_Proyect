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

    QWidget* centralWidget = new QWidget();
    mainLayout = new QGridLayout(centralWidget);
    mainLayout->setSpacing(4);
    mainLayout->setContentsMargins(4, 4, 4, 4);
    globalLayout->addWidget(centralWidget);

    this->setAttribute(Qt::WA_DeleteOnClose);

    // === Cámaras ===
    containerGeneral2D = new QWidget();
    QVBoxLayout* camLayout = new QVBoxLayout(containerGeneral2D);
    camLayout->setContentsMargins(0, 0, 0, 0);
    camLayout->setSpacing(4);

    m_camera1 = new CameraWidget("Cámara Principal", containerGeneral2D);
    m_camera2 = new CameraWidget("Cámara Secundaria", containerGeneral2D);

    camLayout->addWidget(m_camera1);
    camLayout->addWidget(m_camera2);

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
    container3D->setAutoFillBackground(true);
    QPalette pal = container3D->palette();
    pal.setColor(QPalette::Window, Qt::white);
    container3D->setPalette(pal);

    // === Layout general ===
    mainLayout->addWidget(containerGeneral2D, 0, 0);
    mainLayout->addWidget(container3D,         0, 1);

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
