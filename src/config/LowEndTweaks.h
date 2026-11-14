#pragma once

#include "config/CemuConfig.h"
#include <cstdlib>

// Cemu Low-End: small host-side (Windows) tweaks. Nothing here touches audio, emulation state or guest memory.
// Header-only on purpose so no build-system changes are needed.

#if BOOST_OS_WINDOWS

#include <windows.h>

namespace LowEndTweaks
{
	// Opt out of EcoQoS / power throttling so Windows does not park the emulator on slow/efficiency cores.
	// enable=false hands the decision back to the system (default behavior).
	inline void SetPowerThrottlingDisabled(bool disabled)
	{
		PROCESS_POWER_THROTTLING_STATE state{};
		state.Version = PROCESS_POWER_THROTTLING_CURRENT_VERSION;
		if (disabled)
		{
			state.ControlMask = PROCESS_POWER_THROTTLING_EXECUTION_SPEED;
			state.StateMask = 0; // throttling explicitly off
		}
		else
		{
			state.ControlMask = 0; // system managed
			state.StateMask = 0;
		}
		SetProcessInformation(GetCurrentProcess(), ProcessPowerThrottling, &state, sizeof(state));
	}

	// 0 = normal, 1 = above normal (stock Cemu), 2 = high
	inline void SetProcessPriorityLevel(sint32 level)
	{
		DWORD priorityClass = NORMAL_PRIORITY_CLASS;
		if (level >= 2)
			priorityClass = HIGH_PRIORITY_CLASS;
		else if (level == 1)
			priorityClass = ABOVE_NORMAL_PRIORITY_CLASS;
		SetPriorityClass(GetCurrentProcess(), priorityClass);
	}

	// Windows 10 (1709+) / Windows 11 "power mode" overlay: switches to "Best performance" while Cemu runs and
	// puts the previous mode back when the option is switched off or Cemu exits. On low power CPUs the default
	// mode can keep the clock speed low. Loaded dynamically, so nothing happens on Windows versions without it.
	inline void SetBestPerformancePowerMode(bool enabled)
	{
		typedef DWORD(WINAPI * PowerSetActiveOverlayScheme_t)(GUID);
		typedef DWORD(WINAPI * PowerGetActualOverlayScheme_t)(GUID*);
		static HMODULE s_powrprof = LoadLibraryW(L"powrprof.dll");
		if (!s_powrprof)
			return;
		static PowerSetActiveOverlayScheme_t s_setOverlay = reinterpret_cast<PowerSetActiveOverlayScheme_t>(reinterpret_cast<void*>(GetProcAddress(s_powrprof, "PowerSetActiveOverlayScheme")));
		static PowerGetActualOverlayScheme_t s_getOverlay = reinterpret_cast<PowerGetActualOverlayScheme_t>(reinterpret_cast<void*>(GetProcAddress(s_powrprof, "PowerGetActualOverlayScheme")));
		if (!s_setOverlay || !s_getOverlay)
			return;
		static const GUID kBestPerformance = { 0xded574b5, 0x45a0, 0x4f42, { 0x87, 0x37, 0x46, 0x34, 0x5c, 0x09, 0xc2, 0x38 } };
		static bool s_changed = false;
		static GUID s_previous = {};
		static bool s_exitHandlerRegistered = false;
		if (enabled && !s_changed)
		{
			GUID current = {};
			if (s_getOverlay(&current) != ERROR_SUCCESS)
				return;
			if (IsEqualGUID(current, kBestPerformance))
				return; // already in best performance mode, nothing to change or restore
			s_previous = current;
			if (s_setOverlay(kBestPerformance) == ERROR_SUCCESS)
			{
				s_changed = true;
				if (!s_exitHandlerRegistered)
				{
					s_exitHandlerRegistered = true;
					std::atexit([]() { SetBestPerformancePowerMode(false); });
				}
			}
		}
		else if (!enabled && s_changed)
		{
			s_setOverlay(s_previous);
			s_changed = false;
		}
	}

	// Request a 1ms system timer resolution (better sleep/wait precision -> steadier frame pacing).
	// Loaded dynamically so no extra import library is required.
	inline void SetHighResTimer(bool enabled)
	{
		typedef LONG(NTAPI* NtSetTimerResolution_t)(ULONG, BOOLEAN, PULONG);
		static NtSetTimerResolution_t s_fn = []() -> NtSetTimerResolution_t {
			HMODULE ntdll = GetModuleHandleW(L"ntdll.dll");
			if (!ntdll)
				return nullptr;
			return reinterpret_cast<NtSetTimerResolution_t>(reinterpret_cast<void*>(GetProcAddress(ntdll, "NtSetTimerResolution")));
		}();
		static bool s_active = false;
		if (!s_fn || s_active == enabled)
			return;
		ULONG currentRes = 0;
		if (s_fn(10000, enabled ? TRUE : FALSE, &currentRes) >= 0) // 10000 * 100ns = 1ms
			s_active = enabled;
	}
}

#else

namespace LowEndTweaks
{
	inline void SetPowerThrottlingDisabled(bool) {}
	inline void SetHighResTimer(bool) {}
	inline void SetProcessPriorityLevel(sint32) {}
	inline void SetBestPerformancePowerMode(bool) {}
}

#endif

// applies all Low-End host tweaks according to the current config
namespace LowEndTweaks
{
	inline void ApplyFromConfig()
	{
		SetPowerThrottlingDisabled(GetConfig().lowend.disable_power_throttling.GetValue());
		SetHighResTimer(GetConfig().lowend.high_res_timer.GetValue());
		SetProcessPriorityLevel(GetConfig().lowend.process_priority.GetValue());
		SetBestPerformancePowerMode(GetConfig().lowend.best_power_mode.GetValue());
	}
}
