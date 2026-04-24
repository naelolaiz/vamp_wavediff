#ifndef WAVEDIFF_BACKEND_H
#define WAVEDIFF_BACKEND_H

#include <QObject>
#include <QProcess>
#include <QString>
#include <QStringList>
#include <QUrl>

/**
 * Exposes the diff pipeline to QML:
 *   1. interleave fileA / fileB into a single multichannel file via SoX
 *   2. optionally run vamp-simple-host over the merged file with the
 *      wavediff Vamp plugin to produce summary metrics
 *
 * State is reported through properties so the UI can bind to it.
 */
class Backend : public QObject
{
  Q_OBJECT
  Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)
  Q_PROPERTY(QString statusMessage READ statusMessage NOTIFY statusChanged)
  Q_PROPERTY(QString resultsText READ resultsText NOTIFY resultsChanged)
  Q_PROPERTY(QString lastError READ lastError NOTIFY lastErrorChanged)
  Q_PROPERTY(
    QString mergedFilePath READ mergedFilePath NOTIFY mergedFilePathChanged)

public:
  explicit Backend(QObject* parent = nullptr);
  ~Backend() override;

  bool busy() const { return m_busy; }
  QString statusMessage() const { return m_status; }
  QString resultsText() const { return m_results; }
  QString lastError() const { return m_lastError; }
  QString mergedFilePath() const { return m_mergedPath; }

public slots:
  /** Run the full merge-and-analyze pipeline. Inputs may be QUrl "file://" form
   * or plain paths. */
  void runDiff(const QString& fileA, const QString& fileB);

  /** Cancel the currently running process, if any. */
  void cancel();

signals:
  void busyChanged();
  void statusChanged();
  void resultsChanged();
  void lastErrorChanged();
  void mergedFilePathChanged();
  void finished(bool success);

private slots:
  void onMergeFinished(int exitCode, QProcess::ExitStatus status);
  void onAnalyzeFinished(int exitCode, QProcess::ExitStatus status);
  void onProcessError(QProcess::ProcessError error);

private:
  void setBusy(bool busy);
  void setStatus(const QString& text);
  void setResults(const QString& text);
  void setError(const QString& text);
  void setMergedPath(const QString& text);

  void startMerge();
  void startAnalyze();
  void finishWith(bool success, const QString& reason = {});

  static QString urlOrPathToLocal(const QString& raw);
  static bool hasCommand(const QString& cmd);

  QProcess* m_process = nullptr;
  QString m_fileA;
  QString m_fileB;
  QString m_mergedPath;
  QString m_status;
  QString m_results;
  QString m_lastError;
  bool m_busy = false;
  bool m_analyzeAvailable = false;
};

#endif // WAVEDIFF_BACKEND_H
