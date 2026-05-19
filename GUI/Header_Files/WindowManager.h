#ifndef WINDOWMANAGER_H
#define WINDOWMANAGER_H

#include <QObject>
#include <QTime>
#include <QPointer>
#include <QProcess>
#include "SensorManager.h"
#include "data/FileHelper.h"

class Widget;
class Graph3DWindow;
class QTimer;

class WindowManager : public QObject {
    Q_OBJECT

public:
    static WindowManager* instance();

    void setSensorManager(SensorManager* manager);
    SensorManager* sensorManager() const { return m_sensorManager; }

    void registerWidget(Widget* w);
    void registerGraph3D(Graph3DWindow* g);

    Widget* widget() const;
    Graph3DWindow* graph3D() const;

    // Recording logic
    void iniciarGrabacion();
    void detenerGrabacion();
    bool isRecording() const { return m_recording; }
    QString tiempoGrabacionTexto() const;

    // Reception logic
    void setReceivingEnabled(bool enabled);
    bool isReceivingEnabled() const;

    // Session Timer logic
    void setSessionTimerText(const QString& text);
    QString sessionTimerTexto() const { return m_sessionTimer; }

    // Reset logic
    void requestReset();

    // Data handling
    void procesarDatos(const SensorData& data);

    // Mode logic
    void setModoArchivo(bool enabled, const QString& fileName = "");
    bool modoArchivo() const { return m_modoArchivo; }
    QString archivoActual() const { return m_archivoActual; }

    // External process tracking
    void setMapProcess(QProcess* process);
    bool isMapActive() const;

    // Serial Dialog
    void abrirDialogoSerial();

signals:
    void recordingStatusChanged(bool recording);
    void recordingTimerUpdated(const QString& tiempo);
    void sessionTimerUpdated(const QString& tiempo);
    void receptionStatusChanged(bool enabled);
    void dataResetRequested();
    void modoArchivoChanged(bool enabled, const QString& fileName);
    void serialConfigChanged(const QString& port, int baud, bool ok);

private:
    explicit WindowManager(QObject* parent = nullptr);
    ~WindowManager();

    static WindowManager* m_instance;

    SensorManager* m_sensorManager = nullptr;
    QPointer<Widget> m_widget;
    QPointer<Graph3DWindow> m_graph3D;
    QPointer<QProcess> m_mapProcess;

    FileHelper* m_fileHelper = nullptr;
    QTimer* m_timerGrabacion = nullptr;
    QTime m_tiempoGrabacion;
    bool m_recording = false;
    bool m_modoArchivo = false;
    QString m_archivoActual;
    QString m_sessionTimer = "00:00:00";
};

#endif // WINDOWMANAGER_H
