#include "VideoManager.h"
#include <QDebug>
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QUdpSocket>
#include <QNetworkDatagram>
#include <QProcess>
#include <QStandardPaths>

#ifdef HAS_FFMPEG
extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
}
#endif

VideoManager::VideoManager(quint16 port, QObject* parent)
    : QObject(parent), m_port(port), m_running(false), m_formatCtx(nullptr) {
}

VideoManager::~VideoManager() {
    stop();
}

void VideoManager::start() {
    if (m_running.exchange(true)) return;

#ifdef HAS_FFMPEG
    runReceptionLoop();
#else
    runFfmpegCliLoop();
#endif
}

void VideoManager::stop() {
    m_running.store(false);
}

void VideoManager::runReceptionLoop() {
#ifdef HAS_FFMPEG
    qInfo() << "Starting VideoManager FFmpeg reception loop on port" << m_port;
    QString url = QString("udp://0.0.0.0:%1").arg(m_port);

    while (m_running.load()) {
        AVDictionary* options = nullptr;
        av_dict_set(&options, "buffer_size", "1048576", 0); // 1M bufsize
        av_dict_set(&options, "fifo_size", "10000", 0);
        av_dict_set(&options, "overrun_nonfatal", "1", 0);
        av_dict_set(&options, "stimeout", "3000000", 0); // 3s timeout

        m_formatCtx = nullptr;
        if (avformat_open_input(&m_formatCtx, url.toStdString().c_str(), nullptr, &options) < 0) {
            av_dict_free(&options);
            qWarning() << "Could not open video stream:" << url << ". Retrying in 2 seconds...";
            emit connectionStatusChanged(false);
            QThread::sleep(2);
            continue;
        }

        if (avformat_find_stream_info(m_formatCtx, nullptr) < 0) {
            qWarning() << "Could not find stream information for" << url << ". Retrying...";
            avformat_close_input(&m_formatCtx);
            av_dict_free(&options);
            QThread::sleep(1);
            continue;
        }

        av_dict_free(&options); // Free if not consumed or partially consumed

        int videoStreamIndex = -1;
        for (unsigned int i = 0; i < m_formatCtx->nb_streams; i++) {
            if (m_formatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
                videoStreamIndex = i;
                break;
            }
        }

        if (videoStreamIndex == -1) {
            qWarning() << "Could not find video stream in" << url;
            avformat_close_input(&m_formatCtx);
            QThread::sleep(1);
            continue;
        }

        // Notify about the codec parameters
        emit codecParametersDetected(m_formatCtx->streams[videoStreamIndex]->codecpar);
        emit connectionStatusChanged(true);

        AVPacket* packet = av_packet_alloc();
        while (m_running.load()) {
            if (av_read_frame(m_formatCtx, packet) >= 0) {
                if (packet->stream_index == videoStreamIndex) {
                    emit packetReceived(packet);
                }
                av_packet_unref(packet);
            } else {
                qWarning() << "Stream read error on" << url << ". Attempting reconnect...";
                emit connectionStatusChanged(false);
                break;
            }
            // Small sleep to avoid CPU saturation if reading is too fast
            QThread::msleep(1);
        }

        av_packet_free(&packet);
        avformat_close_input(&m_formatCtx);
    }
#else
    qWarning() << "FFmpeg NOT enabled. VideoManager will NOT receive real video.";
#endif
}

QString VideoManager::findFfmpegExecutable() const {
    const QString envPath = qEnvironmentVariable("FRECCIA_FFMPEG_PATH");
    if (!envPath.isEmpty() && QFileInfo::exists(envPath)) {
        return envPath;
    }

    const QString pathExecutable = QStandardPaths::findExecutable("ffmpeg");
    if (!pathExecutable.isEmpty()) {
        return pathExecutable;
    }

    const QString appDir = QCoreApplication::applicationDirPath();
    const QStringList candidates = {
        QDir(appDir).filePath("ffmpeg.exe"),
        QDir(appDir).filePath("ffmpeg/bin/ffmpeg.exe"),
        QStringLiteral("C:/ffmpeg/bin/ffmpeg.exe"),
        QStringLiteral("C:/Program Files/ffmpeg/bin/ffmpeg.exe"),
        QStringLiteral("C:/Program Files (x86)/ffmpeg/bin/ffmpeg.exe")
    };

    for (const QString& candidate : candidates) {
        if (QFileInfo::exists(candidate)) {
            return candidate;
        }
    }

    return QString();
}

void VideoManager::runFfmpegCliLoop() {
    const QString ffmpegPath = findFfmpegExecutable();
    if (ffmpegPath.isEmpty()) {
        qWarning() << "FFmpeg executable not found. Install ffmpeg or set FRECCIA_FFMPEG_PATH.";
        emit connectionStatusChanged(false);
        return;
    }

    constexpr int outputWidth = 640;
    constexpr int outputHeight = 360;
    constexpr int bytesPerPixel = 3;
    const int frameSize = outputWidth * outputHeight * bytesPerPixel;
    const QString inputUrl = QString("udp://127.0.0.1:0?localport=%1&buffer_size=1048576&fifo_size=10000&overrun_nonfatal=1")
                                 .arg(m_port);
    const QString videoFilter =
        QString("fps=30,scale=%1:%2:force_original_aspect_ratio=decrease,"
                "pad=%1:%2:(ow-iw)/2:(oh-ih)/2,setsar=1")
            .arg(outputWidth)
            .arg(outputHeight);

    qInfo() << "Starting VideoManager FFmpeg CLI receiver on port" << m_port << "using" << ffmpegPath;

    while (m_running.load()) {
        QProcess process;
        process.setProcessChannelMode(QProcess::SeparateChannels);

        const QStringList args = {
            "-hide_banner",
            "-loglevel", "warning",
            "-timeout", "3000000",
            "-i", inputUrl,
            "-an",
            "-sn",
            "-dn",
            "-vf", videoFilter,
            "-pix_fmt", "rgb24",
            "-f", "rawvideo",
            "pipe:1"
        };

        process.start(ffmpegPath, args);
        if (!process.waitForStarted(3000)) {
            qWarning() << "Could not start ffmpeg for UDP video port" << m_port << ":" << process.errorString();
            emit connectionStatusChanged(false);
            QThread::sleep(2);
            continue;
        }

        QByteArray pendingFrameBytes;
        bool connected = false;

        while (m_running.load() && process.state() != QProcess::NotRunning) {
            process.waitForReadyRead(100);

            const QByteArray stderrBytes = process.readAllStandardError();
            if (!stderrBytes.isEmpty()) {
                qWarning().noquote() << QString::fromLocal8Bit(stderrBytes).trimmed();
            }

            const QByteArray stdoutBytes = process.readAllStandardOutput();
            if (!stdoutBytes.isEmpty()) {
                pendingFrameBytes.append(stdoutBytes);
            }

            while (pendingFrameBytes.size() >= frameSize) {
                const QByteArray frameBytes = pendingFrameBytes.left(frameSize);
                pendingFrameBytes.remove(0, frameSize);

                QImage image(reinterpret_cast<const uchar*>(frameBytes.constData()),
                             outputWidth,
                             outputHeight,
                             outputWidth * bytesPerPixel,
                             QImage::Format_RGB888);

                if (!connected) {
                    connected = true;
                    emit connectionStatusChanged(true);
                }
                emit frameReceived(image.copy());
            }
        }

        if (process.state() != QProcess::NotRunning) {
            process.terminate();
            if (!process.waitForFinished(1000)) {
                process.kill();
                process.waitForFinished(1000);
            }
        }

        emit connectionStatusChanged(false);

        if (m_running.load()) {
            qWarning() << "FFmpeg video receiver stopped on port" << m_port << ". Retrying...";
            QThread::sleep(1);
        }
    }
}
