#ifndef GRAPH3DWINDOW_H
#define GRAPH3DWINDOW_H

#include "SensorManager.h"
#include "TopToolbar.h"
#include "CameraWidget.h"

// Qt Widgets y Layouts
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QTimer>
#include <QPushButton>
#include <QVector3D>
#include <QList>
#include <QMenu>
#include <QAction>

// Qt Data Visualization
#include <QtDataVisualization/Q3DScatter>
#include <QtDataVisualization/QScatter3DSeries>
#include <QtDataVisualization/QScatterDataProxy>
#include <QtDataVisualization/QScatterDataItem>
#include <QGridLayout>

// Qt 3D
#include <Qt3DCore/QEntity>
#include <Qt3DCore/QTransform>
#include <Qt3DExtras/Qt3DWindow>
#include <QCoreApplication>
#include <Qt3DExtras/QOrbitCameraController>
#include <Qt3DRender/QCamera>

class PiPContainer : public QFrame {
    Q_OBJECT
public:
    explicit PiPContainer(QWidget* parent = nullptr) : QFrame(parent) {
        setFrameShape(QFrame::NoFrame);
        m_layout = new QHBoxLayout(this);
        m_layout->setContentsMargins(2, 2, 2, 2);
        m_layout->setSpacing(2);
        setStyleSheet("background-color: rgba(0, 0, 0, 150); border: 1px solid #555; border-radius: 4px;");
        setToolTip("Arrastra para mover. Clic derecho para posiciones rápidas.");
    }
    QHBoxLayout* layout() { return m_layout; }

protected:
    void mousePressEvent(QMouseEvent* event) override {
        if (event->button() == Qt::LeftButton) {
            m_dragging = true;
            m_dragStart = event->globalPosition().toPoint() - frameGeometry().topLeft();
            event->accept();
        } else if (event->button() == Qt::RightButton) {
            showContextMenu(event->globalPosition().toPoint());
        }
    }
    void mouseMoveEvent(QMouseEvent* event) override {
        if (m_dragging && (event->buttons() & Qt::LeftButton)) {
            move(event->globalPosition().toPoint() - m_dragStart);
            event->accept();
        }
    }
    void mouseReleaseEvent(QMouseEvent* event) override {
        m_dragging = false;
        event->accept();
    }

    void showContextMenu(const QPoint& pos) {
        QMenu menu(this);
        menu.setStyleSheet("background-color: #333; color: white;");

        QAction* tl = menu.addAction("Arriba Izquierda");
        QAction* tr = menu.addAction("Arriba Derecha");
        QAction* bl = menu.addAction("Abajo Izquierda");
        QAction* br = menu.addAction("Abajo Derecha");

        connect(tl, &QAction::triggered, this, [this]() { move(10, 10); });
        connect(tr, &QAction::triggered, this, [this]() {
            if (parentWidget()) move(parentWidget()->width() - width() - 10, 10);
        });
        connect(bl, &QAction::triggered, this, [this]() {
            if (parentWidget()) move(10, parentWidget()->height() - height() - 10);
        });
        connect(br, &QAction::triggered, this, [this]() {
            if (parentWidget()) move(parentWidget()->width() - width() - 10, parentWidget()->height() - height() - 10);
        });

        menu.exec(pos);
    }

private:
    QHBoxLayout* m_layout;
    bool m_dragging = false;
    QPoint m_dragStart;
};

class Graph3DWindow : public QWidget {
    Q_OBJECT

public:
    explicit Graph3DWindow(SensorManager* manager, QWidget* parent = nullptr);

public slots:
    void resetData();

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    enum MaximizeMode { Grid, Cam1, Cam2, Cam3, Cam4, Graph3D };
    MaximizeMode m_maximizeMode = Grid;

    SensorManager* m_sensorManager = nullptr;
    TopToolbar* m_topToolbar = nullptr;
    CameraWidget* m_camera1 = nullptr;
    CameraWidget* m_camera2 = nullptr;
    CameraWidget* m_camera3 = nullptr;
    CameraWidget* m_camera4 = nullptr;
    QList<CameraWidget*> m_cameras;
    QGridLayout* m_camLayout = nullptr;

    // === Layout principal ===
    QGridLayout* mainLayout;
    QWidget* m_centralWidget;

    // === Contenedores de visualización ===
    QWidget* containerGeneral2D;
    QWidget* container3D;
    PiPContainer* m_pipContainer = nullptr;
    

    // === Gráfico 3D ===
    Q3DScatter* scatterGraph;
    QScatter3DSeries* mainSeries;
    QScatter3DSeries* trailSeries;
    QVector<QVector3D> pointHistory;
    int xIndex3D = 0;


    // === Series y ejes para gráfica 3D ===
    Qt3DCore::QEntity* rootEntity;
    Qt3DCore::QTransform* rocketTransform;
    Qt3DExtras::Qt3DWindow* view3D;
    QWidget* container3DModel;

    // === Métodos ===
    void applyLayout();

private slots:
    void toggleCameraMaximize(CameraWidget* clickedCamera);

};

#endif // GRAPH3DWINDOW_H
