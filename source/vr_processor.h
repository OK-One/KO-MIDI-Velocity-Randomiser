#pragma once

#include "public.sdk/source/vst/vstaudioeffect.h"
#include "vr_velocity.h"

namespace VelRand {

class Processor : public Steinberg::Vst::AudioEffect
{
public:
	Processor ();

	static Steinberg::FUnknown* createInstance (void*)
	{
		return static_cast<Steinberg::Vst::IAudioProcessor*> (new Processor ());
	}

	Steinberg::tresult PLUGIN_API initialize (Steinberg::FUnknown* context) SMTG_OVERRIDE;
	Steinberg::tresult PLUGIN_API canProcessSampleSize (Steinberg::int32 symbolicSampleSize) SMTG_OVERRIDE;
	Steinberg::tresult PLUGIN_API process (Steinberg::Vst::ProcessData& data) SMTG_OVERRIDE;
	Steinberg::tresult PLUGIN_API setState (Steinberg::IBStream* state) SMTG_OVERRIDE;
	Steinberg::tresult PLUGIN_API getState (Steinberg::IBStream* state) SMTG_OVERRIDE;

private:
	// Normalised (0..1) parameter values; plain value = normalised * 127 (stepCount 127).
	Steinberg::Vst::ParamValue devHighNorm = 5.0 / 127.0;
	Steinberg::Vst::ParamValue devLowNorm = 5.0 / 127.0;
	bool bypass = false;

	VelocityRandomiser randomiser;
};

} // namespace VelRand
