#include "WaveDiffPlugin.h"

#include <vamp-sdk/PluginAdapter.h>
#include <vamp/vamp.h>

static Vamp::PluginAdapter<WaveDiffPlugin> waveDiffAdapter;

extern "C" const VampPluginDescriptor*
vampGetPluginDescriptor(unsigned int version, unsigned int index)
{
  if (version < 1u) {
    return nullptr;
  }

  switch (index) {
    case 0:
      return waveDiffAdapter.getDescriptor();
    default:
      return nullptr;
  }
}
