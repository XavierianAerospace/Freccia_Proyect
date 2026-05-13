#ifndef INICIOWINDOW_H
#define INICIOWINDOW_H

#include <QWidget>
#include <QCheckBox>
#include <QPushButton>
#include <QLabel>
#include <QHBoxLayout>
#include <QProcess> 

class InicioWindow : public QWidget {
    Q_OBJECT

public:
    explicit InicioWindow(QWidget* parent = nullptr);

signals:
    void iniciar(bool abrir2D, bool abrir3D);

private slots:
    void checkSelection();
    void onError();
    void onFinished(int exitCode, QProcess::ExitStatus exitStatus); 

private:
    QCheckBox* check2D;
    QCheckBox* check3D;
    QCheckBox* checkPy;
    QPushButton* btnIniciar;
    QPushButton* btnExit;
    QLabel* gifLabel;
    QProcess* process;

    void setupStyle();
};

#endif // INICIOWINDOW_H
