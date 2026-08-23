#include "pch.h"
#include "TimeManager.h"

void TimeManager::Init()
{
	::QueryPerformanceFrequency(reinterpret_cast<LARGE_INTEGER*>(&_frequency));
	::QueryPerformanceCounter(reinterpret_cast<LARGE_INTEGER*>(&_prevCount)); // CPU 클럭
}

void TimeManager::Update(uint32 maxFps)
{
	LimitFrameRate(maxFps);

	uint64 currentCount;
	::QueryPerformanceCounter(reinterpret_cast<LARGE_INTEGER*>(&currentCount));

	_deltaTime = (currentCount - _prevCount) / static_cast<float>(_frequency);
	_prevCount = currentCount;

	_frameCount++;
    _totalFrameCount++;
	_frameTime += _deltaTime;
	_gameTime += _deltaTime;

	if (_frameTime > 1.f)
	{
		// Do not display 59 when the measured value is 59.9 FPS.
		_fps = static_cast<uint32>(_frameCount / _frameTime + 0.5f);

		_frameTime = 0.f;
		_frameCount = 0;
	}
}

void TimeManager::LimitFrameRate(uint32 maxFps) const
{
	if (maxFps == 0 || _frequency == 0)
		return;

	// Round up so a frame never finishes sooner than the requested interval.
	const uint64 minFrameTicks = (_frequency + maxFps - 1) / maxFps;

	while (true)
	{
		uint64 currentCount;
		::QueryPerformanceCounter(reinterpret_cast<LARGE_INTEGER*>(&currentCount));

		const uint64 elapsedTicks = currentCount - _prevCount;
		if (elapsedTicks >= minFrameTicks)
			break;

		const uint64 remainingTicks = minFrameTicks - elapsedTicks;
		const uint64 remainingMs = remainingTicks * 1000 / _frequency;
		if (remainingMs > 1)
			::Sleep(static_cast<DWORD>(remainingMs - 1));
		else
			YieldProcessor();
	}
}
