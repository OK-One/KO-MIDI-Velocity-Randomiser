#pragma once

// Pure C++ velocity randomisation logic (no SDK dependencies, so it can be unit tested).

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace VelRand {

class VelocityRandomiser
{
public:
	explicit VelocityRandomiser (uint32_t seed = 0x9E3779B9u) { setSeed (seed); }

	void setSeed (uint32_t seed) { state = seed ? seed : 0x9E3779B9u; }

	// Uniform random number in [0, 1). xorshift32: fast, no allocation, audio-thread safe.
	double nextUnit ()
	{
		state ^= state << 13;
		state ^= state >> 17;
		state ^= state << 5;
		return (state >> 8) * (1.0 / 16777216.0);
	}

	// What happened to one note (used for the on-screen display).
	struct Details
	{
		bool valid = false;           // true when the note was a real note-on (velocity >= 1)
		int inVelocity = 0;           // incoming MIDI velocity 1..127
		int outVelocity = 0;          // outgoing MIDI velocity 1..127
		double allowedDeviation = 0;  // max +/- deviation that applied to this note
	};

	// velocity01: VST 3 note-on velocity (0..1).
	// devHigh: max deviation (in MIDI velocity steps) for a note at velocity 127.
	// devLow:  max deviation (in MIDI velocity steps) for a note at velocity 1.
	// The allowed deviation is interpolated in a straight line between the two.
	// Results are whole MIDI velocities in 1..127. Velocity 0 is never touched.
	float apply (float velocity01, double devHigh, double devLow, Details* details = nullptr)
	{
		int v = static_cast<int> (std::lround (velocity01 * 127.0f));
		if (v < 1)
			return velocity01;
		if (v > 127)
			v = 127;

		const double maxDev = devLow + (devHigh - devLow) * (v - 1) / 126.0;
		if (details)
		{
			details->valid = true;
			details->inVelocity = v;
			details->outVelocity = v;
			details->allowedDeviation = std::max (0.0, maxDev);
		}
		if (maxDev <= 0.0)
			return velocity01; // nothing to randomise: pass through untouched

		const double dev = (nextUnit () * 2.0 - 1.0) * maxDev;
		int out = static_cast<int> (std::floor (v + dev + 0.5));
		out = std::max (1, std::min (127, out));
		if (details)
			details->outVelocity = out;
		return static_cast<float> (out) / 127.0f;
	}

private:
	uint32_t state;
};

} // namespace VelRand
