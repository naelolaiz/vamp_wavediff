#ifndef _CROSS_CORRELATION_H
#define _CROSS_CORRELATION_H

#include "vamp-sdk/Plugin.h"

/**
 * Example plugin that calculates the positions and density of
 * zero-crossing points in an audio waveform.
*/

class XCorrelation : public Vamp::Plugin
{
public:
    XCorrelation(float inputSampleRate);
    virtual ~XCorrelation();

    bool initialise(size_t channels, size_t stepSize, size_t blockSize);
    void reset();

    InputDomain getInputDomain() const { return TimeDomain; }

    std::string getIdentifier() const;
    std::string getName() const;
    std::string getDescription() const;
    std::string getMaker() const;
    int getPluginVersion() const;
    std::string getCopyright() const;

    OutputList getOutputDescriptors() const;

    FeatureSet process(const float *const *inputBuffers,
                       Vamp::RealTime timestamp);

    FeatureSet getRemainingFeatures();

protected:
    size_t m_stepSize;
    float m_previousSample;
};


#endif // _CROSS_CORRELATION_H
