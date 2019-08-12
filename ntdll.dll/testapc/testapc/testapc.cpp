// testapc.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#define UNICODE 1
#define _UNICODE 1

#include <windows.h>
#include <stdio.h>
#include <tchar.h>

#define _SECOND 10000000

#define	TEST_VERSION		_T("1.0.0")

#define	TEST_COUNT_MAX		1000000

#define	TIMING_INTERVAL_NS		1				// 1 ms
#define	TIMING_INTERVAL_HNS		10000			// 1 ms

// Time API of ntdll.dll
typedef NTSTATUS(CALLBACK* LPFN_NtQueryTimerResolution)(PULONG, PULONG, PULONG);
typedef NTSTATUS(CALLBACK* LPFN_NtSetTimerResolution)(ULONG, BOOLEAN, PULONG);
// Timer API of ntdll.dll
typedef struct _TIMER_BASIC_INFORMATION {
	LARGE_INTEGER           RemainingTime;
	BOOLEAN                 TimerState;
} TIMER_BASIC_INFORMATION, *PTIMER_BASIC_INFORMATION;
typedef enum _TIMER_INFORMATION_CLASS {
	TimerBasicInformation
} TIMER_INFORMATION_CLASS, *PTIMER_INFORMATION_CLASS;
typedef void(*PTIMER_APC_ROUTINE)(
	IN PVOID TimerContext,
	IN ULONG TimerLowValue,
	IN LONG TimerHighValue
	);
typedef struct _UNICODE_STRING
{
	WORD Length;
	WORD MaximumLength;
	WORD * Buffer;
} UNICODE_STRING, *PUNICODE_STRING;
typedef struct _OBJECT_ATTRIBUTES
{
	ULONG Length;
	PVOID RootDirectory;
	PUNICODE_STRING ObjectName;
	ULONG Attributes;
	PVOID SecurityDescriptor;
	PVOID SecurityQualityOfService;
} OBJECT_ATTRIBUTES, *POBJECT_ATTRIBUTES;
typedef enum _TIMER_TYPE {
	NotificationEvent,
	SynchronizationEvent
} TIMER_TYPE, *PTIMER_TYPE;

typedef NTSTATUS(CALLBACK* LPFN_NtCancelTimer)(HANDLE, PBOOLEAN);
typedef NTSTATUS(CALLBACK* LPFN_NtCreateTimer)(OUT PHANDLE, IN ACCESS_MASK, IN POBJECT_ATTRIBUTES, IN TIMER_TYPE);
typedef NTSTATUS(CALLBACK* LPFN_NtOpenTimer)(OUT PHANDLE, IN ACCESS_MASK, IN POBJECT_ATTRIBUTES);
typedef NTSTATUS(CALLBACK* LPFN_NtQueryTimer)(OUT PHANDLE, IN TIMER_INFORMATION_CLASS, OUT PVOID, IN ULONG, OUT PULONG);
typedef NTSTATUS(CALLBACK* LPFN_NtSetTimer)(OUT PHANDLE, IN PLARGE_INTEGER, IN PTIMER_APC_ROUTINE, IN PVOID, IN BOOLEAN, IN LONG, OUT PBOOLEAN);

#if 0
NTSYSAPI NTSTATUS NTAPI NtCancelTimer(
	IN HANDLE               TimerHandle,
	OUT PBOOLEAN            CurrentState OPTIONAL );
/*
	TimerHandle     HANDLE to Timer Object opened with TIMER_MODIFY_STATE access.
	CurrentState     Pointer to BOOLEAN value, that received state of timer before function call.
*/
NTSYSAPI NTSTATUS NTAPI NtCreateTimer(
	OUT PHANDLE             TimerHandle,
	IN ACCESS_MASK          DesiredAccess,
	IN POBJECT_ATTRIBUTES   ObjectAttributes OPTIONAL,
	IN TIMER_TYPE           TimerType);
/*
	TimerHandle     Result of call - HANDLE to Timer Object.
	DesiredAccess     Access mask for TimerHandle.Can be set of(from <WinNT.h>) :
		TIMER_QUERY_STATE
		TIMER_MODIFY_STATE
		TIMER_ALL_ACCESS
	ObjectAttributes     Optional name of Timer Object.
	TimerType     Can be NotificationTimer or SynchronizationTimer(enumerated type definition from <ntdef.h>).See also EVENT_TYPE.
*/
NTSYSAPI NTSTATUS NTAPI NtOpenTimer(
	OUT PHANDLE             TimerHandle,
	IN ACCESS_MASK          DesiredAccess,
	IN POBJECT_ATTRIBUTES   ObjectAttributes);
/*
	TimerHandle     Result of call - HANDLE to Timer Object.
	DesiredAccess     Access mask for TimerHandle.See NtCreateTimer for possible values.
	ObjectAttributes     Name of Timer Object.
*/
NTSYSAPI NTSTATUS NTAPI NtQueryTimer(
	IN HANDLE               TimerHandle,
	IN TIMER_INFORMATION_CLASS TimerInformationClass,
	OUT PVOID               TimerInformation,
	IN ULONG                TimerInformationLength,
	OUT PULONG              ReturnLength OPTIONAL);
/*
	TimerHandle     HANDLE to Timer Object opened with TIMER_QUERY_STATE access.
	TimerInformationClass     Information class.See TIMER_INFORMATION_CLASS for details.
	TimerInformation     User's allocated buffer for result data.
	TimerInformationLength     Length of TimerInformation buffer, in bytes.
	ReturnLength     Optional pointer to value received used / required length of TimerInformation buffer.
*/
NTSYSAPI NTSTATUS NTAPI NtSetTimer(
	IN HANDLE               TimerHandle,
	IN PLARGE_INTEGER       DueTime,
	IN PTIMER_APC_ROUTINE   TimerApcRoutine OPTIONAL,
	IN PVOID                TimerContext OPTIONAL,
	IN BOOLEAN              ResumeTimer,
	IN LONG                 Period OPTIONAL,
	OUT PBOOLEAN            PreviousState OPTIONAL);
/*
	TimerHandle     HANDLE to Timer Object opened with TIMER_MODIFY_STATE access.
	DueTime     Time when timer should be set, in 100ns units.If it is negative value, it means relative time.
	TimerApcRoutine     User's APC routine, defined as follows:
		typedef void(*PTIMER_APC_ROUTINE)(
			IN PVOID TimerContext,
			IN ULONG TimerLowValue,
			IN LONG TimerHighValue
		);
	TimerContext     Optional parameter to TimerApcRoutine.
	ResumeTimer     If set, Power Management restores system to normal mode when timer is signaled.
	Period     If zero, timer is set only once.Else will be set periodic in time intervals defined in Period value(in 100ms units).
	PreviousState     Optional pointer to value receiving state of Timer Object before NtSetTimer call.
*/

#endif

HANDLE          hTimer = INVALID_HANDLE_VALUE;

typedef struct _MYDATA {
	TCHAR *szText;
	DWORD dwValue;
} MYDATA;

LARGE_INTEGER	g_liTick_prev = { 0 };
LARGE_INTEGER	g_liTick_curr = { 0 };

LARGE_INTEGER	g_liTick_0;
LARGE_INTEGER	g_liTick[TEST_COUNT_MAX + 1];

VOID CALLBACK TimerAPCProc(
	LPVOID lpArg,               // Data value
	DWORD dwTimerLowValue,      // Timer low value
	DWORD dwTimerHighValue)    // Timer high value

{
	// Formal parameters not used in this example.
	UNREFERENCED_PARAMETER(dwTimerLowValue);
	UNREFERENCED_PARAMETER(dwTimerHighValue);

	BOOL            bSuccess;
	LARGE_INTEGER   liDueTime;

	MYDATA *pMyData = (MYDATA *)lpArg;

	LARGE_INTEGER  liTick;
	QueryPerformanceCounter(&liTick);
	g_liTick_prev = g_liTick_curr;
	g_liTick_curr = liTick;

	g_liTick[pMyData->dwValue] = liTick;

	pMyData->dwValue++;
}

int _tmain(int argc, _TCHAR* argv[])
{
	BOOL            bSuccess;
	LARGE_INTEGER   liDueTime;
	LARGE_INTEGER	liTick;
	MYDATA          MyData;
	LONG			lTimingInterval;

	int nUs, nMs;
	DWORD dwMax;
	int nPP, nTP;

	nUs = 1000;
	nMs = nUs / 1000;

	dwMax = 1000;
	nPP = 0;
	nTP = 0;

	if (1 < argc)
	{
		int tn = 0;
		// us
		argc--;
		argv++;

		tn = _wtoi(*argv);
		if (tn)
		{
			nUs = tn;
			nMs = (nUs) / 1000;
			if (0 == nMs) nMs = 1;
		}
	}
	if (1 < argc)
	{
		DWORD tdw = 0;
		// nMax
		argc--;
		argv++;

		tdw = _wtoi(*argv);
		if (tdw)
		{
			if (tdw > TEST_COUNT_MAX)
			{
				tdw = TEST_COUNT_MAX;
			}
			dwMax = tdw;
		}
	}

	if (1 < argc)
	{
		int tn = 0;
		// Process Priority
		argc--;
		argv++;

		tn = _wtoi(*argv);
		if (tn)
		{
			nPP = tn;
		}
	}

	if (1 < argc)
	{
		int tn = 0;
		// Thread Priority
		argc--;
		argv++;

		tn = _wtoi(*argv);
		if (tn)
		{
			nTP = tn;
		}
	}

	// for high-resolution timer
	LARGE_INTEGER frequency, frqv, milv, uxi, uxin;
	QueryPerformanceFrequency(&frequency);
	frqv = frequency;
	milv.QuadPart = uxin.QuadPart = 1000000;
	if (frqv.QuadPart < uxin.QuadPart)	uxin.QuadPart = frqv.QuadPart;
	for (uxi.QuadPart = 2; uxi.QuadPart <= uxin.QuadPart; uxi.QuadPart++)
	{
		if ((0 == frqv.QuadPart % uxi.QuadPart) && (0 == milv.QuadPart % uxi.QuadPart))
		{
			// mod = 0,
			frqv.QuadPart /= uxi.QuadPart;
			milv.QuadPart /= uxi.QuadPart;
			uxin.QuadPart /= uxi.QuadPart;
			uxi.QuadPart = 1;
		}
	}

	LPFN_NtQueryTimerResolution pQueryResolution = NULL;
	LPFN_NtSetTimerResolution pSetResolution = NULL;
	LPFN_NtCancelTimer pCancelTimer = NULL;
	LPFN_NtCreateTimer pCreateTimer = NULL;
	LPFN_NtOpenTimer pOpenTimer = NULL;
	LPFN_NtQueryTimer pQueryTimer = NULL;
	LPFN_NtSetTimer pSetTimer = NULL;

	NTSTATUS stResult;

	HMODULE hNtDll = ::GetModuleHandle(_T("Ntdll"));
	if (hNtDll)
	{
		pQueryResolution = (LPFN_NtQueryTimerResolution)::GetProcAddress(hNtDll, "NtQueryTimerResolution");
		pSetResolution = (LPFN_NtSetTimerResolution)::GetProcAddress(hNtDll, "NtSetTimerResolution");
		pCancelTimer = (LPFN_NtCancelTimer)::GetProcAddress(hNtDll, "NtCancelTimer");
		pCreateTimer = (LPFN_NtCreateTimer)::GetProcAddress(hNtDll, "NtCreateTimer");
		pOpenTimer = (LPFN_NtOpenTimer)::GetProcAddress(hNtDll, "NtOpenTimer");
		pQueryTimer = (LPFN_NtQueryTimer)::GetProcAddress(hNtDll, "NtQueryTimer");
		pSetTimer = (LPFN_NtSetTimer)::GetProcAddress(hNtDll, "NtSetTimer");
	}
		
	MyData.szText = TEXT("This is my data");
	MyData.dwValue = 0;

	stResult = 0;
	hTimer = CreateWaitableTimer(
		NULL,                   // Default security attributes
		FALSE,                  // Create auto-reset timer
		TEXT("MyTimer"));       // Name of waitable timer
	if (hTimer != NULL)
	{
		// Test condition
		_tprintf(TEXT("Test version is v%s\ntestapc [interval] [count] [PP] [TP]\n\tPP - 0, 1, 2\n\tTP - 0, 1, 2, 3\n")
			, TEST_VERSION
		);
		_tprintf(TEXT("Test interval: %7d [us], %7d [ms]\nTest count: %16lu\n")
			, nUs, nMs
			, dwMax
		);

#if 1
		BOOL			bRetPriority, bRetThreadPriority;
		DWORD			dwProcessPriority, dwThreadPriority;
		DWORD			dwProcessPriorityNew, dwThreadPriorityNew;

		if (1 == nPP)
		{
			dwProcessPriorityNew = HIGH_PRIORITY_CLASS;
		}
		else if (2 == nPP)
		{
			dwProcessPriorityNew = REALTIME_PRIORITY_CLASS;
		}
		else
		{
			dwProcessPriorityNew = NORMAL_PRIORITY_CLASS;
		}

		if (1 == nTP)
		{
			dwThreadPriorityNew = THREAD_PRIORITY_ABOVE_NORMAL;
		}
		else if (2 == nTP)
		{
			dwThreadPriorityNew = THREAD_PRIORITY_HIGHEST;
		}
		else if (3 == nTP)
		{
			dwThreadPriorityNew = THREAD_PRIORITY_TIME_CRITICAL;
		}
		else
		{
			dwThreadPriorityNew = THREAD_PRIORITY_NORMAL;
		}


		// Set priority
		HANDLE			hProcess;
		hProcess = GetCurrentProcess();
		dwProcessPriority = GetPriorityClass(hProcess);

		if (dwProcessPriority != dwProcessPriorityNew)
		{
			bRetPriority = SetPriorityClass(hProcess, dwProcessPriorityNew);
		}
		else
		{
			bRetPriority = true;
		}

		HANDLE			hThread;
		hThread = GetCurrentThread();
		dwThreadPriority = GetThreadPriority(hThread);

		if (dwThreadPriority != dwThreadPriorityNew)
		{
			bRetThreadPriority = SetThreadPriority(hThread, dwThreadPriorityNew);
		}
		else
		{
			bRetThreadPriority = true;
		}

		_tprintf(TEXT("Process Priority: %8X --> %8X  %s\n")
			, dwProcessPriority
			, dwProcessPriorityNew
			, (bRetPriority ? _T("OK") : _T("FAIL"))
		);

		_tprintf(TEXT("Thread  Priority: %8X --> %8X  %s\n")
			, dwThreadPriority
			, dwThreadPriorityNew
			, (bRetThreadPriority ? _T("OK") : _T("FAIL"))
		);
#endif

		_tprintf(TEXT("Frequency: %16llu, mil = %16llu, frq = %16llu\n")
			, frequency.QuadPart
			, milv.QuadPart
			, frqv.QuadPart
		);

		ULONG nMinRes, nMaxRes, nCurRes, nSetRes;
		NTSTATUS stQueryRes, stSetRes;
		if (pQueryResolution)
		{
			stQueryRes = pQueryResolution(&nMinRes, &nMaxRes, &nCurRes);
			_tprintf(TEXT("NT timer resolutions (min/max/cur): %u.%u / %u.%u / %u.%u ms %s %s\n")
				, nMinRes / 10000, (nMinRes % 10000) / 10
				, nMaxRes / 10000, (nMaxRes % 10000) / 10
				, nCurRes / 10000, (nCurRes % 10000) / 10
				, (bRetPriority ? _T("OK") : _T("FAIL"))
				, (bRetThreadPriority ? _T("OK") : _T("FAIL"))
			);
		}
		nSetRes = nMaxRes;
		if (pSetResolution && nSetRes)
		{
			NTSTATUS nStatus = pSetResolution(nSetRes, TRUE, &nCurRes);
			_tprintf(TEXT("NT timer resolutions (set val/cur):        / %u.%u / %u.%u ms\n")
				, nSetRes / 10000, (nSetRes % 10000) / 10
				, nCurRes / 10000, (nCurRes % 10000) / 10
			);
		}

		__try
		{
			// Create an integer that will be used to signal the timer 
			liDueTime.QuadPart = (__int64) -10 * (__int64) nUs;
			lTimingInterval = nMs;

			QueryPerformanceCounter(&liTick);
			g_liTick_curr = liTick;
			g_liTick_0 = liTick;

			bSuccess = SetWaitableTimer(
				hTimer,           // Handle to the timer object
				&liDueTime,       // When timer will become signaled
				lTimingInterval,             // Periodic timer interval of 2 seconds
				TimerAPCProc,     // Completion routine
				&MyData,          // Argument to the completion routine
				FALSE);          // Do not restore a suspended system

			if (bSuccess)
			{
				for (; MyData.dwValue < dwMax; )
				{
					SleepEx(
						INFINITE,     // Wait forever
						TRUE);       // Put thread in an alertable state
				}

			}
			else
			{
				printf("SetWaitableTimer failed with error %d\n", GetLastError());
			}

		}
		__finally
		{
			CloseHandle(hTimer);
		}
	}
	else
	{
		printf("CreateWaitableTimer failed with error %d\n", GetLastError());
	}

	LARGE_INTEGER liTick_prev, liTick_curr;
	liTick_curr = g_liTick_0;
	for (int i = 0; i < dwMax; i++)
	{
		liTick_prev = liTick_curr;
		liTick_curr = g_liTick[i];
		_tprintf(TEXT("%5d %16llu %16llu\n")
			, i
			, liTick_curr.QuadPart * milv.QuadPart / frqv.QuadPart
			, (liTick_curr.QuadPart - liTick_prev.QuadPart) * milv.QuadPart / frqv.QuadPart
			);

	}
	return 0;
}

