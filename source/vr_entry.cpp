#include "vr_cids.h"
#include "vr_controller.h"
#include "vr_processor.h"
#include "version.h"

#include "public.sdk/source/main/pluginfactory.h"

using namespace Steinberg;

BEGIN_FACTORY_DEF (VR_COMPANY_NAME, VR_COMPANY_WEB, VR_COMPANY_EMAIL)

	// Processor (the "component")
	DEF_CLASS2 (INLINE_UID_FROM_FUID (VelRand::kProcessorUID),
	            PClassInfo::kManyInstances,
	            kVstAudioEffectClass,
	            VR_PLUGIN_NAME,
	            Vst::kDistributable,
	            Vst::PlugType::kFx,
	            VR_VERSION_STR,
	            kVstVersionString,
	            VelRand::Processor::createInstance)

	// Controller
	DEF_CLASS2 (INLINE_UID_FROM_FUID (VelRand::kControllerUID),
	            PClassInfo::kManyInstances,
	            kVstComponentControllerClass,
	            VR_PLUGIN_NAME "Controller",
	            0,
	            "",
	            VR_VERSION_STR,
	            kVstVersionString,
	            VelRand::Controller::createInstance)

END_FACTORY
