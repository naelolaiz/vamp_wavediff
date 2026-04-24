#ifndef __WAVE_DIFF_H__
#define __WAVE_DIFF_H__

#include "vamp-sdk/Plugin.h"

class WaveDiff : public Vamp::Plugin
{
public:
    WaveDiff(float inputSampleRate);
    virtual ~WaveDiff();

    bool initialise(size_t channelCount, size_t stepSize, size_t blockSize);
    void reset();

    InputDomain getInputDomain() const { return FrequencyDomain; }

    std::string getIdentifier() const;
    std::string getName() const;
    std::string getDescription() const;
    std::string getMaker() const;
    int getPluginVersion() const;
    std::string getCopyright() const;

    size_t getMinChannelCount() const { return 2; }
    size_t getMaxChannelCount() const { return 256; }

//    size_t getPreferredStepSize() const;
//    size_t getPreferredBlockSize() const;

    ParameterList getParameterDescriptors() const;
    float getParameter(std::string id) const;
    void setParameter(std::string id, float value);

    OutputList getOutputDescriptors() const;

    FeatureSet process(const float *const *inputBuffers,
                       Vamp::RealTime timestamp);

    FeatureSet getRemainingFeatures();

protected:
    class D;
    D *m_d;
};


#endif // __WAVE_DIFF_H__
