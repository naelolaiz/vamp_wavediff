#include "Backend.h"
#include <QDebug>

Backend::Backend(QObject *parent) : QObject(parent), process(new QProcess(this)) {}

void Backend::processFiles(const QString &file1, const QString &file2) {
    QString command = "sox"; // The command to run
    QStringList arguments;
    arguments << file1 << file2; // Add additional arguments as needed

    // Run the process
    process->start(command, arguments);

    connect(process, static_cast<void (QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished),
        this, [this](int exitCode, QProcess::ExitStatus exitStatus) {
            if (exitStatus == QProcess::CrashExit) {
                qDebug() << "The process crashed.";
            } else {
                qDebug() << "The process completed with exit code" << exitCode;
            }
        });

}

