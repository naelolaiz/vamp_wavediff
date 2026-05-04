#include "Backend.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QFileInfo>
#include <QProcessEnvironment>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QUrl>
#include <QVariantMap>

namespace {

constexpr const char* kSoxBinary = "sox";
constexpr const char* kSoxiBinary = "soxi";
constexpr const char* kVampHostBinary = "vamp-simple-host";
constexpr const char* kPluginKey = "vamp_wavediff:wavediff";

struct OutputDef
{
  const char* id;
  const char* label;
  const char* color;
};

constexpr OutputDef kOutputs[] = {
  { "rms_a", "RMS A", "#42a5f5" },
  { "rms_b", "RMS B", "#66bb6a" },
  { "rms_diff", "RMS A-B", "#ef5350" },
  { "peak_diff", "Peak |A-B|", "#ffa726" },
};

// Build a VAMP_PATH that puts the freshly-built plugin first, so a stale
// plugin in ~/.vamp/ or /usr/lib/vamp/ does not get loaded ahead of it.
QProcessEnvironment
buildPluginEnvironment()
{
  QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
  const QString exeDir = QCoreApplication::applicationDirPath();
  const QStringList relCandidates = {
    QStringLiteral("/../plugin"),   // out-of-source build tree
    QStringLiteral("/../lib/vamp"), // install tree (GNUInstallDirs)
    QStringLiteral("/../lib64/vamp"),
  };
  QStringList prefer;
  for (const QString& sub : relCandidates) {
    const QString p = QDir(exeDir + sub).absolutePath();
    if (QFileInfo(p + QStringLiteral("/vamp_wavediff.so")).isFile()) {
      prefer << p;
    }
  }
  const QString existing = env.value(QStringLiteral("VAMP_PATH"));
  if (!existing.isEmpty()) {
    prefer << existing;
  }
  if (!prefer.isEmpty()) {
    env.insert(QStringLiteral("VAMP_PATH"),
               prefer.join(QDir::listSeparator()));
  }
  return env;
}

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
  m_process->setProcessEnvironment(buildPluginEnvironment());
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
  resetPlotState();

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

  m_series.clear();
  for (const auto& o : kOutputs) {
    Series s;
    s.id = QString::fromLatin1(o.id);
    s.label = QString::fromLatin1(o.label);
    s.color = QString::fromLatin1(o.color);
    m_series.append(s);
  }
  m_currentSeriesIdx = 0;
  startAnalyzeNext();
}

void
Backend::startAnalyzeNext()
{
  if (m_currentSeriesIdx >= m_series.size()) {
    rebuildResultsAndPlot();
    setStatus(tr("Done."));
    finishWith(true);
    return;
  }

  const Series& s = m_series[m_currentSeriesIdx];
  setStatus(tr("Running wavediff plugin (%1 of %2: %3)…")
              .arg(m_currentSeriesIdx + 1)
              .arg(m_series.size())
              .arg(s.label));

  disconnect(m_process, nullptr, this, nullptr);
  connect(m_process, &QProcess::errorOccurred, this, &Backend::onProcessError);
  connect(m_process,
          QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
          this,
          &Backend::onAnalyzeFinished);

  const QStringList args{
    QStringLiteral("%1:%2").arg(QString::fromLatin1(kPluginKey)).arg(s.id),
    m_mergedPath
  };
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

  parseAnalyzeOutput(m_series[m_currentSeriesIdx], output);
  ++m_currentSeriesIdx;
  startAnalyzeNext();
}

void
Backend::parseAnalyzeOutput(Series& s, const QString& text)
{
  // Lines emitted by vamp-simple-host look like:
  //   <sample>: <value> [<extra-bins> ...] [<label-text>]
  // The "overall" summary line ends with a label such as "overall RMS A".
  // We only parse the first bin (single-pair stereo input is the common
  // case for this UI; multi-pair files would still get bin 0 plotted).
  static const QRegularExpression re(
    QStringLiteral(R"(^(\d+):\s+([\d.eE+\-]+)(?:\s+(.*))?$)"));

  const auto lines = text.split(QLatin1Char('\n'));
  for (const QString& raw : lines) {
    const QString line = raw.trimmed();
    if (line.isEmpty())
      continue;
    const auto m = re.match(line);
    if (!m.hasMatch())
      continue;
    const qint64 sample = m.captured(1).toLongLong();
    const double value = m.captured(2).toDouble();
    const QString rest = m.captured(3);
    if (!rest.isEmpty() &&
        rest.contains(QStringLiteral("overall"), Qt::CaseInsensitive)) {
      s.overall = value;
      s.hasOverall = true;
    } else {
      s.points.append(QPointF(static_cast<qreal>(sample), value));
    }
  }
}

void
Backend::rebuildResultsAndPlot()
{
  // Find the global x-extent and y-max across all series so the plot can
  // share a single axis.
  qint64 maxSample = 0;
  double maxVal = 0.0;
  for (const Series& s : m_series) {
    for (const QPointF& p : s.points) {
      if (p.x() > maxSample)
        maxSample = static_cast<qint64>(p.x());
      if (p.y() > maxVal)
        maxVal = p.y();
    }
    if (s.hasOverall && s.overall > maxVal)
      maxVal = s.overall;
  }
  m_totalSamples = maxSample;
  m_maxValue = maxVal;

  // Build the QML-facing variant list.
  m_plotSeries.clear();
  for (const Series& s : m_series) {
    QVariantList pts;
    pts.reserve(s.points.size());
    for (const QPointF& p : s.points) {
      pts.append(QVariant::fromValue(p));
    }
    QVariantMap entry;
    entry.insert(QStringLiteral("id"), s.id);
    entry.insert(QStringLiteral("label"), s.label);
    entry.insert(QStringLiteral("color"), s.color);
    entry.insert(QStringLiteral("points"), pts);
    entry.insert(QStringLiteral("overall"), s.overall);
    entry.insert(QStringLiteral("hasOverall"), s.hasOverall);
    m_plotSeries.append(entry);
  }
  emit plotChanged();

  // Compose a compact textual summary for the existing results pane.
  QStringList lines;
  lines << tr("Overall summary (per output, bin 0):");
  for (const Series& s : m_series) {
    if (s.hasOverall) {
      lines << QStringLiteral("  %1: %2")
                 .arg(s.label, -12)
                 .arg(s.overall, 0, 'g', 6);
    }
  }
  lines << QString();
  lines << tr("Per-block series collected: %1 outputs × ~%2 blocks each.")
             .arg(m_series.size())
             .arg(m_series.isEmpty() ? 0 : m_series.first().points.size());
  setResults(lines.join(QLatin1Char('\n')));
}

void
Backend::resetPlotState()
{
  m_series.clear();
  m_currentSeriesIdx = 0;
  m_plotSeries.clear();
  m_totalSamples = 0;
  m_maxValue = 0.0;
  emit plotChanged();
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
