// Standalone test. Build: g++ -std=c++17 -I../source test_debug.cpp -o test_debug
#include "vr_debug.h"
#include "vr_velocity.h"
#include <cstdio>
#include <cstdlib>

static int failures = 0;
#define CHECK(cond, ...) do { if (!(cond)) { ++failures; std::printf ("FAIL: " __VA_ARGS__); std::printf ("\n"); } } while (0)

int main ()
{
	using namespace VelRand;

	// Encode/decode round trips through a normalised parameter
	for (int v = 0; v <= 127; ++v)
		CHECK (decodeVelocity (encodeVelocity (v)) == v, "velocity %d", v);
	for (int d = -127; d <= 127; ++d)
		CHECK (decodeDeviation (encodeDeviation (d)) == d, "deviation %d", d);
	for (uint32_t c : {0u, 1u, 7u, 1234u, 999999u})
		CHECK (decodeCount (encodeCount (c)) == c, "count %u", c);
	CHECK (decodeCount (encodeCount (1000000u)) == 0u, "count wraps to 0");
	CHECK (std::fabs (decodeAllowed (encodeAllowed (12.34)) - 12.34) < 1e-9, "allowed");

	// Running statistics
	DebugStats s;
	s.record (64, 70, 5.0);   // +6
	s.record (64, 58, 5.0);   // -6
	s.record (100, 100, 0.0); // 0
	CHECK (s.count () == 3, "count");
	CHECK (s.lastInput () == 100 && s.lastOutput () == 100, "last in/out");
	CHECK (s.smallestDeviation () == -6 && s.largestDeviation () == 6, "min/max");

	// apply() details agree with what it returns
	VelocityRandomiser r (99);
	for (int i = 0; i < 20000; ++i)
	{
		const int v = 1 + (i % 127);
		VelocityRandomiser::Details det;
		float out01 = r.apply (v / 127.0f, 10, 20, &det);
		int out = (int) std::lround (out01 * 127.0f);
		CHECK (det.valid && det.inVelocity == v, "details in");
		CHECK (det.outVelocity == out, "details out %d vs %d", det.outVelocity, out);
		const double expect = 20 + (10 - 20) * (v - 1) / 126.0;
		CHECK (std::fabs (det.allowedDeviation - expect) < 1e-9, "allowed %g vs %g", det.allowedDeviation, expect);
	}
	VelocityRandomiser::Details zero;
	r.apply (0.0f, 10, 10, &zero);
	CHECK (!zero.valid, "velocity 0 must not be reported");
	VelocityRandomiser::Details none;
	float same = r.apply (64 / 127.0f, 0, 0, &none);
	CHECK (same == 64 / 127.0f && none.valid && none.allowedDeviation == 0.0 && none.outVelocity == 64, "zero deviation");

	std::printf (failures ? "FAILED (%d)\n" : "ALL PASSED\n", failures);
	return failures ? 1 : 0;
}
