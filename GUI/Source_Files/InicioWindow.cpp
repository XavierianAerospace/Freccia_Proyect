#include "InicioWindow.h"
#include <QMovie>
#include <QProcess>
#include <QDebug>
#include <QFile>
#include <QFileInfo>

InicioWindow::InicioWindow(QWidget* parent) : QWidget(parent) {
    setWindowTitle("FRECCIA_XAE - Inicio");
    setFixedSize(600, 400);
    setWindowIcon(QIcon("./assets/logo_xae.png"));

    // === Título superior ===
    QLabel* titulo = new QLabel("Bienvenido a la interfaz gráfica del proyecto FRECCIA");
    titulo->setAlignment(Qt::AlignCenter);
    titulo->setStyleSheet("color: white; font-size: 16px; font-weight: bold;");

    // === Logo izquierdo ===
    QLabel* logo = new QLabel;
    QPixmap logoPixmap("./assets/logo_xae.png");
    if (!logoPixmap.isNull()) {
        logo->setPixmap(logoPixmap.scaledToHeight(180, Qt::SmoothTransformation));
    } else {
        qDebug() << "Error: No se cargó el logo desde el sistema de archivos.";
    }

    // === Checkboxes ===
    check2D = new QCheckBox("Gráficas  Principales 2D");
    check3D = new QCheckBox("Gráficas Secundarias 3D");
    checkPy = new QCheckBox("Graficas Map - Angulo Ataque");
    connect(check2D, &QCheckBox::stateChanged, this, &InicioWindow::checkSelection);
    connect(check3D, &QCheckBox::stateChanged, this, &InicioWindow::checkSelection);
    connect(checkPy, &QCheckBox::stateChanged, this, &InicioWindow::checkSelection);

    // === Botones ===
    btnIniciar = new QPushButton("Iniciar Simulación");
    btnIniciar->setEnabled(false);
    btnExit = new QPushButton("Exit");
    connect(btnExit, &QPushButton::clicked, this, &QWidget::close);
    connect(btnIniciar, &QPushButton::clicked, this, [=]() {
        if (checkPy->isChecked()) {
            QString rutaExe = "./Source_Py/dist/FRECCIA_XAE.exe";
            QFileInfo fileInfo(rutaExe);
            QString rutaAbsoluta = fileInfo.absoluteFilePath();
            
            qDebug() << "[LOG] Intentando ejecutar:" << rutaAbsoluta;
            
            if (fileInfo.exists()) {
                bool exito = QProcess::startDetached(rutaAbsoluta);
                
                if (exito) {
                    qDebug() << "[LOG] Ejecutable FRECCIA_XAE iniciado correctamente";
                    qDebug() << "[LOG] Ruta:" << rutaAbsoluta;
                } else {
                    qDebug() << "[ERROR] No se pudo iniciar el ejecutable";
                }
            } else {
                qDebug() << "[ERROR] No se encontró el archivo en" << rutaAbsoluta;
            }
            // NO hacemos close() aquí - la ventana de inicio se mantiene abierta
            // Solo para los casos 2D/3D - aquí sí cerramos la ventana
        }
        emit iniciar(check2D->isChecked(), check3D->isChecked());
        close();
    });

    // === Layout vertical del logo ===
    QVBoxLayout* logoLayout = new QVBoxLayout;
    logoLayout->addStretch();  // centra verticalmente dentro del espacio disponible
    logoLayout->addWidget(logo, 0, Qt::AlignHCenter);  // horizontalmente centrado
    logoLayout->addStretch();

    // === Layout vertical de los controles ===
    QVBoxLayout* controlesLayout = new QVBoxLayout;
    controlesLayout->addWidget(check2D);
    controlesLayout->addWidget(check3D);
    controlesLayout->addWidget(checkPy); 
    controlesLayout->addSpacing(10);
    controlesLayout->addWidget(btnIniciar);
    controlesLayout->addWidget(btnExit);
    controlesLayout->setAlignment(Qt::AlignVCenter);  // centra verticalmente los controles

    // === Layout horizontal principal (logo a la izquierda, controles a la derecha) ===
    QHBoxLayout* mainLayout = new QHBoxLayout;
    mainLayout->addStretch();  // espacio vacío izquierda
    mainLayout->addLayout(logoLayout);
    mainLayout->addSpacing(40);  // espacio entre logo y controles
    mainLayout->addLayout(controlesLayout);
    mainLayout->addStretch();  // espacio vacío derecha

    // === GIF inferior ===
    gifLabel = new QLabel;
    gifLabel->setFixedHeight(120);
    gifLabel->setAlignment(Qt::AlignCenter);
    gifLabel->setStyleSheet("background: transparent;");
    QMovie* movie = new QMovie("./assets/Inicio.gif");
    if (!movie->isValid()) {
        qDebug() << "Error: GIF no válido.";
    } else {
        gifLabel->setMovie(movie);
        movie->start();
    }

    // === Layout final (todo incluido) ===
    QVBoxLayout* finalLayout = new QVBoxLayout;
    finalLayout->addWidget(titulo);
    finalLayout->addSpacing(30);
    finalLayout->addLayout(mainLayout);
    finalLayout->addSpacing(10);
    finalLayout->addWidget(gifLabel);

    setLayout(finalLayout);

    // === Estilo general ===
    setupStyle();
    qDebug() << "Logo visible:" << !logoPixmap.isNull();
    qDebug() << "GIF cargado:" << movie->isValid();
}

void InicioWindow::checkSelection() {
    btnIniciar->setEnabled(check2D->isChecked() || check3D->isChecked() || checkPy->isChecked());
}

void InicioWindow::setupStyle() {
    QString style = R"(
        QWidget {
            background-color:rgb(0, 0, 0);
            color: white;
        }
        QPushButton {
            background-color: white;
            color: black;
            border-radius: 6px;
            padding: 8px 16px;
            font-weight: bold;
        }
        QPushButton:hover {
            background-color: orange;
            color: black;
        }
        QPushButton#btnExit {
            background-color: red;
            color: white;
        }
        QPushButton#btnExit:hover {
            background-color: darkred;
        }
    )";

    setStyleSheet(style);
    btnExit->setObjectName("btnExit");
}

void InicioWindow::onError() {
    QProcess *process = qobject_cast<QProcess*>(sender());
    if (process) {
        qDebug() << "Error al ejecutar el script Python: " << process->readAllStandardError();
    }
}

void InicioWindow::onFinished(int exitCode, QProcess::ExitStatus exitStatus) {
    qDebug() << "El script Python ha finalizado con código de salida:" << exitCode;
    if (exitStatus == QProcess::CrashExit) {
        qDebug() << "El proceso Python terminó inesperadamente.";
    } else {
        qDebug() << "El proceso Python terminó correctamente.";
    }
}

