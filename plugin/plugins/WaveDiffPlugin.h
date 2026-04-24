#ifndef VAMP_WAVEDIFF_PLUGIN_H
#define VAMP_WAVEDIFF_PLUGIN_H

#include <vamp-sdk/Plugin.h>
#include <vector>

/**
 * A Vamp plugin that compares two interleaved wave streams (A and B)
 * passed in as paired channels (A0, B0, A1, B1, ...).
 *
 * Use the bundled merger script (scripts/merger.sh) to produce a single
 * interleaved file from two independent wave files, then feed it to this
 * plugin with any Vamp host (sonic-visualiser, vamp-simple-host, ...).
 */
class WaveDiffPlugin : public Vamp::Plugin
{
public:
  explicit WaveDiffPlugin(float inputSampleRate);
  ~WaveDiffPlugin() override = default;

  bool initialise(size_t channels, size_t stepSize, size_t blockSize) override;
  void reset() override;

  size_t getMinChannelCount() const override;
  size_t getMaxChannelCount() const override;
  size_t getPreferredBlockSize() const override;
  size_t getPreferredStepSize() const override;

  InputDomain getInputDomain() const override;
  std::string getIdentifier() const override;
  std::string getName() const override;
  std::string getDescription() const override;
  std::string getMaker() const override;
  int getPluginVersion() const override;
  std::string getCopyright() const override;

  OutputList getOutputDescriptors() const override;
  ParameterList getParameterDescriptors() const override;
  float getParameter(std::string paramid) const override;
  void setParameter(std::string paramid, float newval) override;

  FeatureSet process(const float* const* inputBuffers,
                     Vamp::RealTime timestamp) override;

  FeatureSet getRemainingFeatures() override;

private:
  enum OutputIndex
  {
    OutputRmsA = 0,
    OutputRmsB = 1,
    OutputRmsDiff = 2,
    OutputPeakDiff = 3,
    OutputCount
  };

  size_t pairCount() const { return mChannelCount / 2u; }
  float toReported(float linear) const;

  size_t mStepSize{ 0 };
  size_t mBlockSize{ 0 };
  size_t mChannelCount{ 0 };
  size_t mProcessedSamples{ 0 };

  // Per-pair accumulators for the summary (getRemainingFeatures).
  std::vector<double> mSumSqA;
  std::vector<double> mSumSqB;
  std::vector<double> mSumSqDiff;
  std::vector<float> mPeakDiff;

  // Parameters.
  bool mReportInDecibels{ false };
};

#endif // VAMP_WAVEDIFF_PLUGIN_H
