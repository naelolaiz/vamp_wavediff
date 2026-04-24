#include "WaveDiffPlugin.h"

#include <algorithm>
#include <cmath>
#include <iostream>

namespace {

constexpr float kDbFloor = -200.0f;
constexpr float kEpsilon = 1e-20f;

float
linearToDb(float linear)
{
  if (linear <= kEpsilon) {
    return kDbFloor;
  }
  return 20.0f * std::log10(linear);
}

} // namespace

WaveDiffPlugin::WaveDiffPlugin(float inputSampleRate) : Plugin(inputSampleRate)
{
}

bool
WaveDiffPlugin::initialise(size_t channels, size_t stepSize, size_t blockSize)
{
  if (channels < getMinChannelCount() || channels > getMaxChannelCount()) {
    return false;
  }
  // This plugin expects pairs of channels (A, B), so the channel count must
  // be even. Anything else is a caller error.
  if (channels % 2u != 0u) {
    return false;
  }
  if (stepSize == 0u || blockSize == 0u || stepSize > blockSize) {
    return false;
  }

  mStepSize = stepSize;
  mBlockSize = blockSize;
  mChannelCount = channels;
  mProcessedSamples = 0u;

  const auto pairs = pairCount();
  mSumSqA.assign(pairs, 0.0);
  mSumSqB.assign(pairs, 0.0);
  mSumSqDiff.assign(pairs, 0.0);
  mPeakDiff.assign(pairs, 0.0f);

  return true;
}

void
WaveDiffPlugin::reset()
{
  mProcessedSamples = 0u;
  std::fill(mSumSqA.begin(), mSumSqA.end(), 0.0);
  std::fill(mSumSqB.begin(), mSumSqB.end(), 0.0);
  std::fill(mSumSqDiff.begin(), mSumSqDiff.end(), 0.0);
  std::fill(mPeakDiff.begin(), mPeakDiff.end(), 0.0f);
}

size_t
WaveDiffPlugin::getMinChannelCount() const
{
  return 2u;
}

size_t
WaveDiffPlugin::getMaxChannelCount() const
{
  return 256u;
}

size_t
WaveDiffPlugin::getPreferredBlockSize() const
{
  return 8192u;
}

size_t
WaveDiffPlugin::getPreferredStepSize() const
{
  return 8192u;
}

Vamp::Plugin::InputDomain
WaveDiffPlugin::getInputDomain() const
{
  return TimeDomain;
}

std::string
WaveDiffPlugin::getIdentifier() const
{
  return "wavediff";
}

std::string
WaveDiffPlugin::getName() const
{
  return "Wave Diff";
}

std::string
WaveDiffPlugin::getDescription() const
{
  return "Compares two audio streams passed as interleaved channel pairs "
         "(A0, B0, A1, B1, ...) and reports per-pair RMS of each stream, "
         "RMS of the null test (A - B), and peak absolute difference.";
}

std::string
WaveDiffPlugin::getMaker() const
{
  return "Natanael Olaiz";
}

int
WaveDiffPlugin::getPluginVersion() const
{
  return 2;
}

std::string
WaveDiffPlugin::getCopyright() const
{
  return "Copyright 2023-2026 Natanael Olaiz. Licensed under the GPL-3.0.";
}

Vamp::Plugin::OutputList
WaveDiffPlugin::getOutputDescriptors() const
{
  OutputList list;
  const auto pairs = pairCount();

  const auto buildBinNames = [pairs](const char* prefix) {
    std::vector<std::string> names;
    names.reserve(pairs);
    for (size_t p = 0; p < pairs; ++p) {
      names.push_back(std::string(prefix) + " pair " + std::to_string(p + 1));
    }
    return names;
  };

  const std::string unit = mReportInDecibels ? "dB" : "gain";

  auto makeDescriptor = [&](const std::string& id,
                            const std::string& name,
                            const std::string& description,
                            const char* binPrefix) {
    OutputDescriptor d;
    d.identifier = id;
    d.name = name;
    d.description = description;
    d.unit = unit;
    d.hasFixedBinCount = true;
    d.binCount = pairs;
    d.binNames = buildBinNames(binPrefix);
    d.hasKnownExtents = false;
    d.isQuantized = false;
    d.sampleType = OutputDescriptor::OneSamplePerStep;
    d.hasDuration = false;
    return d;
  };

  list.resize(OutputCount);
  list[OutputRmsA] = makeDescriptor(
    "rms_a", "RMS A", "Block RMS of the A (even-indexed) channels.", "A");
  list[OutputRmsB] = makeDescriptor(
    "rms_b", "RMS B", "Block RMS of the B (odd-indexed) channels.", "B");
  list[OutputRmsDiff] =
    makeDescriptor("rms_diff",
                   "RMS A-B (null test)",
                   "Block RMS of the sample-wise difference A - B.",
                   "A-B");
  list[OutputPeakDiff] = makeDescriptor(
    "peak_diff",
    "Peak |A-B|",
    "Peak absolute sample-wise difference |A - B| within the block.",
    "A-B");

  return list;
}

Vamp::PluginBase::ParameterList
WaveDiffPlugin::getParameterDescriptors() const
{
  ParameterList params;

  ParameterDescriptor p;
  p.identifier = "report_in_db";
  p.name = "Report in decibels";
  p.description = "If enabled, outputs are reported in dB (20 * log10). "
                  "Otherwise outputs are reported as linear gain in [0, 1].";
  p.unit = "";
  p.minValue = 0.0f;
  p.maxValue = 1.0f;
  p.defaultValue = 0.0f;
  p.isQuantized = true;
  p.quantizeStep = 1.0f;
  p.valueNames = { "linear", "dB" };
  params.push_back(p);

  return params;
}

float
WaveDiffPlugin::getParameter(std::string paramid) const
{
  if (paramid == "report_in_db") {
    return mReportInDecibels ? 1.0f : 0.0f;
  }
  return 0.0f;
}

void
WaveDiffPlugin::setParameter(std::string paramid, float newval)
{
  if (paramid == "report_in_db") {
    mReportInDecibels = newval >= 0.5f;
  }
}

float
WaveDiffPlugin::toReported(float linear) const
{
  return mReportInDecibels ? linearToDb(linear) : linear;
}

Vamp::Plugin::FeatureSet
WaveDiffPlugin::process(const float* const* inputBuffers,
                        Vamp::RealTime /*timestamp*/)
{
  FeatureSet features;
  if (mStepSize == 0u || mChannelCount == 0u) {
    std::cerr << "WaveDiffPlugin::process: plugin not initialised\n";
    return features;
  }

  const auto pairs = pairCount();

  Feature rmsA;
  Feature rmsB;
  Feature rmsDiff;
  Feature peakDiff;
  rmsA.hasTimestamp = false;
  rmsB.hasTimestamp = false;
  rmsDiff.hasTimestamp = false;
  peakDiff.hasTimestamp = false;

  rmsA.values.reserve(pairs);
  rmsB.values.reserve(pairs);
  rmsDiff.values.reserve(pairs);
  peakDiff.values.reserve(pairs);

  for (size_t p = 0; p < pairs; ++p) {
    const float* a = inputBuffers[2u * p];
    const float* b = inputBuffers[2u * p + 1u];

    double sumSqA = 0.0;
    double sumSqB = 0.0;
    double sumSqD = 0.0;
    float peak = 0.0f;

    for (size_t s = 0; s < mStepSize; ++s) {
      const float av = a[s];
      const float bv = b[s];
      const float dv = av - bv;
      const float ad = std::fabs(dv);

      sumSqA += static_cast<double>(av) * av;
      sumSqB += static_cast<double>(bv) * bv;
      sumSqD += static_cast<double>(dv) * dv;
      if (ad > peak) {
        peak = ad;
      }
    }

    const float norm = static_cast<float>(mStepSize);
    const float blockRmsA = std::sqrt(static_cast<float>(sumSqA / norm));
    const float blockRmsB = std::sqrt(static_cast<float>(sumSqB / norm));
    const float blockRmsD = std::sqrt(static_cast<float>(sumSqD / norm));

    rmsA.values.push_back(toReported(blockRmsA));
    rmsB.values.push_back(toReported(blockRmsB));
    rmsDiff.values.push_back(toReported(blockRmsD));
    peakDiff.values.push_back(toReported(peak));

    mSumSqA[p] += sumSqA;
    mSumSqB[p] += sumSqB;
    mSumSqDiff[p] += sumSqD;
    if (peak > mPeakDiff[p]) {
      mPeakDiff[p] = peak;
    }
  }

  mProcessedSamples += mStepSize;

  features[OutputRmsA].push_back(std::move(rmsA));
  features[OutputRmsB].push_back(std::move(rmsB));
  features[OutputRmsDiff].push_back(std::move(rmsDiff));
  features[OutputPeakDiff].push_back(std::move(peakDiff));
  return features;
}

Vamp::Plugin::FeatureSet
WaveDiffPlugin::getRemainingFeatures()
{
  FeatureSet fs;
  if (mProcessedSamples == 0u) {
    return fs;
  }

  const auto pairs = pairCount();
  const double norm = static_cast<double>(mProcessedSamples);

  Feature rmsA;
  Feature rmsB;
  Feature rmsDiff;
  Feature peakDiff;
  rmsA.hasTimestamp = true;
  rmsB.hasTimestamp = true;
  rmsDiff.hasTimestamp = true;
  peakDiff.hasTimestamp = true;
  rmsA.timestamp = Vamp::RealTime::zeroTime;
  rmsB.timestamp = Vamp::RealTime::zeroTime;
  rmsDiff.timestamp = Vamp::RealTime::zeroTime;
  peakDiff.timestamp = Vamp::RealTime::zeroTime;
  rmsA.label = "overall RMS A";
  rmsB.label = "overall RMS B";
  rmsDiff.label = "overall RMS A-B";
  peakDiff.label = "overall peak |A-B|";

  rmsA.values.reserve(pairs);
  rmsB.values.reserve(pairs);
  rmsDiff.values.reserve(pairs);
  peakDiff.values.reserve(pairs);

  for (size_t p = 0; p < pairs; ++p) {
    rmsA.values.push_back(
      toReported(static_cast<float>(std::sqrt(mSumSqA[p] / norm))));
    rmsB.values.push_back(
      toReported(static_cast<float>(std::sqrt(mSumSqB[p] / norm))));
    rmsDiff.values.push_back(
      toReported(static_cast<float>(std::sqrt(mSumSqDiff[p] / norm))));
    peakDiff.values.push_back(toReported(mPeakDiff[p]));
  }

  fs[OutputRmsA].push_back(std::move(rmsA));
  fs[OutputRmsB].push_back(std::move(rmsB));
  fs[OutputRmsDiff].push_back(std::move(rmsDiff));
  fs[OutputPeakDiff].push_back(std::move(peakDiff));
  return fs;
}
