#include "WaveDiff.h"

using std::string;
using std::vector;
using std::cerr;
using std::endl;

using Vamp::RealTime;

#include <cmath>
#include <cstdio>
#include <iostream>


class WaveDiff::D
// this class just avoids us having to declare any data members in the header
{
public:
    D(float inputSampleRate);
    ~D();

    //size_t getPreferredStepSize() const { return 64; }
    //size_t getPreferredBlockSize() const { return 256; }

    ParameterList getParameterDescriptors() const;
    float getParameter(string id) const;
    void setParameter(string id, float value);

    OutputList getOutputDescriptors() const;

    bool initialise(size_t channelCount, size_t stepSize, size_t blockSize);
    void reset();
    FeatureSet process(const float *const *, RealTime);
    FeatureSet getRemainingFeatures();

private:
    float m_inputSampleRate{};
    size_t m_stepSize{};
    size_t m_blockSize{};
    size_t m_channelCount{};


    Vamp::RealTime m_start;
    Vamp::RealTime m_lasttime;
};

WaveDiff::D::D(float inputSampleRate) :
    m_inputSampleRate(inputSampleRate)
{
}

WaveDiff::D::~D()
{
}

WaveDiff::ParameterList
WaveDiff::D::getParameterDescriptors() const
{
    ParameterList list;

//    ParameterDescriptor d;
//    d.identifier = "minbpm";
//    d.name = "Minimum estimated tempo";
//    d.description = "Minimum beat-per-minute value which the tempo estimator is able to return";
//    d.unit = "bpm";
//    d.minValue = 10;
//    d.maxValue = 360;
//    d.defaultValue = 50;
//    d.isQuantized = false;
//    list.push_back(d);

//    d.identifier = "maxbpm";
//    d.name = "Maximum estimated tempo";
//    d.description = "Maximum beat-per-minute value which the tempo estimator is able to return";
//    d.defaultValue = 190;
//    list.push_back(d);

//    d.identifier = "maxdflen";
//    d.name = "Input duration to study";
//    d.description = "Length of audio input, in seconds, which should be taken into account when estimating tempo.  There is no need to supply the plugin with any further input once this time has elapsed since the start of the audio.  The tempo estimator may use only the first part of this, up to eight times the slowest beat duration: increasing this value further than that is unlikely to improve results.";
//    d.unit = "s";
//    d.minValue = 2;
//    d.maxValue = 40;
//    d.defaultValue = 10;
//    list.push_back(d);

    return list;
}

float
WaveDiff::D::getParameter(string id) const
{
//    if (id == "minbpm") {
//        return m_minbpm;
//    } else if (id == "maxbpm") {
//        return m_maxbpm;
//    } else if (id == "maxdflen") {
//        return m_maxdflen;
//    }
    return 0.f;
}

void
WaveDiff::D::setParameter(string id, float value)
{
//    if (id == "minbpm") {
//        m_minbpm = value;
//    } else if (id == "maxbpm") {
//        m_maxbpm = value;
//    } else if (id == "maxdflen") {
//        m_maxdflen = value;
//    }
}

WaveDiff::OutputList
WaveDiff::D::getOutputDescriptors() const
{
    OutputList list;

    OutputDescriptor d;
    d.identifier = "amplitude_diff";
    d.name = "Amplitude Diff";
    d.description = "Aplitude diff";
    d.unit = "dB";
    d.hasFixedBinCount = true;
    d.binCount = 1;
    d.hasKnownExtents = false;
    d.isQuantized = false;
    d.sampleType = OutputDescriptor::VariableSampleRate;
    d.sampleRate = m_inputSampleRate;
    d.hasDuration = false; // all block
    list.push_back(d);

//    d.identifier = "candidates";
//    d.name = "Tempo candidates";
//    d.description = "Possible tempo estimates, one per bin with the most likely in the first bin";
//    d.unit = "bpm";
//    d.hasFixedBinCount = false;
//    list.push_back(d);

//    d.identifier = "detectionfunction";
//    d.name = "Detection Function";
//    d.description = "Onset detection function";
//    d.unit = "";
//    d.hasFixedBinCount = 1;
//    d.binCount = 1;
//    d.hasKnownExtents = true;
//    d.minValue = 0.0;
//    d.maxValue = 1.0;
//    d.isQuantized = false;
//    d.quantizeStep = 0.0;
//    d.sampleType = OutputDescriptor::FixedSampleRate;
//    if (m_stepSize) {
//        d.sampleRate = m_inputSampleRate / m_stepSize;
//    } else {
//        d.sampleRate = m_inputSampleRate / (getPreferredBlockSize()/2);
//    }
//    d.hasDuration = false;
//    list.push_back(d);

//    d.identifier = "acf";
//    d.name = "Autocorrelation Function";
//    d.description = "Autocorrelation of onset detection function";
//    d.hasKnownExtents = false;
//    d.unit = "r";
//    list.push_back(d);

//    d.identifier = "filtered_acf";
//    d.name = "Filtered Autocorrelation";
//    d.description = "Filtered autocorrelation of onset detection function";
//    d.unit = "r";
//    list.push_back(d);

    return list;
}

bool
WaveDiff::D::initialise(size_t channelCount, size_t stepSize, size_t blockSize)
{

    m_channelCount = channelCount;
    m_stepSize = stepSize;
    m_blockSize = blockSize;


//    float dfLengthSecs = m_maxdflen;
//    m_dfsize = (dfLengthSecs * m_inputSampleRate) / m_stepSize;

//    m_priorMagnitudes = new float[m_blockSize/2];
//    m_df = new float[m_dfsize];

//    for (size_t i = 0; i < m_blockSize/2; ++i) {
//        m_priorMagnitudes[i] = 0.f;
//    }
//    for (size_t i = 0; i < m_dfsize; ++i) {
//        m_df[i] = 0.f;
//    }

//    m_n = 0;

    return true;
}

void
WaveDiff::D::reset()
{
//    if (!m_priorMagnitudes) return;

//    for (size_t i = 0; i < m_blockSize/2; ++i) {
//        m_priorMagnitudes[i] = 0.f;
//    }
//    for (size_t i = 0; i < m_dfsize; ++i) {
//        m_df[i] = 0.f;
//    }

//    delete[] m_r;
//    m_r = 0;

//    delete[] m_fr;
//    m_fr = 0;

//    delete[] m_t;
//    m_t = 0;

//    m_n = 0;

    m_start = RealTime::zeroTime;
    m_lasttime = RealTime::zeroTime;
}

WaveDiff::FeatureSet
WaveDiff::D::process(const float *const *inputBuffers, RealTime ts)
{
    FeatureSet fs;

//    if (m_stepSize == 0) {
//    cerr << "ERROR: WaveDiff::process: "
//         << "WaveDiff has not been initialised"
//	     << endl;
//	return fs;
//    }

//    if (m_n == 0) m_start = ts;
//    m_lasttime = ts;

//    if (m_n == m_dfsize) {
//        // If we have seen enough input, do the estimation and return
//        calculate();
//        fs = assembleFeatures();
//        ++m_n;
//        return fs;
//    }

//    // If we have seen more than enough, just discard and return!
//    if (m_n > m_dfsize) return FeatureSet();

//    float value = 0.f;

//    // m_df will contain an onset detection function based on the rise
//    // in overall power from one spectral frame to the next --
//    // simplistic but reasonably effective for our purposes.

//    for (size_t i = 1; i < m_blockSize/2; ++i) {

//        float real = inputBuffers[0][i*2];
//        float imag = inputBuffers[0][i*2 + 1];

//        float sqrmag = real * real + imag * imag;
//        value += fabsf(sqrmag - m_priorMagnitudes[i]);

//        m_priorMagnitudes[i] = sqrmag;
//    }

//    m_df[m_n] = value;

//    ++m_n;
    return fs;
}    

WaveDiff::FeatureSet
WaveDiff::D::getRemainingFeatures()
{
    FeatureSet fs;
//    if (m_n > m_dfsize) return fs;
//    calculate();
//    fs = assembleFeatures();
//    ++m_n;
    return fs;
}


    

WaveDiff::WaveDiff(float inputSampleRate) :
    Plugin(inputSampleRate),
    m_d(new D(inputSampleRate))
{
}

WaveDiff::~WaveDiff()
{
    delete m_d;
}

string
WaveDiff::getIdentifier() const
{
    return "wavediff";
}

string
WaveDiff::getName() const
{
    return "Wave differ ";
}

string
WaveDiff::getDescription() const
{
    return "Returns information about the diff between input channels";
}

string
WaveDiff::getMaker() const
{
    return "Natanael Olaiz";
}

int
WaveDiff::getPluginVersion() const
{
    return 1;
}

string
WaveDiff::getCopyright() const
{
    return "Copyright 2023. GPL v3";
}

//size_t
//WaveDiff::getPreferredStepSize() const
//{
//    return m_d->getPreferredStepSize();
//}

//size_t
//WaveDiff::getPreferredBlockSize() const
//{
//    return m_d->getPreferredBlockSize();
//}

bool
WaveDiff::initialise(size_t channelCount, size_t stepSize, size_t blockSize)
{
    std::cout << " channelCount: " << channelCount <<std::endl;
    if (channelCount < getMinChannelCount() ||
        channelCount > getMaxChannelCount() ||
        channelCount%2 != 0
        ) return false;


    return m_d->initialise(channelCount, stepSize, blockSize);
}

void
WaveDiff::reset()
{
    return m_d->reset();
}

WaveDiff::ParameterList
WaveDiff::getParameterDescriptors() const
{
    return m_d->getParameterDescriptors();
}

float
WaveDiff::getParameter(std::string id) const
{
    return m_d->getParameter(id);
}

void
WaveDiff::setParameter(std::string id, float value)
{
    m_d->setParameter(id, value);
}

WaveDiff::OutputList
WaveDiff::getOutputDescriptors() const
{
    return m_d->getOutputDescriptors();
}

WaveDiff::FeatureSet
WaveDiff::process(const float *const *inputBuffers, RealTime ts)
{
    return m_d->process(inputBuffers, ts);
}

WaveDiff::FeatureSet
WaveDiff::getRemainingFeatures()
{
    return m_d->getRemainingFeatures();
}
