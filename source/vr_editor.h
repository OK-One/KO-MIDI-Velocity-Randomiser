#pragma once

#include "public.sdk/source/vst/vsteditcontroller.h"

#include <memory>

namespace VelRand {

// A small native window (Windows only) showing the three controls and the debug readout.
// On other platforms it reports "not supported" and the host falls back to its generic UI.
class Editor : public Steinberg::Vst::EditorView
{
public:
	explicit Editor (Steinberg::Vst::EditController* controller);
	~Editor () override;

	Steinberg::tresult PLUGIN_API isPlatformTypeSupported (Steinberg::FIDString type) SMTG_OVERRIDE;
	void attachedToParent () SMTG_OVERRIDE;
	void removedFromParent () SMTG_OVERRIDE;

private:
	struct Impl;
	std::unique_ptr<Impl> impl;
};

} // namespace VelRand
