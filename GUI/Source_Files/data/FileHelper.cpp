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
    using namespace std::chrono;

    auto now = system_clock::now();
    auto t_c = system_clock::to_time_t(now);

    auto micros = duration_cast<microseconds>(now.time_since_epoch()) % 1000000;
    auto millis = duration_cast<milliseconds>(now.time_since_epoch()) % 1000;

    std::stringstream ss;
    ss << std::put_time(std::localtime(&t_c), "%Y-%m-%d %H:%M:%S")
       << ":" << std::setw(3) << std::setfill('0') << millis.count()
       << "," << std::setw(3) << std::setfill('0') << (micros.count() % 1000);

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

// ===================== RAW =====================
void FileHelper::appendRawData(const std::string& path, const SensorData& d) {
    createDataDirectoryIfNeeded();
    ensureExists(path);
    writeHeaderIfNew(path, RAW_HEADER);

    std::ofstream file(path, std::ios::app);
    file << "\n---- Entrada cruda: " << nowTimestamp() << " ----\n";
    file << d.latitude << "," << d.longitude << "," << d.date << "," << d.utc_time << "," << d.secs << ","
         << d.satellites << "," << d.hdop << "," << d.Roll << "," << d.Pitch << "," << d.Yaw << ","
         << d.Servo1 << "," << d.Servo2 << "," << d.Servo3 << "," << d.Servo4 << "," << d.AltDiff << ","
         << d.pressure << "," << d.temperature << "\n";
    file.close();
}

// ===================== CLEAN  =====================
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

// ===================== REC  =====================
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

// ===================== CLEAN por sesión =====================
void FileHelper::iniciarSesionLimpios(const std::string& fechaISO, const std::string& horaISO) {
    createDataDirectoryIfNeeded();

    fechaPrimeraSesionISO = fechaISO;
    horaPrimeraSesionISO  = horaISO;

    // Construir nombre de archivo con fecha/hora de inicio (sanitizando caracteres)
    auto sanitize = [](std::string s) {
        for (char& c : s) if (c==' ' || c==':' || c=='.') c = '_';
        return s;
    };

    std::string base = sanitize(fechaISO + "_" + horaISO);
    rutaCleanSesion   = "../data/clean_session_" + base + ".csv";

    // Abrir y escribir encabezado
    archivoCleanSesion.open(rutaCleanSesion, std::ios::out);
    archivoCleanSesion
        << "Hora,Latitud,Longitud,Fecha,Hora,Segundos,Satélites,HDOP,Roll,Pitch,Yaw,"
           "Servo1,Servo2,Servo3,Servo4,AltDiff,Presión,Temperatura\n";
}

void FileHelper::escribirLimpioDuranteSesion(double tRel,
                                             const SensorData& d,
                                             bool /*primerRegistro*/,
                                             const std::string& /*fechaISO*/,
                                             const std::string& /*horaISO*/) {
    if (!archivoCleanSesion.is_open()) return;

    // Siempre escribimos la fecha/hora de INICIO de la sesión (primeras columnas Fecha/Hora)
    // y en la primera columna "Hora" va el tiempo relativo (tRel).
    archivoCleanSesion.setf(std::ios::fixed, std::ios::floatfield);
    archivoCleanSesion << std::setprecision(3)
                       << tRel << ","
                       << d.latitude << ","
                       << d.longitude << ","
                       << fechaPrimeraSesionISO << ","
                       << horaPrimeraSesionISO << ","
                       << d.secs << ","
                       << d.satellites << ","
                       << d.hdop << ","
                       << d.Roll << ","
                       << d.Pitch << ","
                       << d.Yaw << ","
                       << d.Servo1 << ","
                       << d.Servo2 << ","
                       << d.Servo3 << ","
                       << d.Servo4 << ","
                       << d.AltDiff << ","
                       << d.pressure << ","
                       << d.temperature << "\n";
}

void FileHelper::cerrarSesionLimpios() {
    if (archivoCleanSesion.is_open()) {
        archivoCleanSesion.close();
    }
}
