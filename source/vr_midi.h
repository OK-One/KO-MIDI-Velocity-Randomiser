#pragma once

// Pure C++ helpers for passing MIDI controllers (sustain pedal, mod wheel, pitch bend...)
// through the plug-in. No SDK dependencies, so it can be unit tested.
//
// VST 3 does not deliver MIDI controllers to a plug-in directly. The host converts them
// into parameter changes, but only for parameters the plug-in maps to MIDI controllers
// (IMidiMapping). We expose one hidden parameter per MIDI channel and controller, and the
// processor turns each change back into a MIDI controller output event.

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace VelRand {

constexpr int kNumMidiChannels = 16;
constexpr int kNumMidiControllers = 130;   // 0..127 = control change, 128 = aftertouch, 129 = pitch bend
constexpr int kControllerAfterTouch = 128;
constexpr int kControllerPitchBend = 129;

constexpr uint32_t kMidiParamBase = 1000;  // must not collide with the other parameter IDs
constexpr uint32_t kMidiParamCount = kNumMidiChannels * kNumMidiControllers;

inline uint32_t midiParamId (int channel, int controller)
{
	return kMidiParamBase + static_cast<uint32_t> (channel * kNumMidiControllers + controller);
}

inline bool isMidiParam (uint32_t id)
{
	return id >= kMidiParamBase && id < kMidiParamBase + kMidiParamCount;
}

inline int midiParamChannel (uint32_t id) { return static_cast<int> (id - kMidiParamBase) / kNumMidiControllers; }
inline int midiParamController (uint32_t id) { return static_cast<int> (id - kMidiParamBase) % kNumMidiControllers; }

struct MidiValue
{
	int value;  // 7-bit value (for pitch bend: the low 7 bits)
	int value2; // only used by pitch bend: the high 7 bits
};

// Converts a normalised (0..1) parameter value to MIDI data bytes.
inline MidiValue toMidiValue (int controller, double normalised)
{
	const double n = std::max (0.0, std::min (1.0, normalised));
	if (controller == kControllerPitchBend)
	{
		const int v14 = static_cast<int> (std::lround (n * 16383.0));
		return {v14 & 0x7F, (v14 >> 7) & 0x7F};
	}
	return {static_cast<int> (std::lround (n * 127.0)), 0};
}

} // namespace VelRand
