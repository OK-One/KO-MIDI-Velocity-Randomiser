#pragma once

#include "pluginterfaces/base/funknown.h"
#include "pluginterfaces/vst/vsttypes.h"

namespace VelRand {

// Unique class IDs for this plug-in. These were randomly generated; keep them
// fixed for the life of the plug-in (hosts use them to recall projects).
static const Steinberg::FUID kProcessorUID (0x43C03456, 0xA4D34C2F, 0xA7142561, 0x8710271E);
static const Steinberg::FUID kControllerUID (0xFFDD3D12, 0x405141DC, 0x9312CA34, 0x1FA90EFA);

// Parameter IDs (must be unique within the plug-in and never change once released)
enum ParamIds : Steinberg::Vst::ParamID
{
	kParamDevHighId = 100, // max deviation at velocity 127 (0-127)
	kParamDevLowId = 101,  // max deviation at velocity 1 (0-127)
	kParamBypassId = 102,  // bypass (0 = processing, 1 = bypassed)

	// Read-only values the processor sends to the on-screen display
	kParamDbgCountId = 110,   // notes processed
	kParamDbgInId = 111,      // last input velocity
	kParamDbgOutId = 112,     // last output velocity
	kParamDbgAllowedId = 113, // allowed deviation for the last note
	kParamDbgMinDevId = 114,  // smallest deviation seen
	kParamDbgMaxDevId = 115,  // largest deviation seen
	kParamDbgLastDevId = 116  // deviation applied to the last note (output - input)
};

} // namespace VelRand
