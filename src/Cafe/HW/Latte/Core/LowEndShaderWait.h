#pragma once

#include <atomic>
#include <chrono>
#include <thread>

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

	// number of shader/pipeline compiles that currently run on the render thread (the game is stalled until they are done)
	inline std::atomic<uint32> syncCompilesInProgress{ 0 };

	// Background compile threads call this before they start a new job. While the render thread has to compile something
	// itself, the CPU time is better spent there, so new background jobs step aside for a short, bounded time.
	inline void WaitWhileRenderThreadCompiles()
	{
		if (syncCompilesInProgress.load(std::memory_order_relaxed) == 0)
			return;
		const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(150);
		while (syncCompilesInProgress.load(std::memory_order_relaxed) > 0 && std::chrono::steady_clock::now() < deadline)
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}

	struct ScopedSyncCompile
	{
		ScopedSyncCompile() { syncCompilesInProgress.fetch_add(1); }
		~ScopedSyncCompile() { syncCompilesInProgress.fetch_sub(1); }
		ScopedSyncCompile(const ScopedSyncCompile&) = delete;
		ScopedSyncCompile& operator=(const ScopedSyncCompile&) = delete;
	};
}

// called by the Latte thread once per frame, right after the frame was presented
void LowEndShaderWait_OnFrameEnd();
