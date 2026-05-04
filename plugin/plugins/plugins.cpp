#include "WaveDiffPlugin.h"

#include <vamp-sdk/PluginAdapter.h>
#include <vamp/vamp.h>

// The translation unit is built with hidden default visibility (see
// plugin/CMakeLists.txt). Mark the host entry point as exported explicitly,
// because per-symbol hidden visibility otherwise wins over `global:` in the
// linker version script and dlsym() ends up returning null.
#if defined(__GNUC__) || defined(__clang__)
#define VAMP_PLUGIN_API __attribute__((visibility("default")))
#else
#define VAMP_PLUGIN_API
#endif

static Vamp::PluginAdapter<WaveDiffPlugin> waveDiffAdapter;

extern "C" VAMP_PLUGIN_API const VampPluginDescriptor*
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
