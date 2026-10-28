#include "vr_controller.h"
#include "vr_cids.h"

#include "base/source/fstreamer.h"
#include "public.sdk/source/vst/vstparameters.h"

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
