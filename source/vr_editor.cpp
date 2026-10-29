#include "vr_editor.h"
#include "vr_cids.h"
#include "vr_debug.h"

#include <cstring>

#if defined(_WIN32)

#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <commctrl.h>

#include <cstdio>
#include <cmath>
#include <cwchar>
#include <string>

#pragma comment(lib, "comctl32.lib")

#endif // _WIN32

using namespace Steinberg;

namespace VelRand {

#if defined(_WIN32)

namespace {

constexpr int kEditorWidth = 450;
constexpr int kEditorHeight = 300;
constexpr UINT_PTR kTimerId = 1;
constexpr UINT kTimerMs = 50;

enum ControlIds
{
	kIdSliderHigh = 1001,
	kIdSliderLow = 1002,
	kIdBypass = 1003
};

const wchar_t* const kWindowClass = L"VelocityRandomiserEditorWindow";
int windowClassRefs = 0;

void moduleAnchor () {} // its address identifies this DLL

HINSTANCE moduleInstance ()
{
	HINSTANCE inst = nullptr;
	GetModuleHandleExW (GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
	                        GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
	                    reinterpret_cast<LPCWSTR> (&moduleAnchor), &inst);
	return inst;
}

void setTextIfChanged (HWND window, const wchar_t* text)
{
	if (!window)
		return;
	wchar_t current[160] = {};
	GetWindowTextW (window, current, 160);
	if (std::wcscmp (current, text) != 0)
		SetWindowTextW (window, text);
}

} // namespace

//------------------------------------------------------------------------
struct Editor::Impl
{
	Editor* owner = nullptr;
	HWND window = nullptr;
	bool holdsClassRef = false;
	HWND sliderHigh = nullptr;
	HWND sliderLow = nullptr;
	HWND valueHigh = nullptr;
	HWND valueLow = nullptr;
	HWND bypassBox = nullptr;
	HWND debugLine[6] = {};

	static LRESULT CALLBACK windowProc (HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);

	bool create (HWND parent);
	void destroy ();
	void createControls ();
	HWND makeLabel (const wchar_t* text, int x, int y, int w, int h);
	void setParam (Vst::ParamID id, double normalised);
	void onScroll (HWND source);
	void onBypassClicked ();
	void syncSlider (HWND slider, HWND valueLabel, Vst::ParamID id);
	void refresh ();
};

//------------------------------------------------------------------------
LRESULT CALLBACK Editor::Impl::windowProc (HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
	if (msg == WM_NCCREATE)
	{
		auto* cs = reinterpret_cast<CREATESTRUCTW*> (lp);
		SetWindowLongPtrW (hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR> (cs->lpCreateParams));
		if (auto* self = static_cast<Impl*> (cs->lpCreateParams))
			self->window = hwnd;
		return DefWindowProcW (hwnd, msg, wp, lp);
	}

	auto* self = reinterpret_cast<Impl*> (GetWindowLongPtrW (hwnd, GWLP_USERDATA));
	if (!self)
		return DefWindowProcW (hwnd, msg, wp, lp);

	switch (msg)
	{
		case WM_CREATE:
			self->createControls ();
			return 0;

		case WM_HSCROLL:
			self->onScroll (reinterpret_cast<HWND> (lp));
			return 0;

		case WM_COMMAND:
			if (LOWORD (wp) == kIdBypass && HIWORD (wp) == BN_CLICKED)
				self->onBypassClicked ();
			return 0;

		case WM_TIMER:
			if (wp == kTimerId)
				self->refresh ();
			return 0;

		case WM_DESTROY:
			KillTimer (hwnd, kTimerId);
			return 0;

		case WM_NCDESTROY:
			self->window = nullptr;
			self->sliderHigh = self->sliderLow = self->valueHigh = self->valueLow = nullptr;
			self->bypassBox = nullptr;
			for (HWND& h : self->debugLine)
				h = nullptr;
			SetWindowLongPtrW (hwnd, GWLP_USERDATA, 0);
			return DefWindowProcW (hwnd, msg, wp, lp);

		default:
			break;
	}
	return DefWindowProcW (hwnd, msg, wp, lp);
}

//------------------------------------------------------------------------
bool Editor::Impl::create (HWND parent)
{
	if (!parent || window)
		return false;

	HINSTANCE inst = moduleInstance ();

	if (windowClassRefs == 0)
	{
		INITCOMMONCONTROLSEX icc = {sizeof (INITCOMMONCONTROLSEX), ICC_BAR_CLASSES};
		InitCommonControlsEx (&icc);

		WNDCLASSEXW wc = {};
		wc.cbSize = sizeof (wc);
		wc.lpfnWndProc = &Impl::windowProc;
		wc.hInstance = inst;
		wc.hCursor = LoadCursorW (nullptr, IDC_ARROW);
		wc.hbrBackground = reinterpret_cast<HBRUSH> (static_cast<INT_PTR> (COLOR_BTNFACE + 1));
		wc.lpszClassName = kWindowClass;
		if (!RegisterClassExW (&wc))
			return false;
	}
	++windowClassRefs;

	CreateWindowExW (0, kWindowClass, L"", WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN, 0, 0,
	                 kEditorWidth, kEditorHeight, parent, nullptr, inst, this);
	if (!window)
	{
		if (--windowClassRefs == 0)
			UnregisterClassW (kWindowClass, inst);
		return false;
	}
	holdsClassRef = true;
	return true;
}

//------------------------------------------------------------------------
void Editor::Impl::destroy ()
{
	if (window)
		DestroyWindow (window);
	window = nullptr;

	if (holdsClassRef)
	{
		holdsClassRef = false;
		if (--windowClassRefs == 0)
			UnregisterClassW (kWindowClass, moduleInstance ());
	}
}

//------------------------------------------------------------------------
HWND Editor::Impl::makeLabel (const wchar_t* text, int x, int y, int w, int h)
{
	HWND label = CreateWindowExW (0, L"STATIC", text, WS_CHILD | WS_VISIBLE, x, y, w, h, window,
	                              nullptr, moduleInstance (), nullptr);
	SendMessageW (label, WM_SETFONT, reinterpret_cast<WPARAM> (GetStockObject (DEFAULT_GUI_FONT)), TRUE);
	return label;
}

//------------------------------------------------------------------------
void Editor::Impl::createControls ()
{
	HINSTANCE inst = moduleInstance ();
	HFONT font = static_cast<HFONT> (GetStockObject (DEFAULT_GUI_FONT));

	makeLabel (L"Max deviation at velocity 127", 10, 10, 300, 18);
	valueHigh = makeLabel (L"", 380, 10, 60, 18);
	sliderHigh = CreateWindowExW (0, TRACKBAR_CLASSW, L"", WS_CHILD | WS_VISIBLE | TBS_HORZ | TBS_NOTICKS,
	                              10, 30, 430, 28, window, reinterpret_cast<HMENU> (static_cast<INT_PTR> (kIdSliderHigh)),
	                              inst, nullptr);
	SendMessageW (sliderHigh, TBM_SETRANGE, TRUE, MAKELPARAM (0, 127));

	makeLabel (L"Max deviation at velocity 1", 10, 70, 300, 18);
	valueLow = makeLabel (L"", 380, 70, 60, 18);
	sliderLow = CreateWindowExW (0, TRACKBAR_CLASSW, L"", WS_CHILD | WS_VISIBLE | TBS_HORZ | TBS_NOTICKS,
	                             10, 90, 430, 28, window, reinterpret_cast<HMENU> (static_cast<INT_PTR> (kIdSliderLow)),
	                             inst, nullptr);
	SendMessageW (sliderLow, TBM_SETRANGE, TRUE, MAKELPARAM (0, 127));

	bypassBox = CreateWindowExW (0, L"BUTTON", L"Bypass", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX, 10, 130,
	                             200, 22, window, reinterpret_cast<HMENU> (static_cast<INT_PTR> (kIdBypass)),
	                             inst, nullptr);
	SendMessageW (bypassBox, WM_SETFONT, reinterpret_cast<WPARAM> (font), TRUE);

	for (int i = 0; i < 6; ++i)
		debugLine[i] = makeLabel (L"", 10, 170 + i * 20, 430, 18);

	refresh ();
	SetTimer (window, kTimerId, kTimerMs, nullptr);
}

//------------------------------------------------------------------------
void Editor::Impl::setParam (Vst::ParamID id, double normalised)
{
	Vst::EditController* controller = owner->getController ();
	if (!controller)
		return;
	controller->beginEdit (id);
	controller->setParamNormalized (id, normalised);
	controller->performEdit (id, normalised);
	controller->endEdit (id);
}

//------------------------------------------------------------------------
void Editor::Impl::onScroll (HWND source)
{
	if (source != sliderHigh && source != sliderLow)
		return;
	const LRESULT pos = SendMessageW (source, TBM_GETPOS, 0, 0);
	setParam (source == sliderHigh ? kParamDevHighId : kParamDevLowId, static_cast<double> (pos) / 127.0);
	refresh ();
}

//------------------------------------------------------------------------
void Editor::Impl::onBypassClicked ()
{
	const bool checked = SendMessageW (bypassBox, BM_GETCHECK, 0, 0) == BST_CHECKED;
	setParam (kParamBypassId, checked ? 1.0 : 0.0);
}

//------------------------------------------------------------------------
void Editor::Impl::syncSlider (HWND slider, HWND valueLabel, Vst::ParamID id)
{
	Vst::EditController* controller = owner->getController ();
	if (!controller || !slider)
		return;

	const int pos = static_cast<int> (std::lround (controller->getParamNormalized (id) * 127.0));
	if (SendMessageW (slider, TBM_GETPOS, 0, 0) != pos)
		SendMessageW (slider, TBM_SETPOS, TRUE, pos);

	wchar_t text[32];
	std::swprintf (text, 32, L"%d", pos);
	setTextIfChanged (valueLabel, text);
}

//------------------------------------------------------------------------
void Editor::Impl::refresh ()
{
	Vst::EditController* controller = owner->getController ();
	if (!controller || !window)
		return;

	// Controls follow the host (automation, presets, project load)
	syncSlider (sliderHigh, valueHigh, kParamDevHighId);
	syncSlider (sliderLow, valueLow, kParamDevLowId);

	const bool bypassed = controller->getParamNormalized (kParamBypassId) > 0.5;
	if ((SendMessageW (bypassBox, BM_GETCHECK, 0, 0) == BST_CHECKED) != bypassed)
		SendMessageW (bypassBox, BM_SETCHECK, bypassed ? BST_CHECKED : BST_UNCHECKED, 0);

	// Debug readout (same information as the JSFX version)
	const uint32_t count = decodeCount (controller->getParamNormalized (kParamDbgCountId));
	wchar_t text[160];

	std::swprintf (text, 160, L"Notes processed: %u", static_cast<unsigned> (count));
	setTextIfChanged (debugLine[0], text);

	if (count == 0)
	{
		setTextIfChanged (debugLine[1], L"Last input velocity: -");
		setTextIfChanged (debugLine[2], L"Last output velocity: -");
		setTextIfChanged (debugLine[3], L"Deviation applied to last note: -");
		setTextIfChanged (debugLine[4], L"Allowed deviation for last note: -");
		setTextIfChanged (debugLine[5], L"Smallest / largest deviation seen: -");
		return;
	}

	std::swprintf (text, 160, L"Last input velocity: %d",
	               decodeVelocity (controller->getParamNormalized (kParamDbgInId)));
	setTextIfChanged (debugLine[1], text);

	std::swprintf (text, 160, L"Last output velocity: %d",
	               decodeVelocity (controller->getParamNormalized (kParamDbgOutId)));
	setTextIfChanged (debugLine[2], text);

	std::swprintf (text, 160, L"Deviation applied to last note: %+d",
	               decodeDeviation (controller->getParamNormalized (kParamDbgLastDevId)));
	setTextIfChanged (debugLine[3], text);

	std::swprintf (text, 160, L"Allowed deviation for last note: +/- %.2f",
	               decodeAllowed (controller->getParamNormalized (kParamDbgAllowedId)));
	setTextIfChanged (debugLine[4], text);

	std::swprintf (text, 160, L"Smallest / largest deviation seen: %d / %d",
	               decodeDeviation (controller->getParamNormalized (kParamDbgMinDevId)),
	               decodeDeviation (controller->getParamNormalized (kParamDbgMaxDevId)));
	setTextIfChanged (debugLine[5], text);
}

#else // not Windows

struct Editor::Impl
{
};

#endif

//------------------------------------------------------------------------
namespace {

ViewRect* initialRect ()
{
#if defined(_WIN32)
	static ViewRect rect (0, 0, kEditorWidth, kEditorHeight);
#else
	static ViewRect rect (0, 0, 450, 300);
#endif
	return &rect;
}

} // namespace

//------------------------------------------------------------------------
Editor::Editor (Vst::EditController* controller)
: EditorView (controller, initialRect ()), impl (new Impl)
{
#if defined(_WIN32)
	impl->owner = this;
#endif
}

//------------------------------------------------------------------------
Editor::~Editor () = default;

//------------------------------------------------------------------------
tresult PLUGIN_API Editor::isPlatformTypeSupported (FIDString type)
{
#if defined(_WIN32)
	if (type && std::strcmp (type, kPlatformTypeHWND) == 0)
		return kResultTrue;
#else
	(void)type;
#endif
	return kResultFalse;
}

//------------------------------------------------------------------------
void Editor::attachedToParent ()
{
	EditorView::attachedToParent ();
#if defined(_WIN32)
	impl->create (static_cast<HWND> (systemWindow));
#endif
}

//------------------------------------------------------------------------
void Editor::removedFromParent ()
{
#if defined(_WIN32)
	impl->destroy ();
#endif
	EditorView::removedFromParent ();
}

} // namespace VelRand
