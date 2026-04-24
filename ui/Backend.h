#ifndef BACKEND_H
#define BACKEND_H

#include <QObject>
#include <QString>
#include <QProcess>

class Backend : public QObject
{
    Q_OBJECT
public:
    explicit Backend(QObject *parent = nullptr);

public slots:
    void processFiles(const QString &file1, const QString &file2);

private:
    QProcess *process;
};

#endif // BACKEND_H
