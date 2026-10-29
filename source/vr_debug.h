#pragma once

// Pure C++ helpers for the on-screen debug display (no SDK dependencies, so unit tested).
//
// The processor cannot talk to the editor directly, so it publishes its numbers as read-only
// parameters (normalised 0..1). These functions encode and decode them.

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace VelRand {

constexpr uint32_t kDebugCountWrap = 1000000; // the notes counter wraps back to 0 after this

inline double debugClamp01 (double v) { return std::max (0.0, std::min (1.0, v)); }

inline double encodeVelocity (int v) { return debugClamp01 (v / 127.0); }
inline int decodeVelocity (double n) { return static_cast<int> (std::lround (debugClamp01 (n) * 127.0)); }

inline double encodeAllowed (double a) { return debugClamp01 (a / 127.0); }
inline double decodeAllowed (double n) { return debugClamp01 (n) * 127.0; }

inline double encodeCount (uint32_t c) { return (c % kDebugCountWrap) / static_cast<double> (kDebugCountWrap); }
inline uint32_t decodeCount (double n)
{
	return static_cast<uint32_t> (std::lround (debugClamp01 (n) * static_cast<double> (kDebugCountWrap)));
}

// Deviation range is -127..+127
inline double encodeDeviation (int d) { return (std::max (-127, std::min (127, d)) + 127) / 254.0; }
inline int decodeDeviation (double n) { return static_cast<int> (std::lround (debugClamp01 (n) * 254.0)) - 127; }

// Running statistics, equivalent to the JSFX debug display.
class DebugStats
{
public:
	void record (int inVelocity, int outVelocity, double allowedDeviation)
	{
		++notes;
		lastIn = inVelocity;
		lastOut = outVelocity;
		lastAllowed = allowedDeviation;

		const int d = outVelocity - inVelocity;
		if (notes == 1)
			minDev = maxDev = d;
		else
		{
			minDev = std::min (minDev, d);
			maxDev = std::max (maxDev, d);
		}
	}

	uint32_t count () const { return notes; }
	int lastInput () const { return lastIn; }
	int lastOutput () const { return lastOut; }
	double lastAllowedDeviation () const { return lastAllowed; }
	int lastDeviation () const { return lastOut - lastIn; } // deviation applied to the last note
	int smallestDeviation () const { return minDev; }
	int largestDeviation () const { return maxDev; }

private:
	uint32_t notes = 0;
	int lastIn = 0;
	int lastOut = 0;
	double lastAllowed = 0.0;
	int minDev = 0;
	int maxDev = 0;
};

} // namespace VelRand
