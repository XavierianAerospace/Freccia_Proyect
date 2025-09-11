// FileHelper.cpp
#include "FileHelper.h"

#include <fstream>
#include <filesystem>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include <QString>
#include <QIODevice>
#include <QDebug>
#include <QTime>


// === utilidades internas ===
static std::string nowTimestamp() {
    auto now = std::chrono::system_clock::now();
    std::time_t t_c = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&t_c), "%Y-%m-%d %H:%M:%S");
    return ss.str();
}

static const std::string RAW_HEADER =
    "Latitud,Longitud,Fecha,HoraUTC,Segundos,Satélites,HDOP,Roll,Pitch,Yaw,"
    "Servo1,Servo2,Servo3,Servo4,AltDiff,Presion,Temperatura";

bool FileHelper::exists(const std::string& path) {
    std::ifstream file(path);
    return file.good();
}

void FileHelper::ensureExists(const std::string& path) {
    std::filesystem::create_directories(std::filesystem::path(path).parent_path());
    if (!exists(path)) {
        std::ofstream file(path);
        file.close();
    }
}

void FileHelper::writeHeaderIfNew(const std::string& path, const std::string& header) {
    std::ifstream file(path);
    if (file.peek() == std::ifstream::traits_type::eof()) {
        std::ofstream outFile(path);
        outFile << header << "\n";
        outFile.close();
    }
}

void FileHelper::createDataDirectoryIfNeeded() {
    std::filesystem::create_directories("../data");
}

// ⬇️ AHORA: asegura carpeta+archivo+header, agrega línea de timestamp y escribe EXACTAMENTE 1 registro
void FileHelper::appendRawData(const std::string& path, const SensorData& d) {
    // Asegurar carpeta/archivo y encabezado
    createDataDirectoryIfNeeded();
    ensureExists(path);
    writeHeaderIfNew(path, RAW_HEADER);

    std::ofstream file(path, std::ios::app);

    // Línea de marca temporal (una por evento)
    file << "\n---- Entrada cruda: " << nowTimestamp() << " ----\n";

    // Una sola línea CSV del dato recibido
    // (Opcional: formateo con precisión; aquí se deja como valor bruto)
    file << d.latitude << "," << d.longitude << "," << d.date << "," << d.utc_time << "," << d.secs << ","
         << d.satellites << "," << d.hdop << "," << d.Roll << "," << d.Pitch << "," << d.Yaw << ","
         << d.Servo1 << "," << d.Servo2 << "," << d.Servo3 << "," << d.Servo4 << "," << d.AltDiff << ","
         << d.pressure << "," << d.temperature << "\n";

    file.close();
}

void FileHelper::appendCleanData(const std::string& path, const std::vector<SensorData>& data) {
    std::ofstream file(path, std::ios::app);
    for (const auto& d : data) {
        file << d.latitude << "," << d.longitude << "," << d.date << "," << d.utc_time << "," << d.secs << ","
             << d.satellites << "," << d.hdop << "," << d.Roll << "," << d.Pitch << "," << d.Yaw << ","
             << d.Servo1 << "," << d.Servo2 << "," << d.Servo3 << "," << d.Servo4 << "," << d.AltDiff << ","
             << d.pressure << "," << d.temperature << "\n";
    }
    file.close();
}

void FileHelper::appendErrorData(const std::string& path, const SensorData& d, const std::string& errorDetail) {
    std::ofstream file(path, std::ios::app);
    file << d.latitude << "," << d.longitude << "," << d.date << "," << d.utc_time << ","
         << d.satellites << "," << d.hdop << "," << errorDetail << "\n";
    file.close();
}

// ======= Grabación (tu lógica existente) =======
void FileHelper::iniciarGrabacion() {
    tiempoGrabado = QTime(0, 0, 0);
    createDataDirectoryIfNeeded();

    QString timestamp = "Date_" + QDateTime::currentDateTime().toString("dd_MMMM_yyyy") +
                        "_Time_" + QDateTime::currentDateTime().toString("HH_mm_ss");
    rutaArchivoActual = "../data/rec_" + timestamp.toStdString() + ".csv";

    archivoGrabacion.open(rutaArchivoActual, std::ios::out);
    archivoGrabacion << "Latitud,Longitud,Fecha,Hora,Segundos,Satélites,HDOP,Roll,Pitch,Yaw,Servo1,Servo2,Servo3,Servo4,AltDiff,Presión,Temperatura\n";
}

void FileHelper::escribirDuranteGrabacion(const SensorData& d) {
    if (archivoGrabacion.is_open()) {
        archivoGrabacion << d.latitude << "," << d.longitude << "," << d.date << "," << d.utc_time << "," << d.secs << ","
                         << d.satellites << "," << d.hdop << "," << d.Roll << "," << d.Pitch << "," << d.Yaw << ","
                         << d.Servo1 << "," << d.Servo2 << "," << d.Servo3 << "," << d.Servo4 << "," << d.AltDiff << ","
                         << d.pressure << "," << d.temperature << "\n";
    }
}

void FileHelper::detenerGrabacion() {
    if (archivoGrabacion.is_open()) {
        archivoGrabacion.close();
    }

    // Añadir la duración al principio del archivo
    QString ruta = QString::fromStdString(rutaArchivoActual);
    QFile file(ruta);

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "No se pudo abrir el archivo para leer:" << ruta;
        return;
    }

    QString contenido = QTextStream(&file).readAll();
    file.close();

    if (!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
        qWarning() << "No se pudo abrir el archivo para escribir:" << ruta;
        return;
    }

    QTextStream out(&file);
    out << "Tiempo de grabación: " << tiempoGrabado.toString("hh:mm:ss") << "\n";
    out << contenido;
    file.close();
}
