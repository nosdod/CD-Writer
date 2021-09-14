/*++

Copyright (c) 2005  Microsoft Corporation

Module Name:

    cdwriter.cpp

Abstract:

    A user mode console app that uses the IMAPIv2 interfaces.

Environment:

    User mode only

Revision History:

    01-01-05 : Created
    14/09/2021 : Extensively re-structure to form base of CDWriter

--*/
#include "cdwriter.h"
#include <ntverp.h>

#define BOOLEAN_STRING(_x) ((_x)?"TRUE":"FALSE")

VOID
PrintHelp(
    WCHAR * selfName
    )
{
    printf("%S: %s\n"
           "Usage:\n"           
           "%S -list\n"
           "%S -write <dir> [-multi] [-close] [-drive <#>] [-boot <file>]\n"
           "%S -audio <dir> [-close] [-drive <#>]\n"
           "%S -raw <dir> [-close] [-drive <#>]\n"
           "%S -image <file>[-close] [-drive <#>] [-bufe | -bufd]\n"
           "%S -erase [-drive <#>]\n"
           "\n"
           "\tlist      -- list the available writers and their index.\n"
           "\terase     -- quick erases the chosen recorder.\n"
           "\tfullerase -- full erases the chosen recorder.\n"
           "\twrite     -- Writes a directory to the disc.\n"
           "\t   <dir>  -- Directory to be written.\n"
           "\t   [-SAO] -- Use Cue Sheet recording.\n"
           "\t   [-close] -- Close disc (not appendable).\n"
           "\t   [-drive <#>] -- choose the given recorder index.\n"
           "\t   [-iso, -udf, -joliet] -- specify the file system to write.\n"
           "\t   [-multi] -- Add an additional session to disc.\n"
           "\t   [-boot <file>] -- Create a boot disk.  File is a boot image.\n"
           "\teject     -- Eject the CD tray\n"
           "\tinject    -- Close the CD tray\n",
           selfName, VER_PRODUCTVERSION_STR,
           selfName,selfName,selfName,selfName,selfName,selfName
           );
    return;
}

BOOLEAN
ParseCommandLine(
    IN DWORD Count,
    IN WCHAR * Arguments[],
    OUT PPROGRAM_OPTIONS Options
    )
{
    BOOLEAN goodEnough = FALSE;
    // initialize with defaults
    RtlZeroMemory( Options, sizeof(PROGRAM_OPTIONS) );

    for ( DWORD i = 0; i < Count; i++ )
    {
        if ( (Arguments[i][0] == '/') || (Arguments[i][0] == '-') )
        {
            BOOLEAN validArgument = FALSE;
            Arguments[i][0] = '-'; // minimize checks below

            //
            // If the first character of the argument is a - or a / then
            // treat it as an option.
            //
            if ( _wcsnicmp(Arguments[i], (L"-write"), strlen("-write")) == 0 )
            {
                Options->Write = TRUE;
                validArgument = TRUE;
            }
            else if ( _wcsnicmp(Arguments[i], (L"-image"), strlen("-image")) == 0 )
            {
                Options->Image = TRUE;
                validArgument = TRUE;
            }
            else if ( _wcsnicmp(Arguments[i], (L"-audio"), strlen("-audio")) == 0 )
            {
                Options->Audio = TRUE;
                validArgument = TRUE;
            }
            else if ( _wcsnicmp(Arguments[i], (L"-raw"), strlen("-raw")) == 0 )
            {
                Options->Raw = TRUE;
                validArgument = TRUE;
            }
            else if ( _wcsnicmp(Arguments[i], (L"-close"), strlen("-close")) == 0 )
            {
                Options->CloseDisc = TRUE;
                validArgument = TRUE;
            }
            else if ( _wcsnicmp(Arguments[i], (L"-multi"), strlen("-multi")) == 0 )
            {
                Options->Multi = TRUE;
                validArgument = TRUE;
            }
            else if ( _wcsnicmp(Arguments[i], (L"-iso"), strlen("-iso")) == 0 )
            {
                Options->Iso = TRUE;
                validArgument = TRUE;
            }
            else if ( _wcsnicmp(Arguments[i], (L"-joliet"), strlen("-joliet")) == 0 )
            {
                Options->Joliet = TRUE;
                validArgument = TRUE;
            }
            else if ( _wcsnicmp(Arguments[i], (L"-udf"), strlen("-udf")) == 0 )
            {
                Options->UDF = TRUE;
                validArgument = TRUE;
            }
            else if ( _wcsnicmp(Arguments[i], (L"-free"), strlen("-free")) == 0 )
            {
                Options->FreeSpace = TRUE;
                validArgument = TRUE;
            }
            else if ( _wcsnicmp(Arguments[i], (L"-inject"), strlen("-inject")) == 0 )
            {
                Options->Close = TRUE;
                validArgument = TRUE;
            }
            else if ( _wcsnicmp(Arguments[i], (L"-eject"), strlen("-eject")) == 0 )
            {
                Options->Eject = TRUE;
                validArgument = TRUE;
            }
            else if ( _wcsnicmp(Arguments[i], (L"-drive"), strlen("-drive")) == 0 )
            {                
                // requires second argument
                if ( i+1 < Count )
                {
                    i++; // advance argument index
                    ULONG tmp = 666;
                    if ( (swscanf_s( Arguments[i], (L"%d"), &tmp ) == 1) &&
                         (tmp != 666) )
                    {
                        // Let's do this zero based for now
                        Options->WriterIndex = tmp;
                        validArgument = TRUE;
                    }
                }

                if ( !validArgument )
                {
                    printf("Need a second argument after drive,"
                           " which is the one-based index to the\n"
                           "writer to use in decimal format.  To"
                           "get a list of available drives and"
                           "their indexes, use \"-list\" option\n"
                           );
                }
            }
            else if ( _wcsnicmp(Arguments[i], (L"-boot"), strlen("-boot")) == 0 )
            {                
                // requires second argument
                if ( i+1 < Count )
                {
                    i++; // advance argument index
                    ULONG tmp = 666;
                    if ( Arguments[i] != NULL )
                    {
                        // Let's do this zero based for now
                        Options->BootFileName = Arguments[i];
                        validArgument = TRUE;
                    }
                }

                if ( !validArgument )
                {
                    printf("Need a second argument after boot,"
                           " which is the boot file the\n"
                           "writer will use\n"
                           );
                }
            }
            else if ( _wcsnicmp(Arguments[i], (L"-vol"), strlen("-vol")) == 0 )
            {                
                // requires second argument
                if ( i+1 < Count )
                {
                    i++; // advance argument index
                    ULONG tmp = 666;
                    if ( Arguments[i] != NULL )
                    {
                        // Let's do this zero based for now
                        Options->VolumeName = Arguments[i];
                        validArgument = TRUE;
                    }
                }

                if ( !validArgument )
                {
                    printf("Need a second argument after vol,"
                           " which is the volume name for\n"
                           "the disc\n"
                           );
                }
            }
            else if ( _wcsnicmp(Arguments[i], (L"-list"), strlen("-list")) == 0 )
            {
                Options->ListWriters = TRUE;
                validArgument = TRUE;
            }
            else if ( _wcsnicmp(Arguments[i], (L"-erase"), strlen("-erase")) == 0 )
            {
                Options->Erase = TRUE;
                Options->FullErase = FALSE;
                validArgument = TRUE;
            }
            else if ( _wcsnicmp(Arguments[i], (L"-fullerase"), strlen("-fullerase")) == 0 )
            {
                Options->Erase = TRUE;
                Options->FullErase = TRUE;
                validArgument = TRUE;
            }
            else if ( _wcsnicmp(Arguments[i], (L"-?"), strlen("-?")) == 0 )
            {
                printf("Requesting help\n");
            }
            else
            {
                printf("Unknown option -- %S\n", Arguments[i]);
            }

            if(!validArgument)
            {
                return FALSE;
            }
        }
        else if ( Options->FileName == NULL )
        {
            //
            // The first non-flag argument is the ISO to write name.
            //

            Options->FileName = Arguments[i];

        }
        else
        {

            //
            // Too many non-flag arguments provided.  This must be an error.
            //

            printf("Error: extra argument %S not expected\n", Arguments[i]);
            return FALSE;
        }
    }

    //
    // Validate the command-line arguments.
    //
    if ( Options->ListWriters )
    {
        // listing the Writers stands alone
        if ( !(Options->Write || Options->Erase ) )
        {
            goodEnough = TRUE;
        }
        else
        {
            printf("Error: Listing writers must be used alone\n");
        }
    }
    else if ( Options->Write )
    {
        // Write allows erase, but not self-test
        // Write requires at least a filename
        if ( Options->FileName != NULL )
        {
            goodEnough = TRUE;
        }
        else
        {
            printf("Error: Write requires directory\n");
        }

        // validate erase options?
    }
    else if ( Options->Image )
    {
        // Write allows erase, but not self-test
        // Write requires at least a filename
        if ( Options->FileName != NULL )
        {
            goodEnough = TRUE;
        }
        else
        {
            printf("Error: Image requires filename\n");
        }

    }
    else if ( Options->Audio )
    {
        // Write allows erase, but not self-test
        // Write requires at least a filename
        if ( Options->FileName != NULL )
        {
            goodEnough = TRUE;
        }
        else
        {
            printf("Error: Audio requires directory\n");
        }

    }
    
    else if ( Options->Raw )
    {
        // Write allows erase, but not self-test
        // Write requires at least a filename
        if ( Options->FileName != NULL )
        {
            goodEnough = TRUE;
        }
        else
        {
            printf("Error: Raw requires directory\n");
        }

    }

    else if ( Options->Erase )
    {
        // validate erase options?
        goodEnough = TRUE;
    }


    // These are not stand alone options.
    //if ( Options->CloseDisc )
    //{
    //    //printf("Option 'DiscOpen' is not yet implemented\n");
    //    goodEnough = TRUE;
    //}
    //if ( Options->Multi )
    //{
    //    goodEnough = TRUE;
    //}

    if ( Options->FreeSpace )
    {
        goodEnough = TRUE;
    }
    if ( Options->Eject )
    {
        goodEnough = TRUE;
    }
    if ( Options->Close )
    {
        goodEnough = TRUE;
    }

    if ( !goodEnough )
    {
        RtlZeroMemory( Options, sizeof(PROGRAM_OPTIONS) );
        return FALSE;
    }

    return TRUE;
}

class CConsoleModule :
    public ::ATL::CAtlExeModuleT<CConsoleModule>
{
};

CConsoleModule _AtlModule;

int __cdecl wmain(int argc, WCHAR* argv[])
{
    HRESULT coInitHr = S_OK;
    HRESULT coInitExHr = S_OK;
    HRESULT hr = S_OK;
    PROGRAM_OPTIONS options;

    SYSTEMTIME startTime;
    SYSTEMTIME endTime;
    SYSTEMTIME elapsedTime;

    if (!ParseCommandLine(argc - 1, argv + 1, &options))
    {
        PrintHelp(argv[0]);
        hr = E_INVALIDARG;
        return hr;
    }
    else
    {
        //PrintOptions(&options);
    }

    // Get start time for total time
    GetSystemTime(&startTime);

    if (CAtlBaseModule::m_bInitFailed)
    {
        printf("AtlBaseInit failed...\n");
        coInitHr = E_FAIL;
    }
    else
    {
        // printf("AtlBaseInit passed...\n");
        coInitHr = S_OK;
    }

    // HRESULT CoInitializeEx(LPVOID pvReserved, DWORD dwCoInit);
    // Need to initialise the COM library before use
    DWORD dwCoInit = COINIT_MULTITHREADED;
    coInitExHr = CoInitializeEx(NULL, dwCoInit);

    if (SUCCEEDED(coInitHr) && SUCCEEDED(coInitExHr))
    {
        if (options.ListWriters)
        {
            hr = ListAllRecorders();
        }
        if (options.Erase)
        {
            hr = EraseMedia(options.WriterIndex, options.FullErase);
        }
        if (options.Write)
        {
            hr = DataWriter(options);
        }
        if (options.Image)
        {
            hr = ImageWriter(options);
        }
        if (options.Audio)
        {
            hr = AudioWriter(options);
        }
        if (options.Raw)
        {
            hr = RawWriter(options);
        }
        if (options.Eject)
        {
            hr = EjectClose(options, FALSE);
        }
        if (options.Close)
        {
            hr = EjectClose(options, TRUE);
        }
        CoUninitialize();

        GetSystemTime(&endTime);

        CalcElapsedTime(&startTime, &endTime, &elapsedTime);
        printf(" - Total Time: %02d:%02d:%02d\n", elapsedTime.wHour,
            elapsedTime.wMinute,
            elapsedTime.wSecond);

    }
    if (SUCCEEDED(hr))
        return 0;
    else
    {
        PrintHR(hr);
        return 1;
    }
}

