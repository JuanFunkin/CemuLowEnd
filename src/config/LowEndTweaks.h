#pragma once

#include "config/CemuConfig.h"

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
}

#endif

// applies all Low-End host tweaks according to the current config
namespace LowEndTweaks
{
	inline void ApplyFromConfig()
	{
		SetPowerThrottlingDisabled(GetConfig().lowend.disable_power_throttling.GetValue());
		SetHighResTimer(GetConfig().lowend.high_res_timer.GetValue());
	}
}
