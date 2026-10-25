#pragma once

#include <atomic>

// Cemu Low-End: shared counters for the "shader wait" feature.
// Header-only on purpose (inline variables) so no build-system changes are needed.
//
// When async shader compilation is enabled, draw calls whose pipeline is not compiled yet are skipped.
// If many draws are being skipped while lots of pipelines are still compiling, the game looks broken and runs badly.
// In that case the emulation is paused at a frame boundary (never in the middle of a draw call) and a waiting screen is shown
// until the compile threads have finished.
namespace LowEndShaderWait
{
	// number of pipelines queued for background compilation that are not finished yet
	inline std::atomic<uint32> pendingPipelines{ 0 };
	// number of draw calls skipped since the last frame boundary because their pipeline was not ready
	inline std::atomic<uint32> skippedDrawsThisFrame{ 0 };
}

// called by the Latte thread once per frame, right after the frame was presented
void LowEndShaderWait_OnFrameEnd();
