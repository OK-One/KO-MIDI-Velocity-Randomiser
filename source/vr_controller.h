#pragma once

#include "pluginterfaces/vst/ivstmidicontrollers.h"
#include "public.sdk/source/vst/vsteditcontroller.h"

namespace VelRand {

class Controller : public Steinberg::Vst::EditController, public Steinberg::Vst::IMidiMapping
{
public:
	static Steinberg::FUnknown* createInstance (void*)
	{
		return static_cast<Steinberg::Vst::IEditController*> (new Controller ());
	}

	Steinberg::tresult PLUGIN_API initialize (Steinberg::FUnknown* context) SMTG_OVERRIDE;
	Steinberg::tresult PLUGIN_API setComponentState (Steinberg::IBStream* state) SMTG_OVERRIDE;
	Steinberg::IPlugView* PLUGIN_API createView (Steinberg::FIDString name) SMTG_OVERRIDE;

	// IMidiMapping: tells the host which parameter receives each MIDI controller, so the
	// host will pass sustain pedal, mod wheel, pitch bend etc. to the plug-in.
	Steinberg::tresult PLUGIN_API getMidiControllerAssignment (Steinberg::int32 busIndex,
	                                                           Steinberg::int16 channel,
	                                                           Steinberg::Vst::CtrlNumber midiControllerNumber,
	                                                           Steinberg::Vst::ParamID& id) SMTG_OVERRIDE;

	OBJ_METHODS (Controller, Steinberg::Vst::EditController)
	DEFINE_INTERFACES
		DEF_INTERFACE (Steinberg::Vst::IMidiMapping)
	END_DEFINE_INTERFACES (Steinberg::Vst::EditController)
	DELEGATE_REFCOUNT (Steinberg::Vst::EditController)
};

} // namespace VelRand
