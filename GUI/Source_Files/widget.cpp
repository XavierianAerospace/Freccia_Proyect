#include "widget.h"
#include "Graph3DWindow.h"
#include "data/FileHelper.h"
#include "data/RangeChecker.h"
#include "data/DataTopic.h"
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
    layout->setSpacing(0);
    layout->setContentsMargins(0, 0, 0, 0);

    for (int col = 0; col < 4; ++col) layout->setColumnStretch(col, 1);
    for (int row = 0; row < 3; ++row) layout->setRowStretch(row, 1);

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

    // === Controles de recepción  ===
    QWidget* rightButtons = new QWidget();
    QHBoxLayout* rightLayout = new QHBoxLayout(rightButtons);
    rightLayout->setContentsMargins(0, 0, 8, 0);
    rightLayout->setSpacing(6);

    QPushButton* btnRxOn  = new QPushButton("Recibir datos");
    QPushButton* btnRxOff = new QPushButton("No recibir");
    btnRxOn->setCheckable(true);
    btnRxOff->setCheckable(true);

    btnRxOn->setStyleSheet(
        "QPushButton { "
            "color: white; "
            "background-color: black; "
            "padding: 6px 10px; "
            "font-size: 13px; "
            "border-radius: 8px; "
        "} "
        "QPushButton:hover { "
            "background-color: #444; "
        "} "
        "QPushButton:checked { "
            "color: black; "
            "background-color: #00cc44; " 
            "border-color: #00cc44; "
        "}"
    );

    btnRxOff->setStyleSheet(
        "QPushButton { "
            "color: white; "
            "background-color: black; "
            "padding: 6px 10px; "
            "font-size: 13px; "
            "border-radius: 8px; "
        "} "
        "QPushButton:hover { "
            "background-color: #444; "
        "} "
        "QPushButton:checked { "
            "background-color: #b33939; "
            "border-color: #b33939; "
        "}"
    );

    QButtonGroup* rxGroup = new QButtonGroup(this);
    rxGroup->setExclusive(true);
    rxGroup->addButton(btnRxOn);
    rxGroup->addButton(btnRxOff);

    // Estado inicial
    btnRxOn->setChecked(true);
    m_sensorManager->setReceivingEnabled(true);

    // Conexiones
    connect(btnRxOn, &QPushButton::toggled, this, [this](bool checked){
        if (checked) m_sensorManager->setReceivingEnabled(true);
    });
    connect(btnRxOff, &QPushButton::toggled, this, [this, btnRxOff]() {
        if (btnRxOff->isChecked()) {
            m_sensorManager->setReceivingEnabled(false);
            if (timer && timer->isActive()) {
                timer->stop();
                labelTiempo->setText("Recepción pausada.");
            }
        }
    });

    rightLayout->addWidget(btnRxOn);
    rightLayout->addWidget(btnRxOff);

    // --- Botón de reinicio ---
    QPushButton* btnReset = new QPushButton("Reiniciar");
    btnReset->setStyleSheet(
        "QPushButton { "
            "color: white; "
            "background-color: black; "
            "padding: 6px 10px; "
            "font-size: 13px; "
            "border-radius: 8px; "
        "} "
        "QPushButton:hover { "
            "background-color: #444; "
        "} "
        "QPushButton:pressed { "
            "background-color: #ffaa00; " 
            "border-color: #ffaa00; "
        "}"
    );

    connect(btnReset, &QPushButton::clicked, this, [this, btnRxOn, btnRxOff]() {
        // 1) Respetar el estado actual del usuario (si estaba en "No recibir", NO activar)
        const bool wasReceiving = btnRxOn->isChecked();
        m_sensorManager->setReceivingEnabled(false);

        // 2) Limpiar datos en el manager
        if (m_sensorManager) m_sensorManager->clearData();

        // 3) Reiniciar cronómetro correctamente
        tiempoIniciado = false;
        tiempoInicio = QTime();
        if (timer && timer->isActive()) timer->stop();
        if (timeoutTimer && timeoutTimer->isActive()) timeoutTimer->stop();
        labelTiempo->setText("Tiempo: 00:00:00");

        // 4) Vaciar series y RESETEAR ejes X e Y a 0..1
        for (auto* chartView : findChildren<QChartView*>()) {
            if (auto* chart = chartView->chart()) {
                // Borrar curvas
                for (auto* s : chart->series()) {
                    if (auto* line = qobject_cast<QLineSeries*>(s)) line->clear();
                }
                // Ejes X
                for (auto* ax : chart->axes(Qt::Horizontal)) {
                    if (auto* v = qobject_cast<QValueAxis*>(ax)) v->setRange(0.0, 1.0);
                }
                // Ejes Y
                for (auto* ax : chart->axes(Qt::Vertical)) {
                    if (auto* v = qobject_cast<QValueAxis*>(ax)) v->setRange(0.0, 1.0);
                }
            }
            if (auto* hview = qobject_cast<HoverChartView*>(chartView)) {
                hview->setAutoFollow(true);
            }
        }

        // 5) Restablecer textos
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

        for (int i = 0; i < 6; ++i) if (labelStatus[i]) labelStatus[i]->setText("Esperando...");
        if (labelRaw) labelRaw->setText("Paquete: Esperando...");
        for (int i = 0; i < 6; ++i) if (labelServos[i]) labelServos[i]->setText("0°");

        // 6) Pedir que el manejador de datos reinicie su índice de tiempo 't'
        resetTimeBase_ = true;

        // 7) Volver al estado de recepción que eligió el usuario
        m_sensorManager->setReceivingEnabled(wasReceiving);

        // Salir de modo archivo
        if (labelCom)  { labelCom->setProperty("fileMode", false);  labelCom->setText("Esperando..."); }
        if (labelBaud) { labelBaud->setProperty("fileMode", false); labelBaud->setText("Esperando..."); }
        setWindowTitle("FRECCIA_XAE - Gráficas 2D");

        // Rehabilitar toggles de recepción
        if (btnRxOn)  btnRxOn->setEnabled(true);
        if (btnRxOff) btnRxOff->setEnabled(true);

        // Si veníamos de modo archivo, limpiar bandera y restaurar labels/título
        if (labelCom && labelCom->property("fileMode").toBool()) {
            labelCom->setProperty("fileMode", false);
            labelCom->setText("Esperando...");
        }
        if (labelBaud && labelBaud->property("fileMode").toBool()) {
            labelBaud->setProperty("fileMode", false);
            labelBaud->setText("Esperando...");
        }
        setWindowTitle("FRECCIA_XAE - Gráficas 2D");
        modoArchivo_ = false;

        if (ventanaGraph3D) {
            ventanaGraph3D->resetData();
        }
    });

    // Agregar al layout
    rightLayout->addWidget(btnReset);

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

    // --- Ver antiguos: abrir CSV y poblar gráficas ---
    auto abrirAntiguos = [this, btnRecord, btnStop, btnRxOn, btnRxOff, btnMenu, btnVerAntiguos, btnReset]() {
        const QString file = QFileDialog::getOpenFileName(
            this,
            tr("Abrir sesión CSV"),
            "../data",
            tr("CSV (*.csv)")
        );
        if (file.isEmpty()) return;

        // Pausar recepción y reiniciar base de tiempo
        m_sensorManager->setReceivingEnabled(false);

        if (timer && timer->isActive()) timer->stop();
        tiempoIniciado = false;
        tiempoInicio = QTime();
        if (labelTiempo) labelTiempo->setText("Tiempo: 00:00:00");

        // Asegurar que 't' empiece en 0 en el slot de graficación
        resetTimeBase_ = true;

        // Limpiar series y devolver ejes X a 0..1
        for (auto& p : seriesAndXAxis) {
            if (p.first)  p.first->clear();
            if (p.second) p.second->setRange(0.0, 1.0);
        }
        
        // Cargar CSV y emitir datos hacia las gráficas
        m_sensorManager->loadFromCsv(file);
        modoArchivo_ = true;

        // Parar timers para que no sobreescriban títulos
        if (timer && timer->isActive())         timer->stop();
        if (timeoutTimer && timeoutTimer->isActive()) timeoutTimer->stop();

        // Mostrar COM/Baud como "No aplica" y marcar modo archivo (hasta Reiniciar)
        if (labelCom)  { labelCom->setText(" No aplica");        labelCom->setProperty("fileMode", true); }
        if (labelBaud) { labelBaud->setText(" No aplica"); labelBaud->setProperty("fileMode", true); }

        // Fijar título con el nombre del CSV
        setWindowTitle(QString("FRECCIA_XAE - Gráficas 2D (CSV: %1)").arg(QFileInfo(file).fileName()));

        // Dejar los controles en modo "solo lectura de archivo"
        m_sensorManager->setReceivingEnabled(false);
        if (btnRxOff)   btnRxOff->setChecked(true);
        if (btnRxOn)    btnRxOn->setEnabled(false);
        if (btnRxOff)   btnRxOff->setEnabled(false);
        if (btnRecord)  btnRecord->setEnabled(false);
        if (btnStop)    btnStop->setEnabled(false);
        if (btnMenu)    btnMenu->setEnabled(true);
        if (btnVerAntiguos) btnVerAntiguos->setEnabled(true);

        // Mostrar en el label el tiempo TOTAL (columna 0 = "Hora" en segundos) + nombre del documento
        if (labelTiempo) {
            QFile f2(file);
            if (f2.open(QIODevice::ReadOnly | QIODevice::Text)) {
                QTextStream in(&f2);
        #if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
                in.setEncoding(QStringConverter::Utf8);
        #endif
            QString last;
            while (!in.atEnd()) {
                const QString line = in.readLine().trimmed();
                if (line.isEmpty()) continue;
                if (line.startsWith("Hora,")) continue; // saltar encabezado
                last = line;
            }
            f2.close();

            if (!last.isEmpty()) {
                const QStringList v = last.split(',', Qt::KeepEmptyParts);
                bool ok = false;
                const double secs = v.value(0).toDouble(&ok); // "Hora" relativa
                if (ok) {
                    const int msec = int(std::llround(secs * 1000.0));
                    const QTime dur = QTime(0,0,0).addMSecs(msec);
                    labelTiempo->setText(
                        tr("Tiempo total: %1   |   Datos Documento: \"%2\"")
                        .arg(dur.toString("hh:mm:ss"))
                        .arg(QFileInfo(file).fileName())
                    );
                }
            }
        }
    }
    };

    connect(btnVerAntiguos, &QPushButton::clicked, this, abrirAntiguos);

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

    // --- Menú: acciones de configuración y cerrar ---
    menuDesplegable->addSeparator();

    QAction* configCom  = menuDesplegable->addAction("Configurar COM y Baud");
    QAction* configAcc  = menuDesplegable->addAction("Calibrar Acelerómetro");
    QAction* configGyro = menuDesplegable->addAction("Calibrar Giroscopio");

    menuDesplegable->addSeparator();

    QWidgetAction* cerrarWidgetAction = new QWidgetAction(this);
    QPushButton* cerrarBtn = new QPushButton("Close the program");
    cerrarBtn->setStyleSheet("color: white; background-color: red; border: none; padding: 4px;");
    cerrarWidgetAction->setDefaultWidget(cerrarBtn);
    menuDesplegable->addAction(cerrarWidgetAction);

    // Ventana de selección de puerto/baud
    connect(configCom, &QAction::triggered, this, [this]() {
        abrirDialogoSerial();
    });

    // Mantener sincronizados los labels cuando SensorManager reconfigure el puerto
    connect(m_sensorManager, &SensorManager::serialReconfigured,
            this, [this](const QString& port, int baud, bool ok) {
        // Si estamos en modo archivo, NO toques las etiquetas
        if ((labelCom  && labelCom->property("fileMode").toBool()) ||
            (labelBaud && labelBaud->property("fileMode").toBool())) {
            return;
        }
        if (labelCom)  labelCom->setText(QString(" %1%2").arg(port, ok ? "" : " (error)"));
        if (labelBaud) labelBaud->setText(QString(" %1").arg(baud));
    });

    // Confirmación para cerrar
    connect(cerrarBtn, &QPushButton::clicked, this, []() {
        QMessageBox msgBox;
        msgBox.setIcon(QMessageBox::Critical);
        msgBox.setWindowTitle("Exit Confirmation");
        msgBox.setText("Are you sure about closing the program?");
        msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
        msgBox.setDefaultButton(QMessageBox::No);
        msgBox.setWindowIcon(QIcon("./assets/logo_xae.png"));
        if (msgBox.exec() == QMessageBox::Yes) {
            QCoreApplication::quit();
        }
    });

    btnMenu->setMenu(menuDesplegable);

    // === Añadir a layout superior ===
    topLayout->addWidget(leftButtons);
    topLayout->addStretch();
    topLayout->addWidget(labelTiempo);
    topLayout->addStretch();
    topLayout->addWidget(rightButtons); 
    topLayout->addWidget(btnMenu);

    // === Añadir barra al layout principal ===
    QVBoxLayout* globalLayout = new QVBoxLayout(this);
    globalLayout->setContentsMargins(0, 0, 0, 0);
    globalLayout->setSpacing(0);
    globalLayout->addWidget(topBar);
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
        hview->setMinimumSize(200, 200);
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
    winLay->setContentsMargins(4, 2, 4, 2);
    winLay->setSpacing(2);

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
        card->setMinimumHeight(40);
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

    // === Tarjetas separadas: "Puerto (COM)" y "Velocidad" lado a lado ===
    QFrame* puertoCard = new QFrame();
    puertoCard->setStyleSheet("background-color: #2c2c2c; border-radius: 10px;");
    puertoCard->setMinimumHeight(40);
    puertoCard->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    QVBoxLayout* puertoCardLayout = new QVBoxLayout(puertoCard);
    puertoCardLayout->setAlignment(Qt::AlignCenter);
    puertoCardLayout->setContentsMargins(8, 4, 8, 4);

    QLabel* tituloPuerto = new QLabel("Puerto");
    tituloPuerto->setStyleSheet("color: white; font-size: 10px;");
    tituloPuerto->setAlignment(Qt::AlignCenter);

    labelCom = new QLabel("Esperando...");
    labelCom->setStyleSheet("color: white; font-weight: bold; font-size: 12px;");
    labelCom->setAlignment(Qt::AlignCenter);

    puertoCardLayout->addWidget(tituloPuerto);
    puertoCardLayout->addWidget(labelCom);

    // ---
    QFrame* baudCard = new QFrame();
    baudCard->setStyleSheet("background-color: #2c2c2c; border-radius: 10px;");
    baudCard->setMinimumHeight(40);
    baudCard->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    QVBoxLayout* baudCardLayout = new QVBoxLayout(baudCard);
    baudCardLayout->setAlignment(Qt::AlignCenter);
    baudCardLayout->setContentsMargins(8, 4, 8, 4);

    QLabel* tituloBaud = new QLabel("Velocidad");
    tituloBaud->setStyleSheet("color: white; font-size: 10px;");
    tituloBaud->setAlignment(Qt::AlignCenter);

    labelBaud = new QLabel("Esperando...");
    labelBaud->setStyleSheet("color: white; font-weight: bold; font-size: 12px;");
    labelBaud->setAlignment(Qt::AlignCenter);

    baudCardLayout->addWidget(tituloBaud);
    baudCardLayout->addWidget(labelBaud);

    // Añade ambas tarjetas a la MISMA fila del grid (una izquierda, otra derecha)
    const int nextRow = campos.size() / 2; // con 6 campos, nextRow = 3
    gridEstado->addWidget(puertoCard, nextRow, 0);
    gridEstado->addWidget(baudCard,   nextRow, 1);

    // Añade el grid de estado al contenedor
    estadoFinal->addLayout(gridEstado);

    // === Línea final con paquete RAW ===
    labelRaw = new QLabel("Paquete: Esperando...\n");
    labelRaw->setStyleSheet("color: white; font-size: 10px; background-color: #1e1e1e; padding: 6px;");
    labelRaw->setWordWrap(true);
    labelRaw->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    labelRaw->setMinimumHeight(24);
    labelRaw->setMaximumHeight(30);
    labelRaw->setMinimumWidth(300);
    labelRaw->setMaximumWidth(300);
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
                ax->setRange(0.0, last);
            } else {
                const double from = std::max(0.0, last - double(windowSec));
                ax->setRange(from, last);
            }
        }
    });

    // === Suscripción a DataTopic ===
   static RangeChecker rangeChecker;

   connect(DataTopic::instance(), &DataTopic::dataPublished, this, [this](const QString& line) {
        // Strip prefix like "#6456: " if present
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
            if (hview && hview->autoFollow()) {
                if (windowSec == 0) {
                    // Máx: mostrar todo lo acumulado desde t = 0 hasta t
                    axisX->setRange(0, t);
                } else {
                    // Ventana deslizante: últimos windowSec segundos
                    axisX->setRange(std::max(0, t - windowSec), t);
                }
            }

            // 6) reajustamos ejes Y dinámicamente
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
        labelStatus[4]->setText(QDate::currentDate().toString("dd-MMMM-yyyy"));
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
                qDebug() << "Señal recuperada. Reanudando cronómetro.";
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

void Widget::abrirDialogoSerial() {
    auto leerPuertoActual = [this]() -> QString {

        // Espera formato "COM: <puerto>" o "COM: <puerto> (error)"
        QString txt = labelCom && !labelCom->text().isEmpty() ? labelCom->text() : QString();

        // quita prefijo "COM: "
        if (txt.startsWith("COM: ")) txt = txt.mid(5);

        // quita los errores
        txt.replace(" (error)", "");
        return txt.trimmed();
    };
    auto leerBaudActual = [this]() -> int {

        // Espera formato "Velocidad: <baud>"
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

    // bauds predefinidos
    const QList<int> bauds = {9600, 19200, 38400, 57600, 115200, 230400, 460800};
    for (int b : bauds) cbBaud->addItem(QString::number(b), b);

    // Puertos disponibles según OS
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

    // --- layout ---
    auto* lay = new QFormLayout(&dlg);
    lay->setContentsMargins(20, 20, 20, 20);
    lay->setSpacing(14);
    lay->addRow("Sistema:", cbOS);
    lay->addRow("Puerto:",  cbPuertos);
    lay->addRow("Baud:",    cbBaud);
    lay->addRow(btns);

    // --- valores actuales para preseleccionar ---
    const QString puertoActual = leerPuertoActual();
    const int     baudActual   = leerBaudActual();

    #ifdef Q_OS_WIN
        cbOS->setCurrentText("Windows");
    #else
        cbOS->setCurrentText("Linux");
    #endif

    poblarPuertos(cbOS->currentText(), puertoActual);

    // preselección de baud
    int idxBaud = cbBaud->findData(baudActual);
    cbBaud->setCurrentIndex(idxBaud >= 0 ? idxBaud : cbBaud->findData(115200));

    // Cambios en OS o puerto
    connect(cbOS, &QComboBox::currentTextChanged, &dlg, [&, this](const QString& osName){
        const QString textoActual = cbPuertos->currentText().trimmed();
        poblarPuertos(osName, textoActual.isEmpty() ? puertoActual : textoActual);
    });

    // botones
    QObject::connect(btns, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    QObject::connect(btns, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);

    // Aplicacion de cambios
    if (dlg.exec() == QDialog::Accepted) {
        const QString port = cbPuertos->currentText().trimmed();
        const int baud     = cbBaud->currentData().toInt();
        if (port.isEmpty() || baud <= 0) {
            QMessageBox::warning(this, "Configurar COM", "Selecciona un puerto y un baudrate válido.");
            return;
        }

        const bool ok = m_sensorManager->setSerial(port, baud);

        // Actualización inmediata de labels (además del signal serialReconfigured)
        labelCom->setText(QString(" %1%2").arg(port, ok ? "" : " (error)"));
        labelBaud->setText(QString(" %1").arg(baud));

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
