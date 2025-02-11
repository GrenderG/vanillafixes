#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <math.h>

#include "offsets.h"

BOOL CpuHasInvariantTsc() {
    int cpuInfo[4] = {0};
    __cpuid(cpuInfo, 0x80000000);

    if(cpuInfo[0] >= 0x80000007) {
        __cpuid(cpuInfo, 0x80000007);
        return cpuInfo[3] & (1 << 8);
    }

    return FALSE;
}

inline void CpuTimeSample(PLARGE_INTEGER pQpc, PDWORD64 pTsc) {
    /* Always request a new timeslice before sampling */
    Sleep(0);

    QueryPerformanceCounter(pQpc);
    *pTsc = WowReadTsc();

	/* Wait for QPC and RDTSC to finish */
    _mm_lfence();
}

DWORD64 CpuCalibrateTsc() {
    /* Pin on current core and run with high priority */
    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST);
    SetThreadAffinityMask(GetCurrentThread(), 1 << (GetCurrentProcessorNumber()));

    LARGE_INTEGER baselineQpc;
    DWORD64 baselineTsc;
    CpuTimeSample(&baselineQpc, &baselineTsc);

    Sleep(500);

    LARGE_INTEGER diffQpc;
    DWORD64 diffTsc;
    CpuTimeSample(&diffQpc, &diffTsc);

    diffQpc.QuadPart -= baselineQpc.QuadPart;
    diffTsc -= baselineTsc;

	LARGE_INTEGER performanceFrequency;
    QueryPerformanceFrequency(&performanceFrequency);

    double elapsedTime = diffQpc.QuadPart / (double)performanceFrequency.QuadPart;
    double estimatedTscFrequency = diffTsc / elapsedTime;

	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_NORMAL);
    SetThreadAffinityMask(GetCurrentThread(), 0);

    return estimatedTscFrequency;
}