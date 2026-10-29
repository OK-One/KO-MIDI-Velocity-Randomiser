#include "vr_processor.h"
#include "vr_cids.h"

#include "base/source/fstreamer.h"
#include "pluginterfaces/vst/ivstevents.h"
#include "pluginterfaces/vst/ivstmidicontrollers.h"
#include "pluginterfaces/vst/ivstparameterchanges.h"

#include <cmath>
#include <cstring>
#include <random>

using namespace Steinberg;

static_assert (Vst::kAfterTouch == VelRand::kControllerAfterTouch &&
                   Vst::kPitchBend == VelRand::kControllerPitchBend,
               "MIDI controller numbers differ from the SDK's: check vr_midi.h");

namespace VelRand {

namespace {

//------------------------------------------------------------------------
// Turns a change of a hidden MIDI parameter back into a MIDI controller output event.
void makeControllerEvent (Vst::Event& e, Vst::ParamID id, int32 sampleOffset, double value)
{
	const int controller = midiParamController (id);
	const MidiValue mv = toMidiValue (controller, value);

	std::memset (&e, 0, sizeof (e));
	e.busIndex = 0;
	e.sampleOffset = sampleOffset;
	e.ppqPosition = 0.0;
	e.flags = Vst::Event::kIsLive;
	e.type = Vst::Event::kLegacyMIDICCOutEvent;
	e.midiCCOut.controlNumber = static_cast<uint8> (controller);
	e.midiCCOut.channel = static_cast<int8> (midiParamChannel (id));
	e.midiCCOut.value = static_cast<int8> (mv.value);
	e.midiCCOut.value2 = static_cast<int8> (mv.value2);
}

} // namespace

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
	// 1) Parameter changes.
	//    - The three real parameters: use the last value in each queue for this block.
	//    - The hidden MIDI parameters (sustain pedal, mod wheel, pitch bend...): every point
	//      becomes a MIDI controller output event, so nothing the host sends is lost.
	int32 numControllerEvents = 0;

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

			const Vst::ParamID id = queue->getParameterId ();

			if (isMidiParam (id))
			{
				for (int32 p = 0; p < numPoints && numControllerEvents < kMaxControllerEvents; ++p)
				{
					Vst::ParamValue value = 0.0;
					int32 sampleOffset = 0;
					if (queue->getPoint (p, sampleOffset, value) == kResultTrue)
						makeControllerEvent (controllerEvents[numControllerEvents++], id, sampleOffset, value);
				}
				continue;
			}

			Vst::ParamValue value = 0.0;
			int32 sampleOffset = 0;
			if (queue->getPoint (numPoints - 1, sampleOffset, value) != kResultTrue)
				continue;

			switch (id)
			{
				case kParamDevHighId: devHighNorm = value; break;
				case kParamDevLowId: devLowNorm = value; break;
				case kParamBypassId: bypass = value > 0.5; break;
				default: break;
			}
		}
	}

	if (!data.outputEvents)
		return kResultOk;

	// Keep controller events in time order (insertion sort: small n, stable, no allocation).
	for (int32 a = 1; a < numControllerEvents; ++a)
	{
		const Vst::Event key = controllerEvents[a];
		int32 b = a - 1;
		while (b >= 0 && controllerEvents[b].sampleOffset > key.sampleOffset)
		{
			controllerEvents[b + 1] = controllerEvents[b];
			--b;
		}
		controllerEvents[b + 1] = key;
	}

	// 2) Events: copy every incoming event through (only note-on velocities are changed),
	//    merging in the controller events by time.
	// Whole-number slider values (the parameters have 127 steps).
	const double devHigh = std::round (devHighNorm * 127.0);
	const double devLow = std::round (devLowNorm * 127.0);

	int32 nextController = 0;
	const int32 numEvents = data.inputEvents ? data.inputEvents->getEventCount () : 0;
	for (int32 i = 0; i < numEvents; ++i)
	{
		Vst::Event e;
		if (data.inputEvents->getEvent (i, e) != kResultOk)
			continue;

		while (nextController < numControllerEvents &&
		       controllerEvents[nextController].sampleOffset <= e.sampleOffset)
			data.outputEvents->addEvent (controllerEvents[nextController++]);

		if (!bypass && e.type == Vst::Event::kNoteOnEvent)
		{
			VelocityRandomiser::Details details;
			e.noteOn.velocity = randomiser.apply (e.noteOn.velocity, devHigh, devLow, &details);
			if (details.valid)
			{
				stats.record (details.inVelocity, details.outVelocity, details.allowedDeviation);
				statsChanged = true;
			}
		}

		data.outputEvents->addEvent (e);
	}

	while (nextController < numControllerEvents)
		data.outputEvents->addEvent (controllerEvents[nextController++]);

	// 3) Tell the on-screen display about any notes processed in this block.
	if (statsChanged)
	{
		sendDebugValues (data);
		statsChanged = false;
	}

	return kResultOk;
}

//------------------------------------------------------------------------
void Processor::sendDebugValues (Vst::ProcessData& data)
{
	if (!data.outputParameterChanges)
		return;

	auto send = [&data] (Vst::ParamID id, double value) {
		int32 queueIndex = 0;
		if (Vst::IParamValueQueue* queue = data.outputParameterChanges->addParameterData (id, queueIndex))
		{
			int32 pointIndex = 0;
			queue->addPoint (0, value, pointIndex);
		}
	};

	send (kParamDbgCountId, encodeCount (stats.count ()));
	send (kParamDbgInId, encodeVelocity (stats.lastInput ()));
	send (kParamDbgOutId, encodeVelocity (stats.lastOutput ()));
	send (kParamDbgAllowedId, encodeAllowed (stats.lastAllowedDeviation ()));
	send (kParamDbgLastDevId, encodeDeviation (stats.lastDeviation ()));
	send (kParamDbgMinDevId, encodeDeviation (stats.smallestDeviation ()));
	send (kParamDbgMaxDevId, encodeDeviation (stats.largestDeviation ()));
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
