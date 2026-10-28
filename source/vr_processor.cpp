#include "vr_processor.h"
#include "vr_cids.h"

#include "base/source/fstreamer.h"
#include "pluginterfaces/vst/ivstevents.h"
#include "pluginterfaces/vst/ivstparameterchanges.h"

#include <cmath>
#include <random>

using namespace Steinberg;

namespace VelRand {

//------------------------------------------------------------------------
Processor::Processor ()
{
	setControllerClass (kControllerUID);

	// Seed once, here (not on the audio thread).
	std::random_device rd;
	randomiser.setSeed (rd ());
}

//------------------------------------------------------------------------
tresult PLUGIN_API Processor::initialize (FUnknown* context)
{
	tresult result = AudioEffect::initialize (context);
	if (result != kResultOk)
		return result;

	// MIDI-style event buses only (16 channels each). No audio buses.
	addEventInput (STR16 ("Event In"), 16);
	addEventOutput (STR16 ("Event Out"), 16);

	return kResultOk;
}

//------------------------------------------------------------------------
tresult PLUGIN_API Processor::canProcessSampleSize (int32 symbolicSampleSize)
{
	// No audio is processed, so either sample size is fine.
	return (symbolicSampleSize == Vst::kSample32 || symbolicSampleSize == Vst::kSample64)
	           ? kResultTrue
	           : kResultFalse;
}

//------------------------------------------------------------------------
tresult PLUGIN_API Processor::process (Vst::ProcessData& data)
{
	// 1) Parameter changes: use the last value in each queue for this block.
	if (data.inputParameterChanges)
	{
		const int32 numChanged = data.inputParameterChanges->getParameterCount ();
		for (int32 i = 0; i < numChanged; ++i)
		{
			Vst::IParamValueQueue* queue = data.inputParameterChanges->getParameterData (i);
			if (!queue)
				continue;

			const int32 numPoints = queue->getPointCount ();
			if (numPoints <= 0)
				continue;

			Vst::ParamValue value = 0.0;
			int32 sampleOffset = 0;
			if (queue->getPoint (numPoints - 1, sampleOffset, value) != kResultTrue)
				continue;

			switch (queue->getParameterId ())
			{
				case kParamDevHighId: devHighNorm = value; break;
				case kParamDevLowId: devLowNorm = value; break;
				case kParamBypassId: bypass = value > 0.5; break;
				default: break;
			}
		}
	}

	// 2) Events: copy every event through; only note-on velocities are changed.
	if (!data.inputEvents || !data.outputEvents)
		return kResultOk;

	// Whole-number slider values (the parameters have 127 steps).
	const double devHigh = std::round (devHighNorm * 127.0);
	const double devLow = std::round (devLowNorm * 127.0);

	const int32 numEvents = data.inputEvents->getEventCount ();
	for (int32 i = 0; i < numEvents; ++i)
	{
		Vst::Event e;
		if (data.inputEvents->getEvent (i, e) != kResultOk)
			continue;

		if (!bypass && e.type == Vst::Event::kNoteOnEvent)
			e.noteOn.velocity = randomiser.apply (e.noteOn.velocity, devHigh, devLow);

		data.outputEvents->addEvent (e);
	}

	return kResultOk;
}

//------------------------------------------------------------------------
tresult PLUGIN_API Processor::setState (IBStream* state)
{
	if (!state)
		return kResultFalse;

	IBStreamer streamer (state, kLittleEndian);
	double high = 0.0, low = 0.0, byp = 0.0;
	if (!streamer.readDouble (high) || !streamer.readDouble (low) || !streamer.readDouble (byp))
		return kResultFalse;

	devHighNorm = high;
	devLowNorm = low;
	bypass = byp > 0.5;
	return kResultOk;
}

//------------------------------------------------------------------------
tresult PLUGIN_API Processor::getState (IBStream* state)
{
	IBStreamer streamer (state, kLittleEndian);
	streamer.writeDouble (devHighNorm);
	streamer.writeDouble (devLowNorm);
	streamer.writeDouble (bypass ? 1.0 : 0.0);
	return kResultOk;
}

} // namespace VelRand
