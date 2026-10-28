// Standalone test of the velocity maths. Build: g++ -std=c++17 -I../source test_velocity.cpp -o test_velocity
#include "vr_velocity.h"
#include <cstdio>
#include <cstdlib>

static int failures = 0;
#define CHECK(cond, ...) do { if (!(cond)) { ++failures; std::printf ("FAIL: " __VA_ARGS__); std::printf ("\n"); } } while (0)

int main ()
{
	VelRand::VelocityRandomiser r (12345);
	const int N = 20000;

	// 1) Bounds for many slider combinations and every velocity
	const double combos[][2] = {{10, 20}, {5, 5}, {0, 0}, {127, 127}, {0, 127}, {127, 0}, {10, 10}, {3, 40}};
	for (auto& c : combos)
	{
		const double hi = c[0], lo = c[1];
		for (int v = 1; v <= 127; ++v)
		{
			const double maxDev = lo + (hi - lo) * (v - 1) / 126.0;
			const int allowed = (int) std::floor (maxDev + 0.5);
			for (int n = 0; n < 200; ++n)
			{
				int out = (int) std::lround (r.apply (v / 127.0f, hi, lo) * 127.0f);
				CHECK (out >= 1 && out <= 127, "range: v=%d out=%d", v, out);
				CHECK (std::abs (out - v) <= allowed, "dev: hi=%g lo=%g v=%d out=%d allowed=%d", hi, lo, v, out, allowed);
				if (maxDev <= 0.0)
					CHECK (out == v, "zero dev must pass through: v=%d out=%d", v, out);
			}
		}
	}

	// 2) Velocity 0 is untouched
	CHECK (r.apply (0.0f, 50, 50) == 0.0f, "velocity 0 changed");

	// 3) Endpoint reachability + roughly symmetric around the input (mid velocity, hi=lo=10)
	{
		int minOut = 999, maxOut = -999; double sum = 0;
		for (int n = 0; n < N; ++n)
		{
			int out = (int) std::lround (r.apply (64 / 127.0f, 10, 10) * 127.0f);
			minOut = std::min (minOut, out); maxOut = std::max (maxOut, out); sum += out;
		}
		std::printf ("v=64, +/-10: min=%d max=%d mean=%.2f\n", minOut, maxOut, sum / N);
		CHECK (minOut == 54 && maxOut == 74, "endpoints not reached at v=64");
		CHECK (std::fabs (sum / N - 64.0) < 0.3, "mean not centred");
	}

	// 4) Interpolation: hi=10 at 127, lo=20 at 1 -> check max |dev| seen at 1, 64, 127
	for (int v : {1, 64, 127})
	{
		int maxAbs = 0;
		for (int n = 0; n < N; ++n)
			maxAbs = std::max (maxAbs, std::abs ((int) std::lround (r.apply (v / 127.0f, 10, 20) * 127.0f) - v));
		std::printf ("hi=10 lo=20 v=%d: max |dev| seen = %d\n", v, maxAbs);
	}

	// 5) Quiet note, big deviation: never 0
	for (int n = 0; n < N; ++n)
	{
		int out = (int) std::lround (r.apply (1 / 127.0f, 127, 127) * 127.0f);
		CHECK (out >= 1, "velocity 1 produced %d", out);
	}

	std::printf (failures ? "FAILED (%d)\n" : "ALL PASSED\n", failures);
	return failures ? 1 : 0;
}
