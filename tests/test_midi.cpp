// Standalone test. Build: g++ -std=c++17 -I../source test_midi.cpp -o test_midi
#include "vr_midi.h"
#include <cstdio>
#include <set>

static int failures = 0;
#define CHECK(cond, ...) do { if (!(cond)) { ++failures; std::printf ("FAIL: " __VA_ARGS__); std::printf ("\n"); } } while (0)

int main ()
{
	using namespace VelRand;

	// Every (channel, controller) pair gets a unique ID that decodes back correctly
	std::set<uint32_t> seen;
	for (int ch = 0; ch < kNumMidiChannels; ++ch)
		for (int c = 0; c < kNumMidiControllers; ++c)
		{
			const uint32_t id = midiParamId (ch, c);
			CHECK (isMidiParam (id), "id %u not recognised", id);
			CHECK (midiParamChannel (id) == ch && midiParamController (id) == c, "decode ch=%d c=%d", ch, c);
			CHECK (seen.insert (id).second, "duplicate id %u", id);
		}
	CHECK (seen.size () == (size_t) kMidiParamCount, "wrong count");

	// Does not collide with the plug-in's own parameters (100..102) and rejects neighbours
	CHECK (!isMidiParam (100) && !isMidiParam (101) && !isMidiParam (102), "collides with own params");
	CHECK (!isMidiParam (kMidiParamBase - 1) && !isMidiParam (kMidiParamBase + kMidiParamCount), "range edges");

	// Sustain pedal (CC 64): down = 127, up = 0
	CHECK (toMidiValue (64, 1.0).value == 127, "pedal down");
	CHECK (toMidiValue (64, 0.0).value == 0, "pedal up");
	CHECK (toMidiValue (1, 0.5).value == 64, "mod wheel half");

	// Pitch bend: centre = 8192 (LSB 0, MSB 64), full up = 16383, full down = 0
	MidiValue c = toMidiValue (kControllerPitchBend, 0.5);
	CHECK (c.value == 0 && c.value2 == 64, "pitch bend centre: %d %d", c.value, c.value2);
	MidiValue u = toMidiValue (kControllerPitchBend, 1.0);
	CHECK (u.value == 127 && u.value2 == 127, "pitch bend max");
	MidiValue d = toMidiValue (kControllerPitchBend, 0.0);
	CHECK (d.value == 0 && d.value2 == 0, "pitch bend min");

	// Out-of-range input is clamped
	CHECK (toMidiValue (64, 7.0).value == 127 && toMidiValue (64, -3.0).value == 0, "clamping");

	std::printf (failures ? "FAILED (%d)\n" : "ALL PASSED\n", failures);
	return failures ? 1 : 0;
}
