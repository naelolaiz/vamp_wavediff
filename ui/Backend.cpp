#include "Backend.h"

#include <QDateTime>
#include <QDir>
#include <QFileInfo>
#include <QProcessEnvironment>
#include <QStandardPaths>
#include <QUrl>

namespace {

constexpr const char* kSoxBinary = "sox";
constexpr const char* kSoxiBinary = "soxi";
constexpr const char* kVampHostBinary = "vamp-simple-host";
constexpr const char* kPluginKey = "vamp_wavediff:wavediff";

QStringList
buildRemixArgs(int channelsPerInput)
{
  // Interleave channels so each Vamp pair (A_n, B_n) is adjacent:
  //   for channel c in [1..C]: input A channel c, input B channel c
  QStringList args;
  for (int c = 1; c <= channelsPerInput; ++c) {
    args << QString::number(c);                    // from file A
    args << QString::number(channelsPerInput + c); // from file B
  }
  return args;
}

} // namespace

Backend::Backend(QObject* parent)
  : QObject(parent), m_process(new QProcess(this))
{
  m_process->setProcessChannelMode(QProcess::MergedChannels);
  connect(m_process, &QProcess::errorOccurred, this, &Backend::onProcessError);

  m_analyzeAvailable = hasCommand(QString::fromLatin1(kVampHostBinary));
}

Backend::~Backend() = default;

void
Backend::runDiff(const QString& fileA, const QString& fileB)
{
  if (m_busy) {
    setError(tr("A job is already running. Cancel it first."));
    return;
  }

  const QString a = urlOrPathToLocal(fileA);
  const QString b = urlOrPathToLocal(fileB);

  if (a.isEmpty() || b.isEmpty()) {
    setError(tr("Please select both input wave files."));
    emit finished(false);
    return;
  }

  if (!QFileInfo::exists(a)) {
    setError(tr("File not found: %1").arg(a));
    emit finished(false);
    return;
  }
  if (!QFileInfo::exists(b)) {
    setError(tr("File not found: %1").arg(b));
    emit finished(false);
    return;
  }

  if (!hasCommand(QString::fromLatin1(kSoxBinary))) {
    setError(tr("`sox` was not found on PATH. Install SoX to continue."));
    emit finished(false);
    return;
  }

  m_fileA = a;
  m_fileB = b;
  setResults(QString());
  setError(QString());

  const QString tmpDir =
    QStandardPaths::writableLocation(QStandardPaths::TempLocation);
  QDir().mkpath(tmpDir);
  const QString stamp =
    QDateTime::currentDateTimeUtc().toString("yyyyMMdd-hhmmss-zzz");
  setMergedPath(
    QDir(tmpDir).filePath(QStringLiteral("wavediff-%1.wav").arg(stamp)));

  setBusy(true);
  startMerge();
}

void
Backend::cancel()
{
  if (!m_busy) {
    return;
  }
  if (m_process->state() != QProcess::NotRunning) {
    m_process->kill();
    m_process->waitForFinished(2000);
  }
  finishWith(false, tr("Cancelled."));
}

void
Backend::startMerge()
{
  setStatus(tr("Inspecting input files…"));

  // Determine channel count per input using soxi; both files must match.
  auto channelsOf = [](const QString& path) -> int {
    QProcess probe;
    probe.start(QString::fromLatin1(kSoxiBinary),
                { QStringLiteral("-c"), path });
    if (!probe.waitForStarted(3000) || !probe.waitForFinished(10000)) {
      return -1;
    }
    if (probe.exitStatus() != QProcess::NormalExit || probe.exitCode() != 0) {
      return -1;
    }
    bool ok = false;
    const int n = probe.readAllStandardOutput().trimmed().toInt(&ok);
    return ok ? n : -1;
  };

  const int chA = channelsOf(m_fileA);
  const int chB = channelsOf(m_fileB);
  if (chA <= 0 || chB <= 0) {
    finishWith(false, tr("Could not read channel count. Is `soxi` installed?"));
    return;
  }
  if (chA != chB) {
    finishWith(
      false,
      tr("Channel-count mismatch: A has %1, B has %2.").arg(chA).arg(chB));
    return;
  }

  setStatus(tr("Merging %1-channel streams…").arg(chA));

  QStringList args;
  args << QStringLiteral("-M") << m_fileA << m_fileB << m_mergedPath
       << QStringLiteral("remix") << buildRemixArgs(chA);

  disconnect(m_process, nullptr, this, nullptr);
  connect(m_process, &QProcess::errorOccurred, this, &Backend::onProcessError);
  connect(m_process,
          QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
          this,
          &Backend::onMergeFinished);

  m_process->start(QString::fromLatin1(kSoxBinary), args);
}

void
Backend::onMergeFinished(int exitCode, QProcess::ExitStatus status)
{
  const QByteArray output = m_process->readAll();
  if (status != QProcess::NormalExit || exitCode != 0) {
    finishWith(false,
               tr("Merge failed (exit %1).\n%2")
                 .arg(exitCode)
                 .arg(QString::fromLocal8Bit(output).trimmed()));
    return;
  }

  setStatus(tr("Merged file written to %1").arg(m_mergedPath));

  if (!m_analyzeAvailable) {
    QString note =
      tr("Analysis skipped: `vamp-simple-host` not found on PATH.\n"
         "Open the merged file in a Vamp-capable host "
         "(e.g. Sonic Visualiser) and load the `wavediff` plugin.");
    setResults(note);
    finishWith(true);
    return;
  }

  startAnalyze();
}

void
Backend::startAnalyze()
{
  setStatus(tr("Running wavediff plugin…"));

  disconnect(m_process, nullptr, this, nullptr);
  connect(m_process, &QProcess::errorOccurred, this, &Backend::onProcessError);
  connect(m_process,
          QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
          this,
          &Backend::onAnalyzeFinished);

  const QStringList args{ QString::fromLatin1(kPluginKey), m_mergedPath };
  m_process->start(QString::fromLatin1(kVampHostBinary), args);
}

void
Backend::onAnalyzeFinished(int exitCode, QProcess::ExitStatus status)
{
  const QString output = QString::fromLocal8Bit(m_process->readAll());
  if (status != QProcess::NormalExit || exitCode != 0) {
    finishWith(false,
               tr("Plugin run failed (exit %1).\n%2")
                 .arg(exitCode)
                 .arg(output.trimmed()));
    return;
  }

  setResults(output.trimmed());
  setStatus(tr("Done."));
  finishWith(true);
}

void
Backend::onProcessError(QProcess::ProcessError error)
{
  if (!m_busy) {
    return;
  }
  finishWith(false, tr("Process error: %1").arg(static_cast<int>(error)));
}

void
Backend::finishWith(bool success, const QString& reason)
{
  if (!reason.isEmpty()) {
    if (success) {
      setStatus(reason);
    } else {
      setError(reason);
      setStatus(tr("Failed."));
    }
  }
  setBusy(false);
  emit finished(success);
}

void
Backend::setBusy(bool busy)
{
  if (m_busy == busy)
    return;
  m_busy = busy;
  emit busyChanged();
}

void
Backend::setStatus(const QString& text)
{
  if (m_status == text)
    return;
  m_status = text;
  emit statusChanged();
}

void
Backend::setResults(const QString& text)
{
  if (m_results == text)
    return;
  m_results = text;
  emit resultsChanged();
}

void
Backend::setError(const QString& text)
{
  if (m_lastError == text)
    return;
  m_lastError = text;
  emit lastErrorChanged();
}

void
Backend::setMergedPath(const QString& text)
{
  if (m_mergedPath == text)
    return;
  m_mergedPath = text;
  emit mergedFilePathChanged();
}

QString
Backend::urlOrPathToLocal(const QString& raw)
{
  if (raw.isEmpty())
    return {};
  const QUrl url(raw);
  if (url.isLocalFile()) {
    return url.toLocalFile();
  }
  return raw;
}

bool
Backend::hasCommand(const QString& cmd)
{
  const QString path =
    QProcessEnvironment::systemEnvironment().value(QStringLiteral("PATH"));
  for (const QString& dir :
       path.split(QDir::listSeparator(), Qt::SkipEmptyParts)) {
    if (QFileInfo(QDir(dir).filePath(cmd)).isExecutable()) {
      return true;
    }
  }
  return false;
}
