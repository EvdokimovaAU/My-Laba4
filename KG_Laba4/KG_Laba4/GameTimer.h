#pragma once
#include <windows.h>

class GameTimer {
public:
    GameTimer();

    void Reset();
    void Tick();

    float DeltaTime() const;
    float TotalTime() const;

private:
    double  mSecondsPerCount = 0.0;
    double  mDeltaTime = 0.0;

    __int64 mBaseTime = 0;
    __int64 mPrevTime = 0;
    __int64 mCurrTime = 0;
};


