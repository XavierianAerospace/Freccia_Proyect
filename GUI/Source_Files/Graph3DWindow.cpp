#include "Graph3DWindow.h"
#include "SensorData.h"
#include "data/DataTopic.h"
#include "WindowManager.h"

#include <QVBoxLayout>
#include <QGridLayout>
#include <QChartView>
#include <QtCharts/QLineSeries>
#include <QtCharts/QValueAxis>
#include <QtCharts/QChart>
#include <QtCharts/QChartView>
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

void Graph3DWindow::aplicarEstiloGrafico(QChart* chart, QValueAxis* axisX, QValueAxis* axisY) {
    chart->setBackgroundBrush(QBrush(Qt::black));
    chart->legend()->setLabelColor(Qt::white);
    chart->setTitleBrush(QBrush(Qt::white));

    axisX->setLabelsColor(Qt::white);
    axisX->setTitleBrush(QBrush(Qt::white));
    axisY->setLabelsColor(Qt::white);
    axisY->setTitleBrush(QBrush(Qt::white));
}

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

    // === Gráfica 2D con nuevas variables ===
    QChart *chartAll = new QChart();
    chartAll->setMargins(QMargins(0, 0, 60, 0));
    chartAll->setTitle("Datos Generales");

    // Crear series
    seriesLat   = new QLineSeries(); seriesLat->setName("Latitud");
    seriesLon   = new QLineSeries(); seriesLon->setName("Longitud");
    seriesRoll  = new QLineSeries(); seriesRoll->setName("Roll");
    seriesPitch = new QLineSeries(); seriesPitch->setName("Pitch");
    seriesYaw   = new QLineSeries(); seriesYaw->setName("Yaw");
    seriesAlt   = new QLineSeries(); seriesAlt->setName("AltDiff");
    seriesSats  = new QLineSeries(); seriesSats->setName("Satélites");
    seriesHDOP  = new QLineSeries(); seriesHDOP->setName("HDOP");

    // Colores personalizados
    seriesRoll->setColor(QColor("#1f77b4"));
    seriesPitch->setColor(QColor("#2ca02c"));
    seriesYaw->setColor(QColor("#ff7f0e"));
    seriesAlt->setColor(QColor("#9467bd"));
    seriesSats->setColor(QColor("#8c564b")); 
    seriesHDOP->setColor(QColor("#ff0080"));
    seriesLat->setColor(QColor("#bcbd22"));
    seriesLon->setColor(QColor("#d62728"));

    // Puntos visibles
    for (auto s : {seriesRoll, seriesPitch, seriesYaw, seriesAlt, seriesSats, seriesHDOP, seriesLat, seriesLon}) {
        s->setPointsVisible(true);
        chartAll->addSeries(s);
    }

    // Ejes
    axisX1 = new QValueAxis();
    axisX1->setTitleText("Tiempo");
    axisX1->setRange(0, 1);

    axisY1 = new QValueAxis();
    axisY1->setTitleText("Valor");
    axisY1->setRange(0, 1);

    chartAll->addAxis(axisX1, Qt::AlignBottom);
    chartAll->addAxis(axisY1, Qt::AlignLeft);

    for (auto s : {seriesRoll, seriesPitch, seriesYaw, seriesAlt, seriesSats, seriesHDOP, seriesLat, seriesLon})
        s->attachAxis(axisX1), s->attachAxis(axisY1);

    // Aplicar estilo
    aplicarEstiloGrafico(chartAll, axisX1, axisY1);

    // Vista
    chartAllView = new QChartView(chartAll);
    chartAllView->setMinimumSize(1000, 600);
    chartAllView->setContentsMargins(0, 0, 40, 0);

    // Layout contenedor
    auto* layout2DConValores = new QVBoxLayout();
    contenedorValoresArriba = new QGridLayout();
    layout2DConValores->addLayout(contenedorValoresArriba);
    layout2DConValores->addWidget(chartAllView);

    // Contenedor general 2D
    auto* widget2D = new QWidget();
    widget2D->setLayout(layout2DConValores);
    containerGeneral2D = widget2D;

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
        seriesLat->append(xIndex2D, d.latitude);
        seriesLon->append(xIndex2D, d.longitude);
        seriesRoll->append(xIndex2D, d.Roll);
        seriesPitch->append(xIndex2D, d.Pitch);
        seriesYaw->append(xIndex2D, d.Yaw);
        seriesAlt->append(xIndex2D, d.AltDiff);
        seriesSats->append(xIndex2D, d.satellites);
        seriesHDOP->append(xIndex2D, d.hdop);

        if (xIndex2D > axisX1->max()) {
            axisX1->setMax(xIndex2D);
        }
        if (xIndex2D < axisX1->min()) {
            axisX1->setMin(xIndex2D);
        }

        double nuevoMaxY = std::numeric_limits<double>::lowest();
        double nuevoMinY = std::numeric_limits<double>::max();

        for (const auto& serie : {seriesRoll, seriesPitch, seriesYaw, seriesAlt, seriesSats, seriesHDOP, seriesLat, seriesLon}) {
            for (const QPointF& punto : serie->points()) {
                if (punto.y() > nuevoMaxY) nuevoMaxY = punto.y();
                if (punto.y() < nuevoMinY) nuevoMinY = punto.y();
            }
        }

        double margen = (nuevoMaxY - nuevoMinY) * 0.1;
        axisY1->setRange(nuevoMinY - margen, nuevoMaxY + margen);

        ++xIndex2D;

        for (auto item : tooltipTexts) delete item;
        for (auto line : tooltipLines) delete line;
        tooltipTexts.clear();
        tooltipLines.clear();

        int lastIndex = seriesRoll->count() - 1;
        QList<QRectF> ocupados;

        for (auto serie : {seriesRoll, seriesPitch, seriesYaw, seriesAlt, seriesSats, seriesHDOP, seriesLat, seriesLon}) {
            if (serie->count() > 0) {
                serie->setPointsVisible(true);
                QPointF punto = serie->at(lastIndex);

                QString unidad;
                if (serie == seriesAlt)          unidad = " m";
                else if (serie == seriesHDOP)    unidad = "";
                else if (serie == seriesSats)    unidad = " sat";
                else if (serie == seriesRoll ||
                        serie == seriesPitch ||
                        serie == seriesYaw)     unidad = "°";
                else if (serie == seriesLat ||
                        serie == seriesLon)     unidad = "°";

                QString texto = QString("%1%2").arg(QString::number(punto.y(), 'f', 2)).arg(unidad);

                QGraphicsTextItem* etiqueta = new QGraphicsTextItem(texto);
                etiqueta->setFont(QFont("Arial", 10, QFont::Bold));
                etiqueta->setDefaultTextColor(Qt::white);
                QRectF textRect = etiqueta->boundingRect();

                QGraphicsRectItem* fondo = new QGraphicsRectItem(textRect.adjusted(-6, -4, 6, 4));
                fondo->setBrush(Qt::black);
                fondo->setPen(QPen(serie->color(), 1));
                fondo->setZValue(0);

                QPointF puntoGraf = chartAllView->chart()->mapToPosition(punto, serie);
                QPointF posEtiqueta = puntoGraf + QPointF(10, -textRect.height() - 6);

                QRectF nuevaCaja(posEtiqueta, fondo->rect().size());

                for (const QRectF& ocupado : ocupados) {
                    while (nuevaCaja.intersects(ocupado)) {
                        posEtiqueta.ry() -= 20;
                        nuevaCaja.moveTopLeft(posEtiqueta);
                    }
                }
                ocupados.append(nuevaCaja);

                fondo->setPos(posEtiqueta);
                etiqueta->setPos(posEtiqueta);

                QPointF centroEtiqueta = posEtiqueta + QPointF(fondo->rect().width() / 2, fondo->rect().height() / 2);
                QGraphicsLineItem* linea = new QGraphicsLineItem(QLineF(puntoGraf, centroEtiqueta));
                linea->setPen(QPen(serie->color(), 1, Qt::DashLine));
                linea->setZValue(-1);

                chartAllView->scene()->addItem(fondo);
                chartAllView->scene()->addItem(etiqueta);
                chartAllView->scene()->addItem(linea);

                tooltipTexts.append(fondo);
                tooltipTexts.append(etiqueta);
                tooltipLines.append(linea);
            }
        }

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
    QChart* oldChart = chartAllView ? chartAllView->chart() : nullptr;
    if (oldChart) {
        const auto seriesList = oldChart->series();
        for (QAbstractSeries* s : seriesList) {
            oldChart->removeSeries(s);
            delete s;
        }
        const auto axesList = oldChart->axes();
        for (QAbstractAxis* ax : axesList) {
            oldChart->removeAxis(ax);
            delete ax;
        }
        delete oldChart;
    }

    QChart* newChart = new QChart();
    newChart->setMargins(QMargins(0, 0, 60, 0));
    newChart->setTitle("Datos Generales");
    chartAllView->setChart(newChart);

    axisX1 = new QValueAxis();
    axisY1 = new QValueAxis();
    axisX1->setTitleText("Tiempo");
    axisY1->setTitleText("Valor");
    axisX1->setRange(0.0, 1.0);
    axisY1->setRange(0.0, 1.0);
    newChart->addAxis(axisX1, Qt::AlignBottom);
    newChart->addAxis(axisY1, Qt::AlignLeft);

    seriesLat   = new QLineSeries(); seriesLat->setName("Latitud");    seriesLat->setColor(QColor("#bcbd22"));
    seriesLon   = new QLineSeries(); seriesLon->setName("Longitud");   seriesLon->setColor(QColor("#d62728"));
    seriesRoll  = new QLineSeries(); seriesRoll->setName("Roll");      seriesRoll->setColor(QColor("#1f77b4"));
    seriesPitch = new QLineSeries(); seriesPitch->setName("Pitch");    seriesPitch->setColor(QColor("#2ca02c"));
    seriesYaw   = new QLineSeries(); seriesYaw->setName("Yaw");        seriesYaw->setColor(QColor("#ff7f0e"));
    seriesAlt   = new QLineSeries(); seriesAlt->setName("AltDiff");    seriesAlt->setColor(QColor("#9467bd"));
    seriesSats  = new QLineSeries(); seriesSats->setName("Satélites"); seriesSats->setColor(QColor("#8c564b"));
    seriesHDOP  = new QLineSeries(); seriesHDOP->setName("HDOP");      seriesHDOP->setColor(QColor("#ff0080"));

    for (auto s : {seriesRoll, seriesPitch, seriesYaw, seriesAlt,
                   seriesSats, seriesHDOP, seriesLat, seriesLon}) {
        s->setPointsVisible(true);
        newChart->addSeries(s);
        s->attachAxis(axisX1);
        s->attachAxis(axisY1);
    }

    aplicarEstiloGrafico(newChart, axisX1, axisY1);

    xIndex2D = 0;

    for (auto* it : tooltipTexts) delete it;
    tooltipTexts.clear();
    for (auto* it : tooltipLines) delete it;
    tooltipLines.clear();
    tooltipDots.clear();

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
