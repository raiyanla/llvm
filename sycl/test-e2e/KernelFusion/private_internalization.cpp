// RUN: %{build} -fsycl-embed-ir -O2 -o %t.out
// RUN: env SYCL_PARALLEL_FOR_RANGE_ROUNDING_PARAMS=16:32:512 %{run} %t.out

// Test complete fusion with private internalization specified on the
// accessors.

#include <sycl/sycl.hpp>

using namespace sycl;

template <typename BaseName, size_t dataSize> class KernelName;

template <size_t dataSize> static void test() {
  int in1[dataSize], in2[dataSize], in3[dataSize], tmp[dataSize], out[dataSize];

  for (size_t i = 0; i < dataSize; ++i) {
    in1[i] = i * 2;
    in2[i] = i * 3;
    in3[i] = i * 4;
    tmp[i] = -1;
    out[i] = -1;
  }

  queue q{ext::codeplay::experimental::property::queue::enable_fusion{}};

  {
    buffer<int> bIn1{in1, range{dataSize}};
    buffer<int> bIn2{in2, range{dataSize}};
    buffer<int> bIn3{in3, range{dataSize}};
    buffer<int> bTmp{tmp, range{dataSize}};
    buffer<int> bOut{out, range{dataSize}};

    ext::codeplay::experimental::fusion_wrapper fw{q};
    fw.start_fusion();

    assert(fw.is_in_fusion_mode() && "Queue should be in fusion mode");

    q.submit([&](handler &cgh) {
      auto accIn1 = bIn1.get_access(cgh);
      auto accIn2 = bIn2.get_access(cgh);
      auto accTmp = bTmp.get_access(
          cgh, sycl::ext::codeplay::experimental::property::promote_private{});
      cgh.parallel_for<KernelName<class KernelOne, dataSize>>(
          dataSize, [=](id<1> i) { accTmp[i] = accIn1[i] + accIn2[i]; });
    });

    q.submit([&](handler &cgh) {
      auto accTmp = bTmp.get_access(
          cgh, sycl::ext::codeplay::experimental::property::promote_private{});
      auto accIn3 = bIn3.get_access(cgh);
      auto accOut = bOut.get_access(cgh);
      cgh.parallel_for<KernelName<class KernelTwo, dataSize>>(
          dataSize, [=](id<1> i) { accOut[i] = accTmp[i] * accIn3[i]; });
    });

    fw.complete_fusion({ext::codeplay::experimental::property::no_barriers{}});

    assert(!fw.is_in_fusion_mode() &&
           "Queue should not be in fusion mode anymore");
  }

  // Check the results
  for (size_t i = 0; i < dataSize; ++i) {
    assert(out[i] == (20 * i * i) && "Computation error");
    assert(tmp[i] == -1 && "Not internalized");
  }
}

int main() {
  // Test power-of-two size.
  test<512>();

  // Test prime size large enough to trigger rounded-range kernel insertion.
  // Note that we lower the RR threshold when running this test.
  test<523>();

  return 0;
}
