#ifndef TOPTOOLBAR_H
#define TOPTOOLBAR_H

#include <QWidget>
#include <QPushButton>
#include <QLabel>
#include <QAction>
#include <QTime>

class TopToolbar : public QWidget {
    Q_OBJECT

public:
    explicit TopToolbar(QWidget* parent = nullptr);

private slots:
    void updateRecordingStatus(bool recording);
    void updateTimerLabel(const QString& tiempo);
    void updateReceptionStatus(bool enabled);
    void updateModoArchivo(bool enabled, const QString& fileName);
    void updateSerialConfig(const QString& port, int baud, bool ok);

    void onBtnRecordClicked();
    void onBtnStopClicked();
    void onBtnVerAntiguosClicked();
    void onBtnResetClicked();
    void onAction2DTriggered();
    void onAction3DTriggered();
    void onActionConfigComTriggered();
    void onActionCerrarTriggered();

private:
    void setupUI();
    void setupConnections();
    void actualizarEstilosMenu();

    QPushButton* btnRecord;
    QPushButton* btnStop;
    QPushButton* btnVerAntiguos;
    QLabel* labelTiempo;
    QPushButton* btnRxOn;
    QPushButton* btnRxOff;
    QPushButton* btnReset;
    QPushButton* btnMenu;

    QAction* action2D;
    QAction* action3D;
    QAction* actionConfigCom;
    QAction* actionCalibrarAcc;
    QAction* actionCalibrarGyro;

    bool m_pantalla1Activa = true;
};

#endif // TOPTOOLBAR_H
