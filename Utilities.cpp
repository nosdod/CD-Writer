//
// util.cpp
//
// Helper functions
//

#include "common.h"

//*********************************************************************
//* FUNCTION: GetSecondsElapsed
//*          
//* PURPOSE: 
//*********************************************************************
DWORD
GetSecondsElapsed(
    SYSTEMTIME* StartTime,
    SYSTEMTIME* EndTime)
{
    FILETIME Start, End;
    unsigned __int64 Start64 = 0, End64 = 0, Elapsed64 = 0;


    //
    //--- Convert System time
    //
    SystemTimeToFileTime(StartTime, &Start);
    SystemTimeToFileTime(EndTime, &End);


    //
    //---- Convert start and end file 
    //---- time to 2  64 bit usigned integers
    //
    ((LPDWORD)(&Start64))[1] = Start.dwHighDateTime;
    ((LPDWORD)(&Start64))[0] = Start.dwLowDateTime;

    ((LPDWORD)(&End64))[1] = End.dwHighDateTime;
    ((LPDWORD)(&End64))[0] = End.dwLowDateTime;


    //
    //--- Calc elpased time
    //
    Elapsed64 = End64 - Start64;

    //
    //---- Get micro seconds elpased
    //
    Elapsed64 /= 10;

    //
    //--- Get milly seconds elpased
    //
    Elapsed64 /= 1000;

    //
    //--- Get Secconds elpased
    //
    Elapsed64 /= 1000;

    //
    //--- Return the LowDateTime of seconds elapsed
    //--- This will be good enough for ~136 years elapsed
    //
    return(((LPDWORD)(&Elapsed64))[0]);
}


//*********************************************************************
//* FUNCTION:CalcElapsedTime
//*          
//* PURPOSE: 
//*********************************************************************
#define SECONDS_IN_A_DAY     ((DWORD)(SECONDS_IN_A_HOUR*24))
#define SECONDS_IN_A_HOUR    ((DWORD)(SECONDS_IN_A_MINUTE*60))
#define SECONDS_IN_A_MINUTE  ((DWORD)(60))
void
CalcElapsedTime(
    SYSTEMTIME* StartTime,
    SYSTEMTIME* FinishTime,
    SYSTEMTIME* ElapsedTime)
{
    DWORD SecondsElapsed;

    memset(ElapsedTime, 0, sizeof(SYSTEMTIME));

    SecondsElapsed = GetSecondsElapsed(
        StartTime, FinishTime);


    if (SecondsElapsed >= SECONDS_IN_A_DAY)
    {
        ElapsedTime->wDay = (WORD)(SecondsElapsed / SECONDS_IN_A_DAY);
        SecondsElapsed -= (ElapsedTime->wDay * SECONDS_IN_A_DAY);
    }


    if (SecondsElapsed >= SECONDS_IN_A_HOUR)
    {
        ElapsedTime->wHour = (WORD)(SecondsElapsed / SECONDS_IN_A_HOUR);
        SecondsElapsed -= (ElapsedTime->wHour * SECONDS_IN_A_HOUR);
    }

    if (SecondsElapsed >= SECONDS_IN_A_MINUTE)
    {
        ElapsedTime->wMinute = (WORD)(SecondsElapsed / SECONDS_IN_A_MINUTE);
        SecondsElapsed -= (ElapsedTime->wMinute * SECONDS_IN_A_MINUTE);
    }


    ElapsedTime->wSecond = (WORD)SecondsElapsed;

}

void PrintHR(HRESULT hr)
{
    LPSTR lpMsgBuf = NULL;
    DWORD ret;

    ret = FormatMessage(
                FORMAT_MESSAGE_ALLOCATE_BUFFER | 
                FORMAT_MESSAGE_FROM_HMODULE,
                GetModuleHandle(TEXT("imapi2.dll")),
                hr,
                0, //MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                (LPTSTR) &lpMsgBuf,
                0, NULL );

    if (ret != 0)
    {
        printf("  Returned %08x: %s\n", hr, lpMsgBuf);
    }
    
    if (ret == 0)
    {
        ret = FormatMessage(
                FORMAT_MESSAGE_ALLOCATE_BUFFER | 
                FORMAT_MESSAGE_FROM_HMODULE,
                GetModuleHandle(TEXT("imapi2fs.dll")),
                hr,
                0, //MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                (LPTSTR) &lpMsgBuf,
                0, NULL );

        if (ret != 0)
        {
            printf("  Returned %08x: %s\n", hr, lpMsgBuf);
        }
    }

    if (ret == 0)
    {
        ret = FormatMessage(
                    FORMAT_MESSAGE_ALLOCATE_BUFFER | 
                    FORMAT_MESSAGE_FROM_SYSTEM,
                    NULL,
                    hr,
                    0, //MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                    (LPTSTR) &lpMsgBuf,
                    0, NULL );

        if (ret != 0)
        {
            printf("  Returned %08x: %s\n", hr, lpMsgBuf);
        }
        else
        {
            printf("  Returned %08x (no description)\n\n", hr);
        }
    }

    //if (ret == 0)
    //{
    //    DWORD dw = GetLastError(); 

    //    ret = FormatMessage(
    //        FORMAT_MESSAGE_ALLOCATE_BUFFER | 
    //        FORMAT_MESSAGE_FROM_SYSTEM,
    //        NULL,
    //        dw,
    //        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
    //        (LPTSTR) &lpMsgBuf,
    //        0, NULL );
    //    
    //    if (ret != 0)
    //    {
    //        printf("  GetLastError returned %08x: %s\n", hr, lpMsgBuf);
    //    }
    //    else
    //    {
    //        printf("  Returned %08x (no description)\n", hr);
    //    }
    //    ExitProcess(dw); 
    //}


    LocalFree(lpMsgBuf);
}