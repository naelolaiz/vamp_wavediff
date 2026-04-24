#include "XCorrelation.h"

using std::string;
using std::vector;
using std::cerr;
using std::endl;

#include <cmath>
#include <algorithm>

XCorrelation::XCorrelation(float inputSampleRate) :
    Plugin(inputSampleRate),
    m_stepSize(0),
    m_previousSample(0.0f)
{
}

XCorrelation::~XCorrelation()
{
}

string
XCorrelation::getIdentifier() const
{
    return "zerocrossing";
}

string
XCorrelation::getName() const
{
    return "Zero Crossings";
}

string
XCorrelation::getDescription() const
{
    return "Detect and count zero crossing points";
}

string
XCorrelation::getMaker() const
{
    return "NAEL";
}

int
XCorrelation::getPluginVersion() const
{
    return 2;
}

string
XCorrelation::getCopyright() const
{
    return "Freely redistributable (BSD license)";
}

bool
XCorrelation::initialise(size_t channels, size_t stepSize, size_t blockSize)
{
    if (channels < getMinChannelCount() ||
	channels > getMaxChannelCount()) return false;

    m_stepSize = std::min(stepSize, blockSize);

    return true;
}

void
XCorrelation::reset()
{
    m_previousSample = 0.0f;
}

XCorrelation::OutputList
XCorrelation::getOutputDescriptors() const
{
    OutputList list;

    OutputDescriptor zc;
    zc.identifier = "counts";
    zc.name = "Zero Crossing Counts";
    zc.description = "The number of zero crossing points per processing block";
    zc.unit = "crossings";
    zc.hasFixedBinCount = true;
    zc.binCount = 1;
    zc.hasKnownExtents = false;
    zc.isQuantized = true;
    zc.quantizeStep = 1.0;
    zc.sampleType = OutputDescriptor::OneSamplePerStep;
    list.push_back(zc);

    zc.identifier = "zerocrossings";
    zc.name = "Zero Crossings";
    zc.description = "The locations of zero crossing points";
    zc.unit = "";
    zc.hasFixedBinCount = true;
    zc.binCount = 0;
    zc.sampleType = OutputDescriptor::VariableSampleRate;
    zc.sampleRate = m_inputSampleRate;
    list.push_back(zc);

    return list;
}

XCorrelation::FeatureSet
XCorrelation::process(const float *const *inputBuffers,
                      Vamp::RealTime timestamp)
{
    if (m_stepSize == 0) {
	cerr << "ERROR: XCorrelation::process: "
	     << "XCorrelation has not been initialised"
	     << endl;
	return FeatureSet();
    }

    float prev = m_previousSample;
    size_t count = 0;

    FeatureSet returnFeatures;

    for (size_t i = 0; i < m_stepSize; ++i) {

	float sample = inputBuffers[0][i];
	bool crossing = false;

	if (sample <= 0.0) {
	    if (prev > 0.0) crossing = true;
	} else if (sample > 0.0) {
	    if (prev <= 0.0) crossing = true;
	}

	if (crossing) {
	    ++count; 
	    Feature feature;
	    feature.hasTimestamp = true;
	    feature.timestamp = timestamp +
		Vamp::RealTime::frame2RealTime(i, (size_t)m_inputSampleRate);
	    returnFeatures[1].push_back(feature);
	}

	prev = sample;
    }

    m_previousSample = prev;

    Feature feature;
    feature.hasTimestamp = false;
    feature.values.push_back(float(count));

    returnFeatures[0].push_back(feature);
    return returnFeatures;
}

XCorrelation::FeatureSet
XCorrelation::getRemainingFeatures()
{
    return FeatureSet();
}

