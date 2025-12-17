#include "GameTimer.h"

GameTimer::GameTimer()
{
    __int64 countsPerSec = 0;
    QueryPerformanceFrequency((LARGE_INTEGER*)&countsPerSec);
    mSecondsPerCount = 1.0 / (double)countsPerSec;
}

void GameTimer::Reset()
{
    __int64 currTime = 0;
    QueryPerformanceCounter((LARGE_INTEGER*)&currTime);

    mBaseTime = currTime;
    mPrevTime = currTime;
    mCurrTime = currTime;
}

void GameTimer::Tick()
{
    QueryPerformanceCounter((LARGE_INTEGER*)&mCurrTime);

    mDeltaTime = (mCurrTime - mPrevTime) * mSecondsPerCount;
    mPrevTime = mCurrTime;

    if (mDeltaTime < 0.0)
        mDeltaTime = 0.0;
}

float GameTimer::DeltaTime() const
{
    return (float)mDeltaTime;
}

float GameTimer::TotalTime() const
{
    return (float)((mCurrTime - mBaseTime) * mSecondsPerCount);
}
