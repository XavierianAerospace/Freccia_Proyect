#include "widget.h"
#include "Graph3DWindow.h"
#include "InicioWindow.h" 

#include <QApplication>

int main(int argc, char *argv[]) {
    QApplication a(argc, argv);

    InicioWindow inicio;
    inicio.resize(300, 150);
    inicio.show();

    QObject::connect(&inicio, &InicioWindow::iniciar, [&](bool abrir2D, bool abrir3D) {

        SensorManager* manager = new SensorManager();
        
        if (abrir2D) {
            Widget* w = new Widget(manager);
            w->resize(1280, 720);
            w->show();

            if (abrir3D) {
                w->abrirVentana3DDesdeExterno();
            }
        }

        if (abrir3D) {
            Graph3DWindow* g3d = new Graph3DWindow(manager);
            g3d->resize(1280, 720);
            g3d->show();
        }
    });

    return a.exec();
}
