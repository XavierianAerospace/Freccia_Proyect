#include "widget.h"
#include "Graph3DWindow.h"
#include "data/FileHelper.h"
#include "data/RangeChecker.h"
#include "data/DataTopic.h"
#include "WindowManager.h"
#include "TopToolbar.h"

#include <QSlider>
#include <cmath>
#include <algorithm> 
#include <QGraphicsSimpleTextItem>
#include <QGraphicsTextItem>
#include <QGraphicsEllipseItem>
#include <QGraphicsLineItem>
#include <QMenu>
#include <QWidgetAction>
#include <QPushButton>
#include <QMessageBox>
#include <QGridLayout>
#include <QLabel>
#include <QChartView>
#include <QDateTime>
#include <QTimer>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QtCharts/QValueAxis>
#include <QtCharts/QLineSeries>
#include <QtCharts/QChart>
#include <QDialog>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QGraphicsLayout>
#include <QtSerialPort/QSerialPortInfo>
#include <QButtonGroup>
#include <QFileDialog>
#include <QFileInfo>
#include <QFile>
#include <QTextStream>

static Widget* ventanaUnica = nullptr;

Widget::Widget(SensorManager* manager, QWidget* parent)
    : QWidget(parent), m_sensorManager(manager) {
    
    // === Verificación de instancia única ===
    if (ventanaUnica && ventanaUnica != this) {
        close();
        return;
    }
    ventanaUnica = this;
    WindowManager::instance()->registerWidget(this);

    for (int i = 0; i < 8; ++i) labelStatus[i] = nullptr;
    for (int i = 0; i < 6; ++i) labelServos[i] = nullptr;

    setWindowTitle("FRECCIA_XAE - Gráficas 2D");
    setStyleSheet("background-color: black;");
    QGridLayout* layout = new QGridLayout();
    layout->setSpacing(0);
    layout->setContentsMargins(0, 0, 0, 0);

    for (int col = 0; col < 4; ++col) layout->setColumnStretch(col, 1);
    for (int row = 0; row < 3; ++row) layout->setRowStretch(row, 1);
    layout->setRowStretch(3, 0); // Fila compacta para el panel de estado inferior

    setWindowIcon(QIcon("./assets/logo_xae.png"));

    // === Menu and Toolbar ===
    m_topToolbar = new TopToolbar(this);

    // === Añadir barra al layout principal ===
    QVBoxLayout* globalLayout = new QVBoxLayout(this);
    globalLayout->setContentsMargins(0, 0, 0, 0);
    globalLayout->setSpacing(0);
    globalLayout->addWidget(m_topToolbar);
    globalLayout->addLayout(layout);

    auto crearGrafica = [&](QChart*& chart,
                        QLineSeries*& series,
                        HoverChartView*& view,
                        QLabel*& label,
                        QValueAxis*& ejeX,
                        QValueAxis*& ejeY,
                        const QString& nombre,
                        const QString& yTitulo,
                        const QString& unidad,
                        int fila,
                        int columna)
    {
        static const QVector<QColor> colores = {
            Qt::cyan, Qt::magenta, Qt::green,  Qt::yellow,
            Qt::red,  Qt::blue,    Qt::gray,   Qt::darkCyan,
            Qt::darkMagenta, Qt::darkYellow, Qt::darkRed, Qt::darkBlue
        };
        static int colorIndex = 0;

        // --- Chart y serie ---
        chart  = new QChart();
        series = new QLineSeries();

        // 1) nombre base
        series->setObjectName(nombre);
        // 2) unidad como propiedad
        series->setProperty("tipoDato", unidad);
        // 3) texto inicial
        series->setName(QString("%1: %2 %3").arg(nombre).arg(0).arg(unidad));
        // 4) color
        series->setColor(colores[colores.isEmpty() ? 0 : (colorIndex++ % colores.size())]);

        chart->addSeries(series);

        // --- Ejes ---
        ejeX = new QValueAxis();
        ejeY = new QValueAxis();
        ejeX->setTitleText("Tiempo (s)");
        ejeY->setTitleText(QString("%1 (%2)").arg(yTitulo, unidad));
        ejeX->setLabelsColor(Qt::white);
        ejeY->setLabelsColor(Qt::white);
        ejeX->setTitleBrush(QBrush(Qt::white));
        ejeY->setTitleBrush(QBrush(Qt::white));
        ejeX->setRange(0, 1);
        ejeY->setRange(0, 1);

        chart->addAxis(ejeX, Qt::AlignBottom);
        chart->addAxis(ejeY, Qt::AlignLeft);
        series->attachAxis(ejeX);
        series->attachAxis(ejeY);

        chart->setTitle("");
        chart->setTitleBrush(QBrush(Qt::white));
        chart->legend()->setLabelColor(Qt::white);
        chart->setBackgroundBrush(QBrush(Qt::black));
        chart->setAnimationOptions(QChart::SeriesAnimations);
        chart->setMargins(QMargins(0, 0, 0, 0));
        chart->layout()->setContentsMargins(0, 0, 0, 0);

       // --- View con soporte de hover ---
        auto* hview = new HoverChartView();
        hview->setRenderHint(QPainter::Antialiasing, true);
        hview->setMinimumSize(150, 150);
        hview->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        hview->setChart(chart);
        view = hview;
        layout->addWidget(hview, fila, columna);

        // *** CREA EL LABEL ANTES DE LAS LAMBDAS ***
        label = new QLabel(nombre + ": 0");
        label->setStyleSheet("color: white; font-weight: bold;");

        // --- Overlay (línea, punto, tooltip) por cada gráfica ---
        struct Overlay {
            QGraphicsLineItem*    line = nullptr;
            QGraphicsEllipseItem* dot  = nullptr;
            QGraphicsTextItem*    text = nullptr;
            bool   inside  = false;
            double lastX   = 0.0;
        };
        auto overlay = std::make_shared<Overlay>();

        auto clearOverlay = [hview, overlay]() {
            if (!hview->scene()) return;
            if (overlay->line) { hview->scene()->removeItem(overlay->line); delete overlay->line; overlay->line=nullptr; }
            if (overlay->dot)  { hview->scene()->removeItem(overlay->dot);  delete overlay->dot;  overlay->dot =nullptr; }
            if (overlay->text) { hview->scene()->removeItem(overlay->text); delete overlay->text; overlay->text=nullptr; }
        };

        // Búsqueda rápida del punto más cercano por X
        auto nearestByX = [series](double x)->QPointF {
            const auto pts = series->points();
            if (pts.isEmpty()) return QPointF(x, 0);
            int lo = 0, hi = pts.size()-1, idx = 0;
            while (lo <= hi) {
                int m = (lo+hi)/2;
                if (pts[m].x() < x) lo = m+1;
                else { idx = m; hi = m-1; }
            }
            if (idx > 0 && std::abs(pts[idx-1].x()-x) < std::abs(pts[idx].x()-x)) idx--;
            return pts[idx];
        };

        auto drawOverlayNow = [hview, chart, series, unidad, label, overlay, clearOverlay, nearestByX]() {
            if (!overlay->inside) { clearOverlay(); return; }
            if (!chart || !series || series->count()==0) { clearOverlay(); return; }

            const QPointF p = nearestByX(overlay->lastX);
            const QPointF scene = chart->mapToPosition(p, series);
            if (!chart->plotArea().contains(scene)) { clearOverlay(); return; }

            clearOverlay();

            overlay->line = hview->scene()->addLine(
                scene.x(), chart->plotArea().top(),
                scene.x(), chart->plotArea().bottom(),
                QPen(Qt::white, 1, Qt::DashLine));

            overlay->dot = hview->scene()->addEllipse(
                scene.x()-4, scene.y()-4, 8, 8,
                QPen(series->color(), 2),
                QBrush(series->color()));

            QString txt = QString("t: %1 s\nv: %2 %3")
                            .arg(p.x(), 0, 'f', 2)
                            .arg(p.y(), 0, 'f', 3)
                            .arg(unidad);

            overlay->text = hview->scene()->addText(txt);
            overlay->text->setDefaultTextColor(Qt::white);
            QFont f = overlay->text->font(); f.setBold(true); f.setPointSize(9);
            overlay->text->setFont(f);

            // Actualiza leyenda y label con el valor bajo el mouse
            const QString nombreSerie = series->objectName();
            const QString textoHover  = QString("%1: %2 %3")
                                        .arg(nombreSerie).arg(p.y(), 0, 'f', 3).arg(unidad);
            series->setName(textoHover);
            if (label) label->setText(textoHover);

            // Mantén el tooltip dentro del plot
            const QRectF pr = chart->plotArea();
            const QRectF tr = overlay->text->boundingRect();
            QPointF tp(scene.x()+12, scene.y()-tr.height()-6);
            if (tp.x()+tr.width() > pr.right())   tp.setX(scene.x()-tr.width()-12);
            if (tp.y() < pr.top())                tp.setY(scene.y()+12);
            if (tp.y()+tr.height() > pr.bottom()) tp.setY(pr.bottom()-tr.height()-6);
            overlay->text->setPos(tp);
        };

        QObject::connect(hview, &HoverChartView::hoverUpdate, hview,
            [overlay, clearOverlay, drawOverlayNow](QPointF chartPos, QPoint, bool inside) {
                overlay->inside = inside;
                if (!inside) { clearOverlay(); return; }
                overlay->lastX = chartPos.x();
                drawOverlayNow();
            });

        QObject::connect(series, &QLineSeries::pointAdded,    hview, [drawOverlayNow](int){ drawOverlayNow(); });
        QObject::connect(series, &QLineSeries::pointRemoved,  hview, [drawOverlayNow](int){ drawOverlayNow(); });
        QObject::connect(series, &QLineSeries::pointReplaced, hview, [drawOverlayNow](int){ drawOverlayNow(); });
        QObject::connect(ejeX,   &QValueAxis::rangeChanged,   hview, [drawOverlayNow](qreal, qreal){ drawOverlayNow(); });
        QObject::connect(ejeY,   &QValueAxis::rangeChanged,   hview, [drawOverlayNow](qreal, qreal){ drawOverlayNow(); });

        // Registrar para gestión de ventana de tiempo
        seriesAndXAxis.push_back({series, ejeX});
    };

    // Fila 0
    crearGrafica(chartRoll, seriesRoll, viewRoll, labelRoll, axisX_Roll, axisY_Roll, "Roll", "Ángulo", "°", 0, 0);
    crearGrafica(chartPitch, seriesPitch, viewPitch, labelPitch, axisX_Pitch, axisY_Pitch, "Pitch", "Ángulo", "°", 0, 1);
    crearGrafica(chartYaw, seriesYaw, viewYaw, labelYaw, axisX_Yaw, axisY_Yaw, "Yaw", "Ángulo", "°", 0, 2);
    crearGrafica(chartSats, seriesSats, viewSats, labelSats, axisX_Sats, axisY_Sats, "Satélites", "Conexiones", "Conexiones", 0, 3);

    // Fila 1
    crearGrafica(chartLat, seriesLat, viewLat, labelLat, axisX_Lat, axisY_Lat, "Latitud", "Grados", "°", 1, 0);
    crearGrafica(chartLon, seriesLon, viewLon, labelLon, axisX_Lon, axisY_Lon, "Longitud", "Grados", "°", 1, 1);
    crearGrafica(chartAlt, seriesAlt, viewAlt, labelAlt, axisX_Alt, axisY_Alt, "AltDiff", "Metros", "m", 1, 2);
    crearGrafica(chartHdop, seriesHdop, viewHdop, labelHdop, axisX_Hdop, axisY_Hdop, "HDOP", "Precisión", "°", 1, 3);

    // Fila 2
    crearGrafica(chartPresion, seriesPressure, viewPressure, labelPressure, axisX_Pressure, axisY_Pressure, "Presión", "Presión", "hPa", 2, 0);
    crearGrafica(chartTemp, seriesTemp, viewTemp, labelTemp, axisX_Temp, axisY_Temp, "Temperatura", "Temperatura", "°C", 2, 1);

    // === SERVOS (2,2) ahora 3x2 para 6 servos ===
    QGridLayout* gridServos = new QGridLayout();
    gridServos->setContentsMargins(8, 8, 8, 8);
    gridServos->setHorizontalSpacing(24);
    gridServos->setVerticalSpacing(24);

    const qreal k = 1.0;                   // escala de tamaño
    const int  cardSize = int(100 * k);    // lado de la tarjeta
    const int  rCorner  = int(10  * k);    // radio de esquina
    const int  titlePt  = int(8   * k);    // tamaño título "Servo N"
    const int  valuePt  = int(6  * k * 1.9); // tamaño valor

    // Ahora 6 unidades
    QStringList unidadesServo = {"°", "°", "°", "°", "°", "°"};

    for (int i = 0; i < 6; ++i) {
        QFrame* card = new QFrame();
        card->setFrameShape(QFrame::NoFrame);
        card->setStyleSheet(QString(
            "background-color: #2c2c2c;"
            "border-radius: %1px;"
        ).arg(rCorner));
        card->setMinimumSize(cardSize, cardSize);

        QVBoxLayout* cardLayout = new QVBoxLayout(card);
        cardLayout->setAlignment(Qt::AlignCenter);
        cardLayout->setSpacing(int(4 * k));
        cardLayout->setContentsMargins(int(6 * k), int(6 * k), int(6 * k), int(6 * k));

        // Título
        QLabel* labelTitulo = new QLabel(QString("Servo %1").arg(i + 1));
        QFont tituloFont;
        tituloFont.setPointSize(titlePt);
        tituloFont.setBold(true);
        labelTitulo->setFont(tituloFont);
        labelTitulo->setStyleSheet("color: white;");
        labelTitulo->setAlignment(Qt::AlignCenter);

        // Valor
        if (i >= 6) continue;
        labelServos[i] = new QLabel("0" + unidadesServo[i]);
        QFont valorFont;
        valorFont.setPointSize(valuePt);
        valorFont.setBold(true);
        labelServos[i]->setFont(valorFont);
        labelServos[i]->setStyleSheet("color: white;");
        labelServos[i]->setAlignment(Qt::AlignCenter);

        cardLayout->addWidget(labelTitulo);
        cardLayout->addStretch(1);
        cardLayout->addWidget(labelServos[i]);
        cardLayout->addStretch(1);

        // Colocar en una grilla de 2 filas × 3 columnas
        gridServos->addWidget(card, i / 3, i % 3);
    }

    // Stretch para llenar el espacio disponible
    for (int col = 0; col < 3; ++col) gridServos->setColumnStretch(col, 1);
    for (int row = 0; row < 2; ++row) gridServos->setRowStretch(row, 1);

    QWidget* servoWidget = new QWidget();
    servoWidget->setLayout(gridServos);
    layout->addWidget(servoWidget, 2, 2, 1, 2);

    // === ESTADO DEL SISTEMA (Inferior completamente horizontal) ===
    QVBoxLayout* statusBottomLayout = new QVBoxLayout();
    statusBottomLayout->setContentsMargins(5, 5, 5, 5);
    statusBottomLayout->setSpacing(5);

    QHBoxLayout* indicatorsLayout = new QHBoxLayout();
    indicatorsLayout->setSpacing(10);

    // --- Ventana de tiempo (Slider) ---
    QWidget* winBox = new QWidget();
    winBox->setStyleSheet("background-color: #2c2c2c; border-radius: 10px;");
    winBox->setMinimumWidth(250);
    QVBoxLayout* winLay = new QVBoxLayout(winBox);
    winLay->setContentsMargins(4, 2, 4, 2);
    winLay->setSpacing(2);

    winText = new QLabel(tr("Ventana: Máx"));
    winText->setAlignment(Qt::AlignCenter);
    winText->setStyleSheet("color: white; font-weight: bold; font-size: 10px;");

    winSlider = new QSlider(Qt::Horizontal);
    winSlider->setRange(0, 7);
    winSlider->setValue(7);
    winSlider->setTickPosition(QSlider::TicksBelow);
    winSlider->setTickInterval(1);
    winSlider->setStyleSheet(R"(
    QSlider::groove:horizontal {
        height: 12px; border-radius: 6px;
        background: qlineargradient(x1:0, y1:0.5, x2:1, y2:0.5, stop:0 #2ecc71, stop:0.5 #f1c40f, stop:1 #e74c3c);
    }
    QSlider::handle:horizontal {
        background: white; border: 1px solid #bbb;
        width: 18px; height: 18px; margin: -4px 0;
        border-radius: 9px;
    }
    )");
    winLay->addWidget(winText);
    winLay->addWidget(winSlider);
    indicatorsLayout->addWidget(winBox);

    // --- Indicadores de estado ---
    QStringList campos = {"Conexión", "T. Inicio", "Paracaídas", "Satélital", "F. Local", "H. Local", "Puerto", "Velocidad"};
    for (int i = 0; i < campos.size(); ++i) {
        QFrame* card = new QFrame();
        card->setStyleSheet("background-color: #2c2c2c; border-radius: 8px;");
        card->setMinimumSize(85, 40);
        QVBoxLayout* cardLay = new QVBoxLayout(card);
        cardLay->setContentsMargins(2, 2, 2, 2);
        cardLay->setSpacing(0);

        QLabel* titulo = new QLabel(campos[i]);
        titulo->setStyleSheet("color: #aaa; font-size: 9px;");
        titulo->setAlignment(Qt::AlignCenter);

        labelStatus[i] = new QLabel("...");
        labelStatus[i]->setStyleSheet("color: white; font-weight: bold; font-size: 10px;");
        labelStatus[i]->setAlignment(Qt::AlignCenter);

        if (i == 6) labelCom = labelStatus[i];
        if (i == 7) labelBaud = labelStatus[i];

        cardLay->addWidget(titulo);
        cardLay->addWidget(labelStatus[i]);
        indicatorsLayout->addWidget(card);
    }
    statusBottomLayout->addLayout(indicatorsLayout);

    // --- Paquete RAW (Paquete) al fondo absoluto ---
    labelRaw = new QLabel("Paquete: ...");
    labelRaw->setStyleSheet("color: #00ff00; font-size: 10px; background-color: #1a1a1a; padding: 5px; border-radius: 4px; border: 1px solid #333;");
    labelRaw->setWordWrap(true);
    labelRaw->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    QFont mono("Consolas"); mono.setPointSizeF(9); labelRaw->setFont(mono);

    statusBottomLayout->addWidget(labelRaw);

    QWidget* bottomWidget = new QWidget();
    bottomWidget->setLayout(statusBottomLayout);
    layout->addWidget(bottomWidget, 3, 0, 1, 4);

    // === Ventana de tiempo ===
    auto secsAt = [](int pos)->int {
        static const int map[8] = {5,10,20,30,40,50,60,0};
        return map[qBound(0,pos,7)];
    };

    connect(winSlider, &QSlider::valueChanged, this, [this, secsAt](int pos){
        windowSec = secsAt(pos);
        winText->setText(windowSec == 0 ? tr("Ventana: Máx")
                                        : tr("Ventana: %1 s").arg(windowSec));

        // Reencuadra TODAS las gráficas SIN eliminar datos
        for (auto &p : seriesAndXAxis) {
            auto* s  = p.first;
            auto* ax = p.second;
            if (!s || !ax || s->count() == 0) continue;

            const auto pts = s->points();
            const double last = pts.constLast().x();

            if (windowSec == 0) {
                ax->setRange(0.0, last);
            } else {
                const double from = std::max(0.0, last - double(windowSec));
                ax->setRange(from, last);
            }
        }
    });

    // Mantener sincronizados los labels cuando SensorManager reconfigure el puerto
    connect(m_sensorManager, &SensorManager::serialReconfigured,
            this, [this](const QString& port, int baud, bool ok) {
        if ((labelCom  && labelCom->property("fileMode").toBool()) ||
            (labelBaud && labelBaud->property("fileMode").toBool())) {
            return;
        }
        if (labelCom)  labelCom->setText(QString(" %1%2").arg(port, ok ? "" : " (error)"));
        if (labelBaud) labelBaud->setText(QString(" %1").arg(baud));
    });

    // === Suscripción a DataTopic ===
   static RangeChecker rangeChecker;

   connect(DataTopic::instance(), &DataTopic::dataPublished, this, [this](const QString& line) {
        QString cleanLine = line;
        int colonIndex = line.indexOf(':');
        if (line.startsWith('#') && colonIndex != -1) {
            cleanLine = line.mid(colonIndex + 1).trimmed();
        }

        SensorData d = SensorData::deserialize(cleanLine);
        static int t = 0;

        if (resetTimeBase_) { t = 0; resetTimeBase_ = false; }

        auto actualizarGrafica = [&](QLineSeries* series,
                             double valor,
                             QLabel*     label,
                             QValueAxis* axisX,
                             QValueAxis* axisY,
                             HoverChartView* hview)
        {
            if (!series || !label || !axisX || !axisY
                || std::isnan(valor) || std::isinf(valor))
                return;

            if (hview) {
                hview->setAlertLevel(rangeChecker.check(series->objectName(), valor));
            }

            series->append(t, valor);

            QString nombre = series->objectName();                
            QString unidad = series->property("tipoDato").toString();

            QString texto  = QString("%1: %2 %3")
                            .arg(nombre)                        
                            .arg(valor, 0, 'f', 3)              
                            .arg(unidad);  

            series->setName(texto);
            label->setText(texto);

            if (hview && hview->autoFollow()) {
                if (windowSec == 0) {
                    axisX->setRange(0, t);
                } else {
                    axisX->setRange(std::max(0, t - windowSec), t);
                }
            }

            if (hview && hview->autoFollow()) {
                if (axisY->min() == axisY->max())
                    axisY->setRange(valor, valor+1);
                else {
                    if (valor > axisY->max()) axisY->setMax(valor);
                    if (valor < axisY->min()) axisY->setMin(valor);
                }
            }
        };

        actualizarGrafica(seriesRoll, d.Roll, labelRoll, axisX_Roll, axisY_Roll, viewRoll);
        actualizarGrafica(seriesPitch, d.Pitch, labelPitch, axisX_Pitch, axisY_Pitch, viewPitch);
        actualizarGrafica(seriesYaw, d.Yaw, labelYaw, axisX_Yaw, axisY_Yaw, viewYaw);
        actualizarGrafica(seriesSats, d.satellites, labelSats, axisX_Sats, axisY_Sats, viewSats);
        actualizarGrafica(seriesLat, d.latitude, labelLat, axisX_Lat, axisY_Lat, viewLat);
        actualizarGrafica(seriesLon, d.longitude, labelLon, axisX_Lon, axisY_Lon, viewLon);
        actualizarGrafica(seriesAlt, d.AltDiff, labelAlt, axisX_Alt, axisY_Alt, viewAlt);
        actualizarGrafica(seriesHdop, d.hdop, labelHdop, axisX_Hdop, axisY_Hdop, viewHdop);
        actualizarGrafica(seriesPressure, d.pressure, labelPressure, axisX_Pressure, axisY_Pressure, viewPressure);
        actualizarGrafica(seriesTemp, d.temperature, labelTemp, axisX_Temp, axisY_Temp, viewTemp);

        labelServos[0]->setText(QString::number(d.Servo1) + "°");
        labelServos[1]->setText(QString::number(d.Servo2) + "°");
        labelServos[2]->setText(QString::number(d.Servo3) + "°");
        labelServos[3]->setText(QString::number(d.Servo4) + "°");

        labelStatus[0]->setText("Estable: 3");
        labelStatus[1]->setText("Inicializado");
        labelStatus[2]->setText("N/A");
        labelStatus[3]->setText(QString::fromStdString(d.date));
        labelStatus[4]->setText(QDate::currentDate().toString("dd-MM-yy"));
        labelStatus[5]->setText(QTime::currentTime().toString("hh:mm:ss"));

        labelRaw->setText("Paquete: " + line);

        if (!modoArchivo_) {
            if (timeoutTimer) timeoutTimer->start();

            if (!tiempoIniciado) {
                tiempoInicio = QTime::currentTime();
                tiempoIniciado = true;
                if (timer) timer->start(1000);
            } else if (timer && !timer->isActive()) {
                timer->start(1000);
            }
        }

        ++t;

        procesarDatos(d);
    }, Qt::QueuedConnection);

    this->timer = new QTimer(this);
    timeoutTimer = new QTimer(this);
    timeoutTimer->setInterval(30000);
    timeoutTimer->setSingleShot(true);

    connect(timer, &QTimer::timeout, this, [this]() {
        if (modoArchivo_) return;
        int secs = tiempoInicio.secsTo(QTime::currentTime());
        QTime t(0, 0);
        t = t.addSecs(secs);
        WindowManager::instance()->updateSessionTimer(t);
    });
    
    connect(timeoutTimer, &QTimer::timeout, this, [this]() {
        if (timer->isActive()) {
            timer->stop();
        }
    });

    connect(WindowManager::instance(), &WindowManager::dataResetRequested, this, &Widget::resetCharts);
    connect(WindowManager::instance(), &WindowManager::modoArchivoChanged, this, [this](bool enabled, const QString& fileName){
         modoArchivo_ = enabled;
         if (enabled) {
              if (labelCom)  { labelCom->setText(" No aplica");        labelCom->setProperty("fileMode", true); }
              if (labelBaud) { labelBaud->setText(" No aplica"); labelBaud->setProperty("fileMode", true); }
              setWindowTitle(QString("FRECCIA_XAE - Gráficas 2D (CSV: %1)").arg(fileName));
         } else {
              if (labelCom)  { labelCom->setProperty("fileMode", false);  labelCom->setText("..."); }
              if (labelBaud) { labelBaud->setProperty("fileMode", false); labelBaud->setText("..."); }
              setWindowTitle("FRECCIA_XAE - Gráficas 2D");
         }
    });
}

void Widget::resetCharts() {
        tiempoIniciado = false;
        tiempoInicio = QTime();
        if (timer && timer->isActive()) timer->stop();
        if (timeoutTimer && timeoutTimer->isActive()) timeoutTimer->stop();

        for (auto* chartView : findChildren<QChartView*>()) {
            if (auto* chart = chartView->chart()) {
                for (auto* s : chart->series()) {
                    if (auto* line = qobject_cast<QLineSeries*>(s)) line->clear();
                }
                for (auto* ax : chart->axes(Qt::Horizontal)) {
                    if (auto* v = qobject_cast<QValueAxis*>(ax)) v->setRange(0.0, 1.0);
                }
                for (auto* ax : chart->axes(Qt::Vertical)) {
                    if (auto* v = qobject_cast<QValueAxis*>(ax)) v->setRange(0.0, 1.0);
                }
            }
            if (auto* hview = qobject_cast<HoverChartView*>(chartView)) {
                hview->setAutoFollow(true);
            }
        }

        auto restLabel = [](QLabel* lbl, const QString& nombre, const QString& unidad){
            if (lbl) lbl->setText(QString("%1: 0 %2").arg(nombre, unidad));
        };
        restLabel(labelRoll,  "Roll",        "°");
        restLabel(labelPitch, "Pitch",       "°");
        restLabel(labelYaw,   "Yaw",         "°");
        restLabel(labelSats,  "Satélites",   "Conexiones");
        restLabel(labelLat,   "Latitud",     "°");
        restLabel(labelLon,   "Longitud",    "°");
        restLabel(labelAlt,   "AltDiff",     "m");
        restLabel(labelHdop,  "HDOP",        "°");
        restLabel(labelPressure, "Presión",  "hPa");
        restLabel(labelTemp,     "Temperatura", "°C");

        for (int i = 0; i < 8; ++i) if (labelStatus[i]) labelStatus[i]->setText("...");
        if (labelRaw) labelRaw->setText("Paquete: ...");
        for (int i = 0; i < 6; ++i) if (labelServos[i]) labelServos[i]->setText("0°");

        resetTimeBase_ = true;

        if (WindowManager::instance()->graph3D()) {
            WindowManager::instance()->graph3D()->resetData();
        }
}

void Widget::abrirVentana3DDesdeExterno() {}

void Widget::procesarDatos(const SensorData& data) {
    WindowManager::instance()->procesarDatos(data);
}

void Widget::abrirDialogoSerial() {
    auto leerPuertoActual = [this]() -> QString {
        QString txt = labelCom && !labelCom->text().isEmpty() ? labelCom->text() : QString();
        if (txt.startsWith("COM: ")) txt = txt.mid(5);
        txt.replace(" (error)", "");
        return txt.trimmed();
    };
    auto leerBaudActual = [this]() -> int {
        QString txt = labelBaud && !labelBaud->text().isEmpty() ? labelBaud->text() : QString();
        txt.replace("Velocidad:", "").remove(' ');
        bool ok = false; int b = txt.toInt(&ok);
        return ok ? b : 115200;
    };

    QDialog dlg(this);
    dlg.setWindowTitle("Configurar puerto serie");
    dlg.setModal(true);
    dlg.resize(360, 120); 

    auto* cbOS      = new QComboBox(&dlg);
    auto* cbPuertos = new QComboBox(&dlg);
    auto* cbBaud    = new QComboBox(&dlg);
    auto* btns      = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dlg);

    cbOS->addItems({"Linux", "Windows"});

    const QList<int> bauds = {9600, 19200, 38400, 57600, 115200, 230400, 460800};
    for (int b : bauds) cbBaud->addItem(QString::number(b), b);

    auto listarPuertosLinux = []() -> QStringList {
        QStringList out;
        const auto infos = QSerialPortInfo::availablePorts();
        for (const auto& info : infos) out << info.systemLocation(); 
        for (int i = 0; i < 32; ++i) out << QString("/dev/pts/%1").arg(i);
        out.removeDuplicates();
        out.sort();
        return out;
    };
    auto listarPuertosWindows = []() -> QStringList {
        QStringList out;
        const auto infos = QSerialPortInfo::availablePorts();
        for (const auto& info : infos) out << info.portName();
        if (out.isEmpty()) {
            for (int i = 1; i <= 100; ++i) out << QString("COM%1").arg(i);
        }
        out.removeDuplicates();
        out.sort();
        return out;
    };

    auto poblarPuertos = [&](const QString& osName, const QString& preselect) {
        cbPuertos->clear();
        QStringList lista = (osName == "Windows") ? listarPuertosWindows()
                                                  : listarPuertosLinux();
        cbPuertos->setEditable(true);
        cbPuertos->addItems(lista);
        int idx = lista.indexOf(preselect);
        if (idx >= 0) cbPuertos->setCurrentIndex(idx);
        else if (!preselect.isEmpty()) cbPuertos->setEditText(preselect);
    };

    auto* lay = new QFormLayout(&dlg);
    lay->setContentsMargins(20, 20, 20, 20);
    lay->setSpacing(14);
    lay->addRow("Sistema:", cbOS);
    lay->addRow("Puerto:",  cbPuertos);
    lay->addRow("Baud:",    cbBaud);
    lay->addRow(btns);

    const QString puertoActual = leerPuertoActual();
    const int     baudActual   = leerBaudActual();

    #ifdef Q_OS_WIN
        cbOS->setCurrentText("Windows");
    #else
        cbOS->setCurrentText("Linux");
    #endif

    poblarPuertos(cbOS->currentText(), puertoActual);

    int idxBaud = cbBaud->findData(baudActual);
    cbBaud->setCurrentIndex(idxBaud >= 0 ? idxBaud : cbBaud->findData(115200));

    connect(cbOS, &QComboBox::currentTextChanged, &dlg, [&, this](const QString& osName){
        const QString textoActual = cbPuertos->currentText().trimmed();
        poblarPuertos(osName, textoActual.isEmpty() ? puertoActual : textoActual);
    });

    QObject::connect(btns, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    QObject::connect(btns, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);

    if (dlg.exec() == QDialog::Accepted) {
        const QString port = cbPuertos->currentText().trimmed();
        const int baud     = cbBaud->currentData().toInt();
        if (port.isEmpty() || baud <= 0) {
            QMessageBox::warning(this, "Configurar COM", "Selecciona un puerto y un baudrate válido.");
            return;
        }

        const bool ok = m_sensorManager->setSerial(port, baud);

        if(labelCom) labelCom->setText(QString(" %1%2").arg(port, ok ? "" : " (error)"));
        if(labelBaud) labelBaud->setText(QString(" %1").arg(baud));

            if (!ok) {
                QMessageBox::critical(this, "Puerto serie",
                                    "No se pudo abrir el puerto.\n"
            #ifdef Q_OS_WIN
                                            "Prueba con otro COM o reconecta el dispositivo."
            #else
                                            "Prueba con otro /dev/* o revisa permisos (grupo dialout/tty)."
            #endif
            );
        }
    }
}

Widget::~Widget() {}
