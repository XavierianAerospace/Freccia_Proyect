#include "WindowManager.h"
#include "widget.h"
#include "Graph3DWindow.h"
#include <QTimer>
#include <QDebug>

WindowManager* WindowManager::m_instance = nullptr;

WindowManager* WindowManager::instance() {
    if (!m_instance) {
        m_instance = new WindowManager();
    }
    return m_instance;
}

WindowManager::WindowManager(QObject* parent) : QObject(parent) {
    m_fileHelper = new FileHelper();
    m_timerGrabacion = new QTimer(this);

    connect(m_timerGrabacion, &QTimer::timeout, this, [this]() {
        m_tiempoGrabacion = m_tiempoGrabacion.addSecs(1);
        m_fileHelper->tiempoGrabado = m_tiempoGrabacion;
        emit recordingTimerUpdated(m_tiempoGrabacion.toString("hh:mm:ss"));
    });
}

WindowManager::~WindowManager() {
    delete m_fileHelper;
}

void WindowManager::setSensorManager(SensorManager* manager) {
    m_sensorManager = manager;
    if (m_sensorManager) {
        connect(m_sensorManager, &SensorManager::serialReconfigured,
                this, &WindowManager::serialConfigChanged);
    }
}

void WindowManager::registerWidget(Widget* w) {
    m_widget = w;
}

void WindowManager::registerGraph3D(Graph3DWindow* g) {
    m_graph3D = g;
}

Widget* WindowManager::widget() const {
    return m_widget;
}

Graph3DWindow* WindowManager::graph3D() const {
    return m_graph3D;
}

void WindowManager::iniciarGrabacion() {
    if (m_recording) return;

    m_fileHelper->iniciarGrabacion();
    m_tiempoGrabacion = QTime(0, 0, 0);
    m_timerGrabacion->start(1000);
    m_recording = true;

    emit recordingStatusChanged(true);
    emit recordingTimerUpdated("00:00:00");
}

void WindowManager::detenerGrabacion() {
    if (!m_recording) return;

    m_fileHelper->detenerGrabacion();
    m_timerGrabacion->stop();
    m_recording = false;

    emit recordingStatusChanged(false);
}

QString WindowManager::tiempoGrabacionTexto() const {
    return m_tiempoGrabacion.toString("hh:mm:ss");
}

void WindowManager::setReceivingEnabled(bool enabled) {
    if (m_sensorManager) {
        m_sensorManager->setReceivingEnabled(enabled);
        emit receptionStatusChanged(enabled);
    }
}

bool WindowManager::isReceivingEnabled() const {
    if (m_sensorManager) {
        return m_sensorManager->isReceivingEnabled();
    }
    return true;
}

void WindowManager::updateSessionTimer(const QTime& time) {
    m_sessionTimer = time.toString("hh:mm:ss");
    emit sessionTimerUpdated(m_sessionTimer);
}

void WindowManager::requestReset() {
    if (m_sensorManager) {
        m_sensorManager->clearData();
    }
    emit dataResetRequested();
}

void WindowManager::procesarDatos(const SensorData& data) {
    if (m_recording) {
        m_fileHelper->escribirDuranteGrabacion(data);
    }
}

void WindowManager::setModoArchivo(bool enabled, const QString& fileName) {
    m_modoArchivo = enabled;
    m_archivoActual = fileName;
    emit modoArchivoChanged(enabled, fileName);
}

void WindowManager::setMapProcess(QProcess* process) {
    m_mapProcess = process;
}

bool WindowManager::isMapActive() const {
    return m_mapProcess && m_mapProcess->state() == QProcess::Running;
}

void WindowManager::abrirDialogoSerial() {
    if (m_widget) {
        QMetaObject::invokeMethod(m_widget, "abrirDialogoSerial");
    } else if (m_graph3D) {
        // We could move the dialog logic here to make it truly independent,
        // but it requires a lot of UI code. For now, we use Widget if available.
        // If not, we might need to instantiate a hidden Widget or refactor the dialog.
    }
}
