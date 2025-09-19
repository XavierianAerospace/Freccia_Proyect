import sys
from PySide2.QtWidgets import QApplication, QWidget, QLabel, QVBoxLayout

class VentanaExtra(QWidget):
    def __init__(self):
        super().__init__()
        self.setWindowTitle("Ventana Extra (PySide2)")
        self.setMinimumSize(400, 200)
        layout = QVBoxLayout()
        label = QLabel("¡Ventana Python abierta correctamente!")
        layout.addWidget(label)
        self.setLayout(layout)

if __name__ == "__main__":
    app = QApplication(sys.argv)
    ventana = VentanaExtra()
    ventana.show()
    sys.exit(app.exec_())
