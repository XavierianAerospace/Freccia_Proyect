#include "widget.h"
#include "Graph3DWindow.h"
#include "data/FileHelper.h"

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

static Widget* ventanaUnica = nullptr;
Graph3DWindow* ventanaGraph3D = nullptr;

Widget::Widget(SensorManager* manager, QWidget* parent)
    : QWidget(parent), m_sensorManager(manager) {
    
    // === Verificación de instancia única ===
    if (ventanaUnica && ventanaUnica != this) {
        close();
        return;
    }
    ventanaUnica = this;

    for (int i = 0; i < 6; ++i) labelStatus[i] = nullptr;
    for (int i = 0; i < 4; ++i) labelServos[i] = nullptr;

    setWindowTitle("FRECCIA_XAE - Gráficas 2D");
    setStyleSheet("background-color: black;");
    QGridLayout* layout = new QGridLayout();
    layout->setSpacing(2);
    setWindowIcon(QIcon("./assets/logo_xae.png"));

    pantalla1Activa = true;
    actualizarEstilosMenu();

    // === Menu ===
    QWidget* topBar = new QWidget();
    topBar->setStyleSheet("background-color: black; color: white;");
    topBar->setFixedHeight(30);

    QHBoxLayout* topLayout = new QHBoxLayout(topBar);
    topLayout->setContentsMargins(5, 0, 5, 0);

    // === Botones ===
    QPushButton* btnRecord = new QPushButton("● Grabar");
    btnRecord->setStyleSheet(
        "QPushButton { "
        "color: white; "
        "background-color: red; "
        "border: none; "
        "padding: 6px 14px; "
        "font-size: 15px; "
        "border-radius: 8px; "
        "}"
        "QPushButton:hover { background-color: darkred; }"
    );

    QPushButton* btnStop = new QPushButton("■ Detener");
    btnStop->setStyleSheet(
        "QPushButton { "
        "color: white; "
        "background-color: transparent; "
        "border: none; "
        "padding: 6px 14px; "
        "font-size: 15px; "
        "border-radius: 8px; "
        "}"
        "QPushButton:hover { background-color: #444; }"
    );

    QPushButton* btnVerAntiguos = new QPushButton("Ver antiguos");
    btnVerAntiguos->setStyleSheet(
        "QPushButton { "
        "color: white; "
        "background-color: transparent; "
        "border: none; "
        "padding: 6px 14px; "
        "font-size: 15px; "
        "border-radius: 8px; "
        "}"
        "QPushButton:hover { background-color: #444; }"
    );

    // FileHelper inicializado
    fileHelper = new FileHelper();

    // Inicialmente solo el botón Grabar está habilitado
    btnRecord->setEnabled(true);
    btnStop->setEnabled(false);

    // === Conexión botón GRABAR ===
    connect(btnRecord, &QPushButton::clicked, this, [this, btnRecord, btnStop]() {
        fileHelper->iniciarGrabacion();

        // Reiniciar tiempo y contador
        tiempoGrabacion = QTime(0, 0, 0);

        if (!timerGrabacion) {
            timerGrabacion = new QTimer(this);
            connect(timerGrabacion, &QTimer::timeout, this, [this, btnRecord]() {
                QString tiempoTexto = tiempoGrabacion.toString("hh:mm:ss");
                btnRecord->setText("● Grabando " + tiempoTexto);
                tiempoGrabacion = tiempoGrabacion.addSecs(1);
                fileHelper->tiempoGrabado = tiempoGrabacion;
            });
        }

        timerGrabacion->start(1000);
        btnRecord->setText("● Grabando 00:00:00");

        // Cambiar estilo a activo
        btnRecord->setStyleSheet(
            "QPushButton { "
            "color: red; "
            "background-color: black; "
            "border: none; "
            "padding: 6px 14px; "
            "font-size: 15px; "
            "border-radius: 8px; "
            "} "
            "QPushButton:hover { background-color: #222; }"
        );

        btnRecord->setEnabled(false);
        btnStop->setEnabled(true);
    });

    // === Conexión botón DETENER ===
    connect(btnStop, &QPushButton::clicked, this, [this, btnStop, btnRecord]() {
        fileHelper->detenerGrabacion();

        // Detener y ocultar contador
        if (timerGrabacion) timerGrabacion->stop();
        btnRecord->setText("● Grabar");

        // Restaurar estilo original
        btnRecord->setStyleSheet(
            "QPushButton { "
            "color: white; "
            "background-color: red; "
            "border: none; "
            "padding: 6px 14px; "
            "font-size: 15px; "
            "border-radius: 8px; "
            "} "
            "QPushButton:hover { background-color: darkred; }"
        );

        // Estilo temporal rojo para detener
        btnStop->setStyleSheet(
            "QPushButton { "
            "color: red; "
            "background-color: transparent; "
            "border: none; "
            "padding: 6px 14px; "
            "font-size: 15px; "
            "border-radius: 8px; "
            "} "
            "QPushButton:hover { background-color: #444; }"
        );

        QTimer::singleShot(1200, this, [btnStop]() {
            btnStop->setStyleSheet(
                "QPushButton { "
                "color: white; "
                "background-color: transparent; "
                "border: none; "
                "padding: 6px 14px; "
                "font-size: 15px; "
                "border-radius: 8px; "
                "} "
                "QPushButton:hover { background-color: #444; }"
            );
        });

        btnRecord->setEnabled(true);
        btnStop->setEnabled(false);
    });

    QWidget* leftButtons = new QWidget();
    QHBoxLayout* leftLayout = new QHBoxLayout(leftButtons);
    leftLayout->setContentsMargins(0, 0, 0, 0);
    leftLayout->addWidget(btnRecord);
    leftLayout->addWidget(btnStop);
    leftLayout->addWidget(btnVerAntiguos);

    // === Tiempo ===
    labelTiempo = new QLabel("Tiempo: 00:00:00");
    labelTiempo->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);  // centrado horizontal y vertical
    labelTiempo->setStyleSheet(
        "color: white;"
        "font-weight: bold;"
        "font-size: 18px;"
    );

    // Menú desplegable y acciones
   QPushButton* btnMenu = new QPushButton();
    btnMenu->setIcon(QIcon("./assets/Menu.png"));
    btnMenu->setIconSize(QSize(35, 35));
    btnMenu->setStyleSheet("background-color: transparent; border: none;");

    // Menú
    QMenu* menuDesplegable = new QMenu(this);
    menuDesplegable->setStyleSheet("QMenu { background-color: black; color: white; }"
                                "QMenu::item:selected { background-color: #444; }");

    btnMenu->setMenu(menuDesplegable);
    // Acciones
    pantalla1 = menuDesplegable->addAction("Pantalla Gráficas 2D");
    pantalla2 = menuDesplegable->addAction("Pantalla Gráficas 3D y OSM");

    // Estilo para el menú completo
    menuDesplegable->setStyleSheet(R"(
        QMenu {
            background-color: black;
            color: white;
        }
        QMenu::item {
            padding: 6px 24px;
            background-color: black;
            color: white;
        }
        QMenu::item:disabled {
            background-color: green;
            color: white;
        }
        QMenu::item:selected:enabled {
            background-color: #444;
        }
    )");

    // === Detectar si ya está abierta ===
    if (ventanaUnica && ventanaUnica != this) {
            close();
            return;
        }
        ventanaUnica = this;
        pantalla1->setEnabled(false);
        QObject::connect(this, &QWidget::destroyed, []() {
            ventanaUnica = nullptr;
    });

    // === Boton Pantalla Gráficas 2D ===
    connect(pantalla1, &QAction::triggered, this, [manager]() {
        if (!ventanaUnica) {
            ventanaUnica = new Widget(manager);
            ventanaUnica->resize(1280, 720);
            ventanaUnica->show();
        } else {
            ventanaUnica->raise();
            ventanaUnica->activateWindow();
        }
    });

    if (ventanaGraph3D) {
        pantalla2->setEnabled(false);
        pantalla1Activa = false;
        actualizarEstilosMenu();
    }

    // === Botón Pantalla Gráficas ===
    connect(pantalla2, &QAction::triggered, this, [this, manager]() {
        if (ventanaGraph3D) {
            ventanaGraph3D->raise();
            ventanaGraph3D->activateWindow();
            return;
        }

        pantalla1Activa = false;
        actualizarEstilosMenu();

        pantalla2->setEnabled(false);

        ventanaGraph3D = new Graph3DWindow(manager);
        ventanaGraph3D->setAttribute(Qt::WA_DeleteOnClose);
        ventanaGraph3D->resize(1280, 720);
        ventanaGraph3D->show();

        connect(ventanaGraph3D, &QWidget::destroyed, this, [this]() {
            pantalla2->setEnabled(true);
            actualizarEstilosMenu();
            ventanaGraph3D = nullptr;
        });
    });

    menuDesplegable->addSeparator();
    QAction* configCom = menuDesplegable->addAction("Configurar COM");
    QAction* configBaud = menuDesplegable->addAction("Configurar Baud");
    QAction* configAcc = menuDesplegable->addAction("Calibrar Acelerómetro");
    QAction* configGyro = menuDesplegable->addAction("Calibrar Giroscopio");
    menuDesplegable->addSeparator();
    QWidgetAction* cerrarWidgetAction = new QWidgetAction(this);
    QPushButton* cerrarBtn = new QPushButton("Close the program");
    cerrarBtn->setStyleSheet("color: white; background-color: red; border: none; padding: 4px;");
    cerrarWidgetAction->setDefaultWidget(cerrarBtn);
    menuDesplegable->addAction(cerrarWidgetAction);

    btnMenu->setMenu(menuDesplegable);

    // === Añadir a layout superior ===
    topLayout->addWidget(leftButtons);
    topLayout->addStretch();
    topLayout->addWidget(labelTiempo);
    topLayout->addStretch();
    topLayout->addWidget(btnMenu);

    // === Añadir barra al layout principal ===
    QVBoxLayout* globalLayout = new QVBoxLayout(this);
    globalLayout->setContentsMargins(0, 0, 0, 0);
    globalLayout->addWidget(topBar);
    globalLayout->addLayout(layout);

    auto crearGrafica = [&](QChart*& chart,
                        QLineSeries*& series,
                        QChartView*& view,       // devolvemos como QChartView* para no cambiar firmas
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

       // --- View con soporte de hover ---
        auto* hview = new HoverChartView();
        hview->setRenderHint(QPainter::Antialiasing, true);
        hview->setMinimumSize(400, 250);
        hview->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        hview->setChart(chart);
        view = hview;
        layout->addWidget(hview, fila, columna);

        // *** CREA EL LABEL ANTES DE LAS LAMBDAS ***
        label = new QLabel(nombre + ": 0");
        label->setStyleSheet("color: white; font-weight: bold;");
        layout->addWidget(label, fila + 1, columna);

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

    const qreal k = 1.6;                   // escala de tamaño
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
        card->setFixedSize(cardSize, cardSize);

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
    layout->addWidget(servoWidget, 2, 2);

    // === ESTADO DEL SISTEMA (2,3) con distribución 3 filas x 2 columnas ===
    QGridLayout* gridEstado = new QGridLayout();
    gridEstado->setSpacing(8);
    gridEstado->setContentsMargins(0, 0, 0, 0);

    gridEstado->setHorizontalSpacing(16);
    gridEstado->setVerticalSpacing(10);
    gridEstado->setColumnStretch(0, 1);
    gridEstado->setColumnStretch(1, 1);

    QWidget* winBox = new QWidget();
    winBox->setStyleSheet("background-color: #2c2c2c; border-radius: 10px;");
    QVBoxLayout* winLay = new QVBoxLayout(winBox);
    winLay->setContentsMargins(10, 8, 10, 8);
    winLay->setSpacing(6);

    // Texto / valor seleccionado
    winText = new QLabel(tr("Ventana: Máx"));
    winText->setAlignment(Qt::AlignCenter);
    winText->setStyleSheet("color: white; font-weight: bold;");

    // Slider con 8 posiciones: 5,10,20,30,40,50,60, Máx
    winSlider = new QSlider(Qt::Horizontal);
    winSlider->setRange(0, 7);
    winSlider->setValue(7); // 7 = Máx por defecto
    winSlider->setTickPosition(QSlider::TicksBelow);
    winSlider->setTickInterval(1);
    winSlider->setSingleStep(1);

    // Estilo
    winSlider->setStyleSheet(R"(
    QSlider::groove:horizontal {
        height: 16px; border-radius: 8px;
        margin: 8px 10px;
        background: qlineargradient(x1:0, y1:0.5, x2:1, y2:0.5,
        stop:0 #2ecc71, stop:0.5 #f1c40f, stop:1 #e74c3c);
    }
    QSlider::handle:horizontal {
        background: white; border: 1px solid #bbb;
        width: 24px; height: 24px; margin: -6px 0;
        border-radius: 12px;
    }
    QSlider::sub-page:horizontal { background: transparent; }
    QSlider::add-page:horizontal { background: transparent; }
    QSlider::tick-position:below { color: #aaa; }
    )");

    // Etiquetas “5s ... Máx”
    QStringList tickText = {"5s","10s","20s","30s","40s","50s","60s","Máx"};
    QHBoxLayout* ticks = new QHBoxLayout();
    ticks->setContentsMargins(14, 0, 14, 0);
    for (int i=0;i<tickText.size();++i) {
        QLabel* t = new QLabel(tickText[i]);
        t->setStyleSheet("color:#bbb; font-size:10px;");
        t->setAlignment(Qt::AlignCenter);
        ticks->addWidget(t, 1);
    }

    winLay->addWidget(winText);
    winLay->addWidget(winSlider);
    winLay->addLayout(ticks);

    // Contenedor vertical de la columna derecha
    QVBoxLayout* estadoFinal = new QVBoxLayout();
    estadoFinal->setContentsMargins(0,0,0,0);
    estadoFinal->addWidget(winBox);

    QStringList campos = {"Conexión", "Tiempo Para Inicio", "Paracaídas", "Fecha Satélital", "Fecha Local", "Hora Local"};

    for (int i = 0; i < campos.size(); ++i) {
        QFrame* card = new QFrame();
        if (!card) {
            qDebug() << "Error: card es nullptr en índice" << i;
            continue;
        }

        card->setStyleSheet("background-color: #2c2c2c; border-radius: 10px;");
        card->setMinimumHeight(56);
        card->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

        QVBoxLayout* cardLayout = new QVBoxLayout(card);
        cardLayout->setAlignment(Qt::AlignCenter);
        cardLayout->setContentsMargins(4, 2, 4, 2);

        QLabel* titulo = new QLabel(campos[i]);
        titulo->setStyleSheet("color: white; font-size: 10px;");
        titulo->setAlignment(Qt::AlignCenter);

        labelStatus[i] = new QLabel("Esperando...");
        labelStatus[i]->setStyleSheet("color: white; font-weight: bold; font-size: 12px;");
        labelStatus[i]->setAlignment(Qt::AlignCenter);

        cardLayout->addWidget(titulo);
        cardLayout->addWidget(labelStatus[i]);

        if (labelStatus[i] == nullptr || titulo == nullptr || cardLayout == nullptr) {
            qDebug() << "Fallo en creación de widgets para campo" << campos[i];
            continue;
        }

        gridEstado->addWidget(card, i / 2, i % 2);
    }

    // === Línea inferior con COM y Velocidad ===
    QHBoxLayout* puertoLayout = new QHBoxLayout();
    puertoLayout->setSpacing(12);
    puertoLayout->setContentsMargins(0, 0, 0, 0);

    labelCom = new QLabel("COM: Esperando...");
    labelCom->setStyleSheet("color: white; font-size: 11px; font-weight: bold;");
    labelCom->setAlignment(Qt::AlignLeft);

    labelBaud = new QLabel("Velocidad: Esperando...");
    labelBaud->setStyleSheet("color: white; font-size: 11px; font-weight: bold;");
    labelBaud->setAlignment(Qt::AlignRight);

    puertoLayout->addWidget(labelCom);
    puertoLayout->addStretch();
    puertoLayout->addWidget(labelBaud);

    // Añade el grid de estado al contenedor
    estadoFinal->addLayout(gridEstado);

    // Línea COM y Velocidad
    estadoFinal->addLayout(puertoLayout);

    // === Línea final con paquete RAW ===
    labelRaw = new QLabel("Paquete: Esperando...");
    labelRaw->setStyleSheet("color: white; font-size: 10px; background-color: #1e1e1e; padding: 6px;");
    labelRaw->setWordWrap(true);
    labelRaw->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    labelRaw->setMinimumHeight(30);
    labelRaw->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    labelRaw->setMaximumWidth(475);
    labelRaw->setTextInteractionFlags(Qt::TextSelectableByMouse);

    QFont mono;
    mono.setFamily("Consolas");
    mono.setStyleHint(QFont::Monospace);
    labelRaw->setFont(mono);

    if (labelRaw) {
        estadoFinal->addWidget(labelRaw);
    } else {
        QLabel* fallbackRaw = new QLabel("Error: RAW no inicializado");
        fallbackRaw->setStyleSheet("color: red;");
        estadoFinal->addWidget(fallbackRaw);
    }

    QWidget* estadoWidget = new QWidget();
    estadoWidget->setLayout(estadoFinal);
    layout->addWidget(estadoWidget, 2, 3);

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
                // Mostrar el histórico completo
                ax->setRange(0.0, last);
            } else {
                // Últimos N segundos
                const double from = std::max(0.0, last - double(windowSec));
                ax->setRange(from, last);
            }
        }
    });

    // === Datos en tiempo real ===
    connect(m_sensorManager, &SensorManager::newSensorData, this, [&](const SensorData& d) {
        static int t = 0;

        auto actualizarGrafica = [&](QLineSeries* series,
                             double valor,
                             QLabel*     label,
                             QValueAxis* axisX,
                             QValueAxis* axisY)
        {
            if (!series || !label || !axisX || !axisY
                || std::isnan(valor) || std::isinf(valor))
                return;

            // 1) apuntamos el nuevo valor en la curva
            series->append(t, valor);

            // 2) recuperamos nombre + unidad
            QString nombre = series->objectName();                
            QString unidad = series->property("tipoDato").toString();

            // 3) montamos el texto: "AltDiff: 3.50 m"
            QString texto  = QString("%1: %2 %3")
                            .arg(nombre)                        
                            .arg(valor, 0, 'f', 3)              
                            .arg(unidad);  

            // 4) actualizamos leyenda *y* etiqueta
            series->setName(texto);
            label->setText(texto);

            // 5) reajustamos ejes X según la ventana seleccionada (SIN borrar puntos)
            if (windowSec == 0) {
                // Máx: mostrar todo lo acumulado desde t = 0 hasta t
                axisX->setRange(0, t);
            } else {
                // Ventana deslizante: últimos windowSec segundos
                axisX->setRange(std::max(0, t - windowSec), t);
            }

            // 6) reajustamos ejes Y dinámicamente
            if (axisY->min() == axisY->max())
                axisY->setRange(valor, valor+1);
            else {
                if (valor > axisY->max()) axisY->setMax(valor);
                if (valor < axisY->min()) axisY->setMin(valor);
            }
        };

        actualizarGrafica(seriesRoll, d.Roll, labelRoll, axisX_Roll, axisY_Roll);
        actualizarGrafica(seriesPitch, d.Pitch, labelPitch, axisX_Pitch, axisY_Pitch);
        actualizarGrafica(seriesYaw, d.Yaw, labelYaw, axisX_Yaw, axisY_Yaw);
        actualizarGrafica(seriesSats, d.satellites, labelSats, axisX_Sats, axisY_Sats);
        actualizarGrafica(seriesLat, d.latitude, labelLat, axisX_Lat, axisY_Lat);
        actualizarGrafica(seriesLon, d.longitude, labelLon, axisX_Lon, axisY_Lon);
        actualizarGrafica(seriesAlt, d.AltDiff, labelAlt, axisX_Alt, axisY_Alt);
        actualizarGrafica(seriesHdop, d.hdop, labelHdop, axisX_Hdop, axisY_Hdop);
        actualizarGrafica(seriesPressure, d.pressure, labelPressure, axisX_Pressure, axisY_Pressure);
        actualizarGrafica(seriesTemp, d.temperature, labelTemp, axisX_Temp, axisY_Temp);

        labelServos[0]->setText(QString::number(d.Servo1) + "°");
        labelServos[1]->setText(QString::number(d.Servo2) + "°");
        labelServos[2]->setText(QString::number(d.Servo3) + "°");
        labelServos[3]->setText(QString::number(d.Servo4) + "°");

        labelStatus[0]->setText("Estable: 3");
        labelStatus[1]->setText("Inicializado");
        labelStatus[2]->setText("N/A");
        labelStatus[3]->setText(QString::fromStdString(d.date));
        labelStatus[4]->setText(QDate::currentDate().toString("dd-MMMM-yyyy"));
        labelStatus[5]->setText(QTime::currentTime().toString("hh:mm:ss"));

        labelRaw->setText("Paquete: " +
            QString::number(d.latitude) + "," +
            QString::number(d.longitude) + "," +
            QString::fromStdString(d.date) + "," +
            QString::fromStdString(d.utc_time) + "," +
            QString::number(d.secs) + "," +
            QString::number(d.satellites) + "," +
            QString::number(d.hdop) + "," +
            QString::number(d.Roll) + "," +
            QString::number(d.Pitch) + "," +
            QString::number(d.Yaw) + "," +
            QString::number(d.Servo1) + "," +
            QString::number(d.Servo2) + "," +
            QString::number(d.Servo3) + "," +
            QString::number(d.Servo4) + "," +
            QString::number(d.AltDiff)
        );

       timeoutTimer->start();

        if (!tiempoIniciado) {
            tiempoInicio = QTime::currentTime();
            tiempoIniciado = true;
            timer->start(1000);
        } else if (!timer->isActive()) {
            timer->start(1000);
            qDebug() << "Señal recuperada. Reanudando cronómetro.";
        }

        ++t;

        procesarDatos(d);
    }, Qt::QueuedConnection);

    this->timer = new QTimer(this);
    timeoutTimer = new QTimer(this);
    timeoutTimer->setInterval(30000);
    timeoutTimer->setSingleShot(true);

    connect(timer, &QTimer::timeout, this, [this]() {
        int secs = tiempoInicio.secsTo(QTime::currentTime());
        QTime t(0, 0);
        t = t.addSecs(secs);
        labelTiempo->setText("Tiempo: " + t.toString("hh:mm:ss"));
    });
    
    connect(timeoutTimer, &QTimer::timeout, this, [this]() {
        qWarning() << "No se han recibido datos en 20 segundos. Deteniendo el contador.";
        if (timer->isActive()) {
            timer->stop();
            labelTiempo->setText("Señal perdida. Cronómetro detenido.");
        }
    });

    connect(cerrarBtn, &QPushButton::clicked, this, []() {
        QMessageBox msgBox;
        msgBox.setIcon(QMessageBox::Critical);
        msgBox.setWindowTitle("Exit Confirmation");
        msgBox.setText("Are you sure about closing the program?");
        msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
        msgBox.setDefaultButton(QMessageBox::No);

        msgBox.setWindowIcon(QIcon("./assets/logo_xae.png"));

        int reply = msgBox.exec();
        if (reply == QMessageBox::Yes)
            QCoreApplication::quit();
    });
    pantalla1Activa = true;
    actualizarEstilosMenu();
}

void Widget::actualizarEstilosMenu() {
    if (pantalla1 && pantalla2) {
        pantalla1->setIcon(pantalla1Activa ? QIcon(":/icons/activo.png") : QIcon());
    pantalla2->setIcon(!pantalla1Activa ? QIcon(":/icons/activo.png") : QIcon());
    }
}

void Widget::abrirVentana3DDesdeExterno() {
    if (ventanaGraph3D) return;
    pantalla2->setEnabled(false);
    actualizarEstilosMenu();

    connect(ventanaGraph3D, &QWidget::destroyed, this, [this]() {
        pantalla2->setEnabled(true);
        actualizarEstilosMenu();
        ventanaGraph3D = nullptr;
    });
}

void Widget::procesarDatos(const SensorData& data) {
    fileHelper->escribirDuranteGrabacion(data);
}

Widget::~Widget() {}
