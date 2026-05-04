#ifndef WAVEDIFF_BACKEND_H
#define WAVEDIFF_BACKEND_H

#include <QObject>
#include <QPointF>
#include <QProcess>
#include <QQmlEngine>
#include <QString>
#include <QStringList>
#include <QUrl>
#include <QVariantList>
#include <QVector>

/**
 * Exposes the diff pipeline to QML:
 *   1. interleave fileA / fileB into a single multichannel file via SoX
 *   2. run vamp-simple-host over the merged file with the wavediff Vamp
 *      plugin once per output (rms_a, rms_b, rms_diff, peak_diff) to
 *      collect both the per-block series and the overall summary
 *
 * State is reported through properties so the UI can bind to it. The
 * plot data is published as a QVariantList of {id, label, color, points,
 * overall} maps, ready to consume from a QML Canvas.
 */
class Backend : public QObject
{
  Q_OBJECT
  QML_ELEMENT
  Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)
  Q_PROPERTY(QString statusMessage READ statusMessage NOTIFY statusChanged)
  Q_PROPERTY(QString resultsText READ resultsText NOTIFY resultsChanged)
  Q_PROPERTY(QString lastError READ lastError NOTIFY lastErrorChanged)
  Q_PROPERTY(
    QString mergedFilePath READ mergedFilePath NOTIFY mergedFilePathChanged)
  Q_PROPERTY(QVariantList plotSeries READ plotSeries NOTIFY plotChanged)
  Q_PROPERTY(qint64 totalSamples READ totalSamples NOTIFY plotChanged)
  Q_PROPERTY(double maxValue READ maxValue NOTIFY plotChanged)

public:
  explicit Backend(QObject* parent = nullptr);
  ~Backend() override;

  bool busy() const { return m_busy; }
  QString statusMessage() const { return m_status; }
  QString resultsText() const { return m_results; }
  QString lastError() const { return m_lastError; }
  QString mergedFilePath() const { return m_mergedPath; }
  QVariantList plotSeries() const { return m_plotSeries; }
  qint64 totalSamples() const { return m_totalSamples; }
  double maxValue() const { return m_maxValue; }

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
  void plotChanged();
  void finished(bool success);

private slots:
  void onMergeFinished(int exitCode, QProcess::ExitStatus status);
  void onAnalyzeFinished(int exitCode, QProcess::ExitStatus status);
  void onProcessError(QProcess::ProcessError error);

private:
  struct Series
  {
    QString id;
    QString label;
    QString color;
    QVector<QPointF> points; // (sample_offset, value)
    double overall = 0.0;
    bool hasOverall = false;
    QString rawOutput; // verbatim vamp-simple-host stdout/stderr
  };

  void setBusy(bool busy);
  void setStatus(const QString& text);
  void setResults(const QString& text);
  void setError(const QString& text);
  void setMergedPath(const QString& text);

  void startMerge();
  void startAnalyzeNext();
  void parseAnalyzeOutput(Series& s, const QString& text);
  void rebuildResultsAndPlot();
  void resetPlotState();
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

  QVector<Series> m_series;
  int m_currentSeriesIdx = 0;
  QVariantList m_plotSeries;
  qint64 m_totalSamples = 0;
  double m_maxValue = 0.0;
};

#endif // WAVEDIFF_BACKEND_H
