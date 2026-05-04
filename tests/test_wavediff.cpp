// Unit tests for WaveDiffPlugin. No external test framework — plain C++17
// with <cassert>, wired into CTest from tests/CMakeLists.txt.

#include "WaveDiffPlugin.h"

#include <cassert>
#include <cmath>
#include <cstdio>
#include <vector>

namespace {

constexpr float kTol = 1e-5f;
constexpr double kPi = 3.14159265358979323846;

bool
nearly(float a, float b, float tol = kTol)
{
  return std::fabs(a - b) <= tol;
}

// ---- helpers ---------------------------------------------------------------

Vamp::Plugin::FeatureSet
runOneBlock(WaveDiffPlugin& plugin, const std::vector<const float*>& bufs)
{
  return plugin.process(bufs.data(), Vamp::RealTime::zeroTime);
}

// ---- tests -----------------------------------------------------------------

void
test_identical_streams_produce_zero_diff()
{
  WaveDiffPlugin plugin(48000.0f);
  const size_t block = 1024;
  assert(plugin.initialise(2, block, block));

  std::vector<float> a(block, 0.5f);
  std::vector<float> b(block, 0.5f);
  auto fs = runOneBlock(plugin, { a.data(), b.data() });

  assert(nearly(fs[0].at(0).values.at(0), 0.5f));
  assert(nearly(fs[1].at(0).values.at(0), 0.5f));
  assert(nearly(fs[2].at(0).values.at(0), 0.0f));
  assert(nearly(fs[3].at(0).values.at(0), 0.0f));
  std::printf("  test_identical_streams_produce_zero_diff: OK\n");
}

void
test_constant_offset_diff()
{
  // A = 1.0, B = 0.5  =>  RMS_A=1, RMS_B=0.5, RMS_diff=0.5, peak=0.5
  WaveDiffPlugin plugin(48000.0f);
  const size_t block = 256;
  assert(plugin.initialise(2, block, block));

  std::vector<float> a(block, 1.0f);
  std::vector<float> b(block, 0.5f);
  auto fs = runOneBlock(plugin, { a.data(), b.data() });

  assert(nearly(fs[0].at(0).values.at(0), 1.0f));
  assert(nearly(fs[1].at(0).values.at(0), 0.5f));
  assert(nearly(fs[2].at(0).values.at(0), 0.5f));
  assert(nearly(fs[3].at(0).values.at(0), 0.5f));
  std::printf("  test_constant_offset_diff: OK\n");
}

void
test_overall_summary_accumulates()
{
  // Four blocks of A=0.25, B=0.0  =>  overall RMS_A=0.25, RMS_B=0, diff=0.25.
  WaveDiffPlugin plugin(48000.0f);
  const size_t block = 128;
  assert(plugin.initialise(2, block, block));

  std::vector<float> a(block, 0.25f);
  std::vector<float> b(block, 0.0f);
  for (int i = 0; i < 4; ++i) {
    runOneBlock(plugin, { a.data(), b.data() });
  }
  auto sum = plugin.getRemainingFeatures();
  assert(nearly(sum[0].at(0).values.at(0), 0.25f));
  assert(nearly(sum[1].at(0).values.at(0), 0.0f));
  assert(nearly(sum[2].at(0).values.at(0), 0.25f));
  assert(nearly(sum[3].at(0).values.at(0), 0.25f));
  std::printf("  test_overall_summary_accumulates: OK\n");
}

void
test_two_pairs_independent()
{
  // Pair 0 identical (diff=0). Pair 1: A=0.6, B=0.4 (diff=0.2).
  WaveDiffPlugin plugin(48000.0f);
  const size_t block = 64;
  assert(plugin.initialise(4, block, block));

  std::vector<float> a0(block, 0.3f);
  std::vector<float> b0(block, 0.3f);
  std::vector<float> a1(block, 0.6f);
  std::vector<float> b1(block, 0.4f);
  auto fs = runOneBlock(plugin, { a0.data(), b0.data(), a1.data(), b1.data() });

  assert(nearly(fs[2].at(0).values.at(0), 0.0f));
  assert(nearly(fs[3].at(0).values.at(0), 0.0f));
  assert(nearly(fs[2].at(0).values.at(1), 0.2f));
  assert(nearly(fs[3].at(0).values.at(1), 0.2f));
  std::printf("  test_two_pairs_independent: OK\n");
}

void
test_db_reporting()
{
  WaveDiffPlugin plugin(48000.0f);
  plugin.setParameter("report_in_db", 1.0f);
  const size_t block = 256;
  assert(plugin.initialise(2, block, block));

  std::vector<float> a(block, 1.0f); // 0 dBFS
  std::vector<float> b(block, 1.0f); // identical
  auto fs = runOneBlock(plugin, { a.data(), b.data() });

  // RMS A == 1.0 -> 0 dB
  assert(nearly(fs[0].at(0).values.at(0), 0.0f, 1e-3f));
  // diff == 0 -> floor (-200 dB)
  assert(nearly(fs[2].at(0).values.at(0), -200.0f, 1e-3f));
  std::printf("  test_db_reporting: OK\n");
}

void
test_reject_odd_channel_count()
{
  WaveDiffPlugin plugin(48000.0f);
  assert(!plugin.initialise(3, 256, 256));
  std::printf("  test_reject_odd_channel_count: OK\n");
}

void
test_reject_invalid_block_sizes()
{
  WaveDiffPlugin plugin(48000.0f);
  assert(!plugin.initialise(2, 0, 256));
  assert(!plugin.initialise(2, 256, 0));
  assert(!plugin.initialise(2, 512, 256)); // step > block
  std::printf("  test_reject_invalid_block_sizes: OK\n");
}

void
test_sine_rms_and_peak()
{
  // Full-scale sine: RMS = 1/sqrt(2) ≈ 0.7071, peak = 1.0.
  WaveDiffPlugin plugin(48000.0f);
  const size_t block = 4800; // exactly 48 cycles of 480 Hz at 48 kHz
  assert(plugin.initialise(2, block, block));

  std::vector<float> a(block);
  std::vector<float> b(block, 0.0f);
  const double freq = 480.0;
  const double sr = 48000.0;
  for (size_t i = 0; i < block; ++i) {
    a[i] = static_cast<float>(std::sin(2.0 * kPi * freq * i / sr));
  }
  auto fs = runOneBlock(plugin, { a.data(), b.data() });

  assert(nearly(fs[0].at(0).values.at(0), 0.7071068f, 1e-3f));
  assert(nearly(fs[2].at(0).values.at(0), 0.7071068f, 1e-3f));
  assert(nearly(fs[3].at(0).values.at(0), 1.0f, 1e-3f));
  std::printf("  test_sine_rms_and_peak: OK\n");
}

void
test_reset_clears_summary()
{
  WaveDiffPlugin plugin(48000.0f);
  const size_t block = 128;
  assert(plugin.initialise(2, block, block));

  std::vector<float> a(block, 0.5f);
  std::vector<float> b(block, 0.0f);
  runOneBlock(plugin, { a.data(), b.data() });
  plugin.reset();

  // After reset with no further processing, summary must be empty.
  auto sum = plugin.getRemainingFeatures();
  assert(sum.empty());
  std::printf("  test_reset_clears_summary: OK\n");
}

} // namespace

int
main()
{
  std::printf("WaveDiffPlugin tests\n");
  test_identical_streams_produce_zero_diff();
  test_constant_offset_diff();
  test_overall_summary_accumulates();
  test_two_pairs_independent();
  test_db_reporting();
  test_reject_odd_channel_count();
  test_reject_invalid_block_sizes();
  test_sine_rms_and_peak();
  test_reset_clears_summary();
  std::printf("All tests passed.\n");
  return 0;
}
