#include "TopToolbar.h"
#include "WindowManager.h"
#include "widget.h"
#include "Graph3DWindow.h"
#include <QHBoxLayout>
#include <QMenu>
#include <QWidgetAction>
#include <QFileDialog>
#include <QMessageBox>
#include <QCoreApplication>
#include <QTimer>

TopToolbar::TopToolbar(QWidget* parent) : QWidget(parent) {
    setupUI();
    setupConnections();

    // Sync initial state
    WindowManager* wm = WindowManager::instance();
    updateRecordingStatus(wm->isRecording());
    updateReceptionStatus(wm->isReceivingEnabled());
    updateModoArchivo(wm->modoArchivo(), wm->archivoActual());
    updateTimerLabel(wm->tiempoGrabacionTexto());
    labelTiempo->setText("Tiempo: " + wm->sessionTimerTexto());

    // Detect which window we are in to set menu icons
    if (qobject_cast<Widget*>(parent)) {
        m_pantalla1Activa = true;
    } else if (qobject_cast<Graph3DWindow*>(parent)) {
        m_pantalla1Activa = false;
    }
    actualizarEstilosMenu();
}

void TopToolbar::setupUI() {
    setFixedHeight(40);
    setStyleSheet("background-color: black; color: white;");

    QHBoxLayout* layout = new QHBoxLayout(this);
    layout->setContentsMargins(5, 0, 5, 0);

    btnRecord = new QPushButton("● Grabar");
    btnRecord->setStyleSheet(
        "QPushButton { color: white; background-color: red; border: none; padding: 6px 14px; font-size: 15px; border-radius: 8px; }"
        "QPushButton:hover { background-color: darkred; }"
    );

    btnStop = new QPushButton("■ Detener");
    btnStop->setStyleSheet(
        "QPushButton { color: white; background-color: transparent; border: none; padding: 6px 14px; font-size: 15px; border-radius: 8px; }"
        "QPushButton:hover { background-color: #444; }"
    );
    btnStop->setEnabled(false);

    btnVerAntiguos = new QPushButton("Ver antiguos");
    btnVerAntiguos->setStyleSheet(
        "QPushButton { color: white; background-color: transparent; border: none; padding: 6px 14px; font-size: 15px; border-radius: 8px; }"
        "QPushButton:hover { background-color: #444; }"
    );

    layout->addWidget(btnRecord);
    layout->addWidget(btnStop);
    layout->addWidget(btnVerAntiguos);
    layout->addStretch();

    labelTiempo = new QLabel("Tiempo: 00:00:00");
    labelTiempo->setStyleSheet("color: white; font-weight: bold; font-size: 18px;");
    layout->addWidget(labelTiempo);
    layout->addStretch();

    btnRxOn = new QPushButton("Recibir datos");
    btnRxOff = new QPushButton("No recibir");
    btnRxOn->setCheckable(true);
    btnRxOff->setCheckable(true);

    QString rxStyle = R"(
        QPushButton { color: white; background-color: black; padding: 6px 10px; font-size: 13px; border-radius: 8px; }
        QPushButton:hover { background-color: #444; }
        QPushButton:checked#btnRxOn { color: black; background-color: #00cc44; }
        QPushButton:checked#btnRxOff { background-color: #b33939; }
    )";
    btnRxOn->setObjectName("btnRxOn");
    btnRxOff->setObjectName("btnRxOff");
    btnRxOn->setStyleSheet(rxStyle);
    btnRxOff->setStyleSheet(rxStyle);
    btnRxOn->setChecked(true);

    btnReset = new QPushButton("Reiniciar");
    btnReset->setStyleSheet(
        "QPushButton { color: white; background-color: black; padding: 6px 10px; font-size: 13px; border-radius: 8px; }"
        "QPushButton:hover { background-color: #444; }"
        "QPushButton:pressed { background-color: #ffaa00; }"
    );

    btnMenu = new QPushButton();
    btnMenu->setIcon(QIcon("./assets/Menu.png"));
    btnMenu->setIconSize(QSize(35, 35));
    btnMenu->setStyleSheet("background-color: transparent; border: none;");

    QMenu* menu = new QMenu(this);
    menu->setStyleSheet(R"(
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

    action2D = menu->addAction("Pantalla Gráficas 2D");
    action3D = menu->addAction("Pantalla Gráficas 3D y OSM");
    menu->addSeparator();
    actionConfigCom = menu->addAction("Configurar COM y Baud");
    actionCalibrarAcc = menu->addAction("Calibrar Acelerómetro");
    actionCalibrarGyro = menu->addAction("Calibrar Giroscopio");
    menu->addSeparator();

    QWidgetAction* cerrarWidgetAction = new QWidgetAction(this);
    QPushButton* cerrarBtn = new QPushButton("Close the program");
    cerrarBtn->setStyleSheet("color: white; background-color: red; border: none; padding: 4px;");
    cerrarWidgetAction->setDefaultWidget(cerrarBtn);
    menu->addAction(cerrarWidgetAction);

    btnMenu->setMenu(menu);

    layout->addWidget(btnRxOn);
    layout->addWidget(btnRxOff);
    layout->addWidget(btnReset);
    layout->addWidget(btnMenu);

    connect(cerrarBtn, &QPushButton::clicked, this, &TopToolbar::onActionCerrarTriggered);
}

void TopToolbar::setupConnections() {
    WindowManager* wm = WindowManager::instance();

    connect(wm, &WindowManager::recordingStatusChanged, this, &TopToolbar::updateRecordingStatus);
    connect(wm, &WindowManager::recordingTimerUpdated, this, &TopToolbar::updateTimerLabel);
    connect(wm, &WindowManager::sessionTimerUpdated, this, [this](const QString& t){
        if (!WindowManager::instance()->modoArchivo()) {
            labelTiempo->setText("Tiempo: " + t);
        }
    });
    connect(wm, &WindowManager::receptionStatusChanged, this, &TopToolbar::updateReceptionStatus);
    connect(wm, &WindowManager::modoArchivoChanged, this, &TopToolbar::updateModoArchivo);

    connect(btnRecord, &QPushButton::clicked, this, &TopToolbar::onBtnRecordClicked);
    connect(btnStop, &QPushButton::clicked, this, &TopToolbar::onBtnStopClicked);
    connect(btnVerAntiguos, &QPushButton::clicked, this, &TopToolbar::onBtnVerAntiguosClicked);
    connect(btnRxOn, &QPushButton::clicked, this, [wm](){ wm->setReceivingEnabled(true); });
    connect(btnRxOff, &QPushButton::clicked, this, [wm](){ wm->setReceivingEnabled(false); });
    connect(btnReset, &QPushButton::clicked, this, &TopToolbar::onBtnResetClicked);

    connect(action2D, &QAction::triggered, this, &TopToolbar::onAction2DTriggered);
    connect(action3D, &QAction::triggered, this, &TopToolbar::onAction3DTriggered);
    connect(actionConfigCom, &QAction::triggered, this, &TopToolbar::onActionConfigComTriggered);
}

void TopToolbar::actualizarEstilosMenu() {
    action2D->setEnabled(!m_pantalla1Activa);
    action3D->setEnabled(m_pantalla1Activa);
}

void TopToolbar::updateRecordingStatus(bool recording) {
    if (recording) {
        btnRecord->setStyleSheet(
            "QPushButton { color: red; background-color: black; border: none; padding: 6px 14px; font-size: 15px; border-radius: 8px; }"
            "QPushButton:hover { background-color: #222; }"
        );
        btnRecord->setEnabled(false);
        btnStop->setEnabled(true);
    } else {
        btnRecord->setText("● Grabar");
        btnRecord->setStyleSheet(
            "QPushButton { color: white; background-color: red; border: none; padding: 6px 14px; font-size: 15px; border-radius: 8px; }"
            "QPushButton:hover { background-color: darkred; }"
        );
        btnRecord->setEnabled(true);
        btnStop->setEnabled(false);
    }
}

void TopToolbar::updateTimerLabel(const QString& tiempo) {
    if (WindowManager::instance()->isRecording()) {
        btnRecord->setText("● Grabando " + tiempo);
    }
}

void TopToolbar::updateReceptionStatus(bool enabled) {
    btnRxOn->setChecked(enabled);
    btnRxOff->setChecked(!enabled);
}

void TopToolbar::updateModoArchivo(bool enabled, const QString& fileName) {
    if (enabled) {
        btnRxOn->setEnabled(false);
        btnRxOff->setEnabled(false);
        btnRecord->setEnabled(false);
        btnStop->setEnabled(false);
        labelTiempo->setText("Archivo: " + fileName);
    } else {
        btnRxOn->setEnabled(true);
        btnRxOff->setEnabled(true);
        btnRecord->setEnabled(true);
        btnStop->setEnabled(false);
        labelTiempo->setText("Tiempo: " + WindowManager::instance()->sessionTimerTexto());
    }
}

void TopToolbar::updateSerialConfig(const QString& port, int baud, bool ok) {
}

void TopToolbar::onBtnRecordClicked() {
    WindowManager::instance()->iniciarGrabacion();
}

void TopToolbar::onBtnStopClicked() {
    WindowManager::instance()->detenerGrabacion();
}

void TopToolbar::onBtnVerAntiguosClicked() {
    QString file = QFileDialog::getOpenFileName(this, "Abrir sesión CSV", "../data", "CSV (*.csv)");
    if (file.isEmpty()) return;

    WindowManager::instance()->setModoArchivo(true, QFileInfo(file).fileName());
    if (WindowManager::instance()->sensorManager()) {
        WindowManager::instance()->sensorManager()->loadFromCsv(file);
    }
}

void TopToolbar::onBtnResetClicked() {
    WindowManager::instance()->requestReset();
    WindowManager::instance()->setModoArchivo(false);
}

void TopToolbar::onAction2DTriggered() {
    Widget* w = WindowManager::instance()->widget();
    if (w) {
        w->show();
        w->raise();
        w->activateWindow();
    } else if (WindowManager::instance()->sensorManager()) {
        w = new Widget(WindowManager::instance()->sensorManager());
        w->resize(1280, 720);
        w->show();
    }
}

void TopToolbar::onAction3DTriggered() {
    Graph3DWindow* g = WindowManager::instance()->graph3D();
    if (g) {
        g->show();
        g->raise();
        g->activateWindow();
    } else if (WindowManager::instance()->sensorManager()) {
        g = new Graph3DWindow(WindowManager::instance()->sensorManager());
        g->resize(1280, 720);
        g->show();
    }
}

void TopToolbar::onActionConfigComTriggered() {
    WindowManager::instance()->abrirDialogoSerial();
}

void TopToolbar::onActionCerrarTriggered() {
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
}
