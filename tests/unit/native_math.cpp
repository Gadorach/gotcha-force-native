#include "engine/animation/native_math.h"
#include <array>
#include <cstdio>
#include <cstdlib>
#include <limits>

static void check(bool ok) { if (!ok) std::abort(); }
static bool close(double a, double b, double tolerance = 2e-5) {
  return std::abs(a-b) <= tolerance*(1.0+std::abs(b));
}
static unsigned seed = 0x392813;
static float sample() { seed = seed*1664525u+1013904223u; return (float(int(seed%20001)-10000))/1000.0f; }

int main() {
  unsigned cases = 0;
  for (unsigned trial = 0; trial < 1000; ++trial) {
    std::array<float, 12> a, b, product{}, result{};
    std::array<float, 3> v, intermediate{}, composed{}, sequential{}, cross{};
    for (float& x : a) x = sample();
    for (float& x : b) x = sample();
    for (float& x : v) x = sample();
    check(gf_math_evaluate(GF_MATH_CONCAT, a.data(), b.data(), 0, product.data()) == 12);
    for (unsigned r = 0; r < 3; ++r) for (unsigned c = 0; c < 4; ++c) {
      double expected = c == 3 ? a[r*4+3] : 0;
      for (unsigned k = 0; k < 3; ++k) expected += double(a[r*4+k])*b[k*4+c];
      check(close(product[r*4+c], expected));
    }
    gf_math_evaluate(GF_MATH_POINT, b.data(), v.data(), 0, intermediate.data());
    gf_math_evaluate(GF_MATH_POINT, a.data(), intermediate.data(), 0, sequential.data());
    gf_math_evaluate(GF_MATH_POINT, product.data(), v.data(), 0, composed.data());
    for (unsigned i = 0; i < 3; ++i) check(close(composed[i], sequential[i], 2e-4));
    auto alias = a;
    gf_math_evaluate(GF_MATH_CONCAT, alias.data(), b.data(), 0, alias.data());
    check(alias == product);
    alias = b;
    gf_math_evaluate(GF_MATH_CONCAT, a.data(), alias.data(), 0, alias.data());
    check(alias == product);
    gf_math_evaluate(GF_MATH_CROSS, a.data(), v.data(), 0, cross.data());
    double dot = 0;
    for (unsigned i = 0; i < 3; ++i) dot += double(cross[i])*a[i];
    check(std::abs(dot) < 0.002);
    check(gf_math_evaluate(GF_MATH_NORMALIZE, v.data(), nullptr, 0, result.data()) == 3);
    check(close(double(result[0])*result[0]+double(result[1])*result[1]+double(result[2])*result[2], 1));
    gf_math_evaluate(GF_MATH_DOT, a.data(), v.data(), 0, result.data());
    check(close(result[0], double(a[0])*v[0]+double(a[1])*v[1]+double(a[2])*v[2]));
    gf_math_evaluate(GF_MATH_COPY, a.data(), nullptr, 0, result.data());
    check(result == a);
    for (unsigned op : {GF_MATH_ADD, GF_MATH_SUBTRACT, GF_MATH_SCALE, GF_MATH_DIRECTION}) {
      result.fill(1234);
      check(gf_math_evaluate(op, a.data(), v.data(), 2, result.data()) == 3);
      check(result[3] == 1234); // Never adopt a best-effort four-float vector bug.
      for (unsigned i = 0; i < 3; ++i) {
        double expected = op == GF_MATH_ADD ? double(a[i])+v[i] : op == GF_MATH_SUBTRACT ?
            double(a[i])-v[i] : op == GF_MATH_SCALE ? double(a[i])*2 :
            double(a[i*4])*v[0]+double(a[i*4+1])*v[1]+double(a[i*4+2])*v[2];
        check(close(result[i], expected));
      }
    }
    ++cases;
  }
  std::array<float, 12> input{}, output;
  output.fill(99);
  check(!gf_math_evaluate(GF_MATH_NORMALIZE, input.data(), nullptr, 0, output.data()));
  input[0] = std::numeric_limits<float>::infinity();
  check(!gf_math_evaluate(GF_MATH_COPY, input.data(), nullptr, 0, output.data()));
  input[0] = std::numeric_limits<float>::quiet_NaN();
  check(!gf_math_evaluate(GF_MATH_COPY, input.data(), nullptr, 0, output.data()));
  check(!gf_math_evaluate(999, nullptr, nullptr, 0, output.data()));
  for (float x : output) check(x == 99);
  std::printf("{\"behavioral_property_cases\":%u,\"invalid_domain_controls\":4,\"all_passed\":true}\n", cases);
}
