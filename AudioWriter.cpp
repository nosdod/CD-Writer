#include "common.h"

// Write wav files to CD
HRESULT AudioWriter(PROGRAM_OPTIONS options)
{
    HRESULT hr = S_OK;
    IDiscRecorder2* recorder = NULL;
    IDiscFormat2TrackAtOnce* audioWriter = NULL;
    CComObject<CAudioEvent>* eventSink = NULL;

    BOOLEAN bReturn = TRUE;
    ULONG index = options.WriterIndex;

    BSTR dir = ::SysAllocString(options.FileName);

    // create a IDiscFormat2TrackAtOnce object
    if (SUCCEEDED(hr))
    {
        hr = CoCreateInstance(CLSID_MsftDiscFormat2TrackAtOnce,
            NULL, CLSCTX_ALL,
            IID_PPV_ARGS(&audioWriter)
        );
        if (FAILED(hr))
        {
            printf("Failed CoCreateInstance on dataWriter\n");
            PrintHR(hr);
        }
    }

    // create a DiscRecorder object
    if (SUCCEEDED(hr))
    {
        hr = GetDiscRecorder(index, &recorder);
        if (FAILED(hr))
        {
            printf("Failed GetDiscRecorder\n");
            PrintHR(hr);
        }
    }

    // Set the recorder as the recorder for the audio writer
    if (SUCCEEDED(hr))
    {
        hr = audioWriter->put_Recorder(recorder);
        if (FAILED(hr))
        {
            printf("Failed audioWriter->put_Recorder\n");
            PrintHR(hr);
        }
    }

    // Set the app name for use with exclusive access
    // THIS IS REQUIRED
    if (SUCCEEDED(hr))
    {
        BSTR appName = ::SysAllocString(L"Imapi2Sample");

        hr = audioWriter->put_ClientName(appName);
        if (FAILED(hr))
        {
            printf("FAILED to set client name for Audio!\n");
            PrintHR(hr);
        }
        FreeSysStringAndNull(appName);
    }

    // get the current media in the recorder
    if (SUCCEEDED(hr))
    {
        IMAPI_MEDIA_PHYSICAL_TYPE mediaType = IMAPI_MEDIA_TYPE_UNKNOWN;
        hr = audioWriter->get_CurrentPhysicalMediaType(&mediaType);
        if (FAILED(hr))
        {
            printf("FAILED audioWriter->get_CurrentPhysicalMediaType\n");
            PrintHR(hr);
        }
    }

    if (SUCCEEDED(hr))
    {
        VARIANT_BOOL b;
        hr = audioWriter->IsCurrentMediaSupported(recorder, &b);
        if (FAILED(hr))
        {
            hr = S_OK;
        }
    }

    // NOW PREPARE THE MEDIA, it will be ready to use
    if (SUCCEEDED(hr))
    {
        hr = audioWriter->PrepareMedia();
        if (FAILED(hr))
        {
            printf("Failed audioWriter->PrepareMedia()\n");
            PrintHR(hr);
        }
    }

    //if (SUCCEEDED(hr) && !options.Close)
    //{
    //    hr = audioWriter->put_DoNotFinalizeDisc(VARIANT_FALSE);
    //    if (FAILED(hr))
    //    {
    //        printf("FAILED audioWriter->get_NumberOfExistingTracks\n");
    //        PrintHR(hr);
    //    }
    //}

    // hookup events
    // create an event object for the write engine
    if (SUCCEEDED(hr))
    {
        hr = CComObject<CAudioEvent>::CreateInstance(&eventSink);
        if (FAILED(hr))
        {
            printf("Failed to create event sink\n");
            PrintHR(hr);
        }
        else
        {
            eventSink->AddRef();
        }
    }

    // hookup the event
    if (SUCCEEDED(hr))
    {
        hr = eventSink->DispEventAdvise(audioWriter);
        if (FAILED(hr))
        {
            printf("Failed to hookup event sink\n");
            PrintHR(hr);
        }
    }

    // Add a track
    if (SUCCEEDED(hr))
    {
        WCHAR             AppendPath[MAX_PATH];
        WCHAR             FullPath[MAX_PATH];
        DWORD             ReturnCode;
        //	    DWORD             FileAttributes;
        HANDLE            Files;
        WIN32_FIND_DATAW  FileData;
        memset(&FileData, 0, sizeof(WIN32_FIND_DATA));


        StringCchPrintfW(AppendPath, (sizeof(AppendPath)) / (sizeof(AppendPath[0])), (L"%s\\*"), options.FileName);
        Files = FindFirstFileW(AppendPath, &FileData);

        if (INVALID_HANDLE_VALUE != Files) {
            //We have the search handle for the first file.
            ReturnCode = 1;
            while (ReturnCode) {
                if (FileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                {
                    //This is a real directory that we should deal with.
                    printf("Skipping directory %ws\n", FileData.cFileName);
                }
                else
                {
                    // We have a file, let's add it
                    IStream* testStream = NULL;
                    STATSTG stat;

                    StringCchPrintfW(FullPath, (sizeof(FullPath)) / (sizeof(FullPath[0])),
                        (L"%s\\%s"), options.FileName, FileData.cFileName);
                    printf("Attempting to add %ws\n", FullPath);

                    // get a stream to write to the disc
                    hr = SHCreateStreamOnFileW(FullPath,
                        STGM_READWRITE,
                        &testStream
                    );

                    if (FAILED(hr))
                    {
                        printf("FAILED to get file stream\n");
                        PrintHR(hr);
                    }

                    if (SUCCEEDED(hr))
                    {
                        hr = testStream->Stat(&stat, STATFLAG_DEFAULT);
                        if (FAILED(hr))
                        {
                            printf("FAILED to get testStream->Stat\n");
                            PrintHR(hr);
                        }
                    }

                    // Mod the size so that it is 2352 byte aligned
                    if (SUCCEEDED(hr))
                    {
                        ULARGE_INTEGER newSize = { 0 };

                        newSize.HighPart = stat.cbSize.HighPart;
                        newSize.LowPart = stat.cbSize.LowPart - (stat.cbSize.LowPart % 2352);

                        hr = testStream->SetSize(newSize);
                        if (FAILED(hr))
                        {
                            printf("FAILED testStream->SetSize\n");
                            PrintHR(hr);
                        }
                    }

                    if (SUCCEEDED(hr))
                    {
                        //write the stream
                        printf("adding track now...\n");
                        hr = audioWriter->AddAudioTrack(testStream);
                        if (FAILED(hr))
                        {
                            printf("FAILED audioWriter->AddAudioTrack(testStream)\n");
                            PrintHR(hr);
                        }
                        else
                        {
                            printf("\n");
                        }
                    }

                    ReleaseAndNull(testStream);

                }
                memset(&FileData, 0, sizeof(WIN32_FIND_DATA));
                ReturnCode = FindNextFileW(Files, &FileData);
            }

            if (!ReturnCode)
            {
                ReturnCode = GetLastError();
                if (ReturnCode != ERROR_NO_MORE_FILES)
                {
                    wprintf(L"Error in attempting to get the next file in %ls\n.",
                        AppendPath);
                }
                else
                {
                    printf("No More Files\n");
                }
            }
            FindClose(Files);
        }
        else
        {
            ReturnCode = GetLastError();
            printf("Could not open a search handle on %ws.\n", options.FileName);
            printf("return = %d\n", ReturnCode);
        }
    }

    // unhook events
    if (NULL != eventSink)
    {
        eventSink->DispEventUnadvise(audioWriter);
    }

    // Release the media now that we are done
    if (SUCCEEDED(hr))
    {
        hr = audioWriter->ReleaseMedia();
        if (FAILED(hr))
        {
            printf("FAILED audioWriter->ReleaseMedia()\n");
            PrintHR(hr);
        }
    }

    // Let's clear the recorder also
    if (SUCCEEDED(hr))
    {
        hr = audioWriter->put_Recorder(NULL);
        if (FAILED(hr))
        {
            printf("FAILED audioWriter->put_Recorder(NULL)\n");
            PrintHR(hr);
        }
    }

    ReleaseAndNull(eventSink);
    ReleaseAndNull(audioWriter);
    ReleaseAndNull(recorder);

    if (SUCCEEDED(hr))
    {
        printf("AudioWriter succeeded for drive index %d\n",
            index
        );
    }
    else
    {
        printf("AudioWriter FAILED for drive index %d\n", index);
        PrintHR(hr);
    }
    return hr;
}

