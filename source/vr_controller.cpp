#include "vr_controller.h"
#include "vr_cids.h"
#include "vr_midi.h"

#include "base/source/fstreamer.h"
#include "pluginterfaces/base/ustring.h"
#include "public.sdk/source/vst/vstparameters.h"

#include <cstdio>

using namespace Steinberg;

namespace VelRand {

//------------------------------------------------------------------------
tresult PLUGIN_API Controller::initialize (FUnknown* context)
{
	tresult result = EditController::initialize (context);
	if (result != kResultOk)
		return result;

	// Whole-number parameters 0..127 (127 steps), default 5.
	parameters.addParameter (new Vst::RangeParameter (
	    STR16 ("Max Deviation @ Vel 127"), kParamDevHighId, STR16 ("vel"), 0.0, 127.0, 5.0, 127,
	    Vst::ParameterInfo::kCanAutomate));

	parameters.addParameter (new Vst::RangeParameter (
	    STR16 ("Max Deviation @ Vel 1"), kParamDevLowId, STR16 ("vel"), 0.0, 127.0, 5.0, 127,
	    Vst::ParameterInfo::kCanAutomate));

	// Two states (off / on); kIsBypass tells the host this is the bypass parameter.
	parameters.addParameter (STR16 ("Bypass"), nullptr, 1, 0,
	                         Vst::ParameterInfo::kCanAutomate | Vst::ParameterInfo::kIsBypass,
	                         kParamBypassId);

	// Hidden parameters that carry MIDI controllers (sustain pedal, mod wheel, pitch bend,
	// aftertouch...) for all 16 channels. The host fills them via getMidiControllerAssignment()
	// and the processor sends each change on as a MIDI controller event.
	for (int ch = 0; ch < kNumMidiChannels; ++ch)
	{
		for (int ctrl = 0; ctrl < kNumMidiControllers; ++ctrl)
		{
			char ascii[48];
			std::snprintf (ascii, sizeof (ascii), "MIDI Ch %d Controller %d", ch + 1, ctrl);
			UString128 title;
			title.fromAscii (ascii);

			const double defaultNormalised = (ctrl == kControllerPitchBend) ? 0.5 : 0.0;
			parameters.addParameter (new Vst::Parameter (
			    title, midiParamId (ch, ctrl), nullptr, defaultNormalised, 0,
			    Vst::ParameterInfo::kIsHidden | Vst::ParameterInfo::kCanAutomate));
		}
	}

	return kResultTrue;
}

//------------------------------------------------------------------------
tresult PLUGIN_API Controller::getMidiControllerAssignment (int32 busIndex, int16 channel,
                                                            Vst::CtrlNumber midiControllerNumber,
                                                            Vst::ParamID& id)
{
	if (busIndex != 0 || channel < 0 || channel >= kNumMidiChannels || midiControllerNumber < 0 ||
	    midiControllerNumber >= kNumMidiControllers)
		return kResultFalse;

	id = midiParamId (channel, midiControllerNumber);
	return kResultTrue;
}

//------------------------------------------------------------------------
tresult PLUGIN_API Controller::setComponentState (IBStream* state)
{
	// Receives the same data the processor wrote in getState().
	if (!state)
		return kResultFalse;

	IBStreamer streamer (state, kLittleEndian);
	double high = 0.0, low = 0.0, byp = 0.0;
	if (!streamer.readDouble (high) || !streamer.readDouble (low) || !streamer.readDouble (byp))
		return kResultFalse;

	if (auto* p = parameters.getParameter (kParamDevHighId))
		p->setNormalized (high);
	if (auto* p = parameters.getParameter (kParamDevLowId))
		p->setNormalized (low);
	if (auto* p = parameters.getParameter (kParamBypassId))
		p->setNormalized (byp);

	return kResultOk;
}

} // namespace VelRand
