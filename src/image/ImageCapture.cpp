#include "ImageCapture.h"

#include <QCoreApplication>
#include <QDebug>
#include <QProcess>
#include <QTemporaryFile>

#include "globals/Helper.h"
#include "ui/notifications/NotificationBox.h"

namespace mediaelch {

ImageCapture::ImageCapture(QObject* parent) : QObject(parent)
{
}

bool ImageCapture::captureImage(QString file, StreamDetails* streamDetails, ThumbnailDimensions dim, QImage& img)
{
    if (streamDetails->videoDetails().value(StreamDetails::VideoDetails::DurationInSeconds, nullptr) == nullptr) {
        streamDetails->loadStreamDetails();
    }
    if (streamDetails->videoDetails().value(StreamDetails::VideoDetails::DurationInSeconds, nullptr) == nullptr) {
        NotificationBox::instance()->showError(tr("Could not get duration of file"));
        return false;
    }

#ifdef Q_OS_OSX
    QString ffmpegbin = QCoreApplication::applicationDirPath() + "/ffmpeg";
#elif defined(Q_OS_WIN)
    QString ffmpegbin = QCoreApplication::applicationDirPath() + "/vendor/ffmpeg.exe";
#else
    QString ffmpegbin = "ffmpeg";
#endif

    QProcess ffmpeg;
    qsrand(QTime::currentTime().msec());

    unsigned duration =
        streamDetails->videoDetails().value(StreamDetails::VideoDetails::DurationInSeconds, nullptr).toUInt();

    if (duration == 0) {
        NotificationBox::instance()->showError(tr("Could not detect runtime of file"));
        return false;
    }
    unsigned t = static_cast<unsigned>(qrand()) % duration;
    QString timeCode = helper::secondsToTimeCode(t);

    QTemporaryFile tmpFile;
    if (!tmpFile.open()) {
        NotificationBox::instance()->showError(tr("Temporary output file could not be opened"));
        return false;
    }
    tmpFile.close();

    ffmpeg.start(ffmpegbin,
        QStringList() << "-y"
                      << "-ss" << timeCode << "-i" << file << "-vframes"
                      << "1"
                      << "-q:v"
                      << "2"
                      << "-f"
                      << "mjpeg" << tmpFile.fileName());
    if (!ffmpeg.waitForStarted()) {
#if defined(Q_OS_WIN) || defined(Q_OS_OSX)
        NotificationBox::instance()->showError(tr("Could not start ffmpeg"));
#else
        NotificationBox::instance()->showError(
            tr("Could not start ffmpeg. Please install it and make it available in your $PATH"));
#endif
        return false;
    }

    if (!ffmpeg.waitForFinished(10000)) {
        NotificationBox::instance()->showError(tr("ffmpeg did not finish"));
        return false;
    }
    if (!tmpFile.open()) {
        NotificationBox::instance()->showError(tr("Temporary output file could not be opened"));
        return false;
    }

    img = QImage::fromData(tmpFile.readAll());
    tmpFile.close();

    img = img.scaled(dim.width, dim.height, Qt::KeepAspectRatio, Qt::SmoothTransformation);

    return true;
}

} // namespace mediaelch
