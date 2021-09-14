#include "common.h"

// Write audio to disc using disc at once
HRESULT RawWriter(PROGRAM_OPTIONS options)
{
    HRESULT hr = S_OK;

    ::ATL::CComPtr<IRawCDImageCreator> raw;
    ::ATL::CComPtr<IStream> resultStream;
    ATL::CComPtr<IDiscRecorder2> iDiscRecorder;
    ATL::CComPtr<IDiscFormat2RawCD> iDiscFormatRaw;
    ATL::CComObject<CTestRawWriter2Event>* events = NULL;
    ULONG index = options.WriterIndex;

    // cocreate all burning classes

    // create a DiscRecorder object
    if (SUCCEEDED(hr))
    {
        hr = GetDiscRecorder(index, &iDiscRecorder);
        if (FAILED(hr))
        {
            printf("Failed GetDiscRecorder\n");
            PrintHR(hr);
        }
    }

    if (SUCCEEDED(hr))
    {
        hr = iDiscFormatRaw.CoCreateInstance(CLSID_MsftDiscFormat2RawCD);

        if (FAILED(hr))
        {
            printf("FAILED iDiscFormatRaw.CoCreateInstance\n");
            PrintHR(hr);
        }
    }

    // attach disc recorder and disc format
    if (SUCCEEDED(hr))
    {
        hr = iDiscFormatRaw->put_Recorder(iDiscRecorder);

        if (FAILED(hr))
        {
            printf("FAILED iDiscFormatRaw->put_Recorder\n");
            PrintHR(hr);
        }
    }

    // Set the app name for use with exclusive access
    // THIS IS REQUIRED
    if (SUCCEEDED(hr))
    {
        BSTR appName = ::SysAllocString(L"Imapi2Sample");

        hr = iDiscFormatRaw->put_ClientName(appName);
        if (FAILED(hr))
        {
            printf("FAILED to set client name for Raw!\n");
            PrintHR(hr);
        }
        FreeSysStringAndNull(appName);
    }

    // check if the current recorder and media support burning
    VARIANT_BOOL recorderSupported = VARIANT_FALSE;
    VARIANT_BOOL mediaSupported = VARIANT_FALSE;

    if (SUCCEEDED(hr))
    {
        hr = iDiscFormatRaw->IsRecorderSupported(iDiscRecorder, &recorderSupported);

        if (FAILED(hr))
        {
            printf("FAILED iDiscFormatRaw->IsRecorderSupported!\n");
            PrintHR(hr);
        }

        if (recorderSupported != VARIANT_TRUE)
        {
            printf("ERROR: recorder reports it doesn't support burning DAO RAW capabilities!\n");
            hr = E_FAIL;
        }
    }

    if (SUCCEEDED(hr))
    {
        hr = iDiscFormatRaw->IsCurrentMediaSupported(iDiscRecorder, &mediaSupported);

        if (FAILED(hr))
        {
            printf("FAILED iDiscFormatRaw->IsCurrentMediaSupported!\n");
            PrintHR(hr);
        }

        if (mediaSupported != VARIANT_TRUE)
        {
            printf("ERROR: recorder reports the current media doesn't support burning DAO RAW!\n");
            hr = E_FAIL;
        }
    }

    // create a raw image creator
    if (SUCCEEDED(hr))
    {
        hr = raw.CoCreateInstance(CLSID_MsftRawCDImageCreator);

        if (FAILED(hr))
        {
            printf("FAILED raw.CoCreateInstance\n");
            PrintHR(hr);
        }
    }

    // set the image type
    if (SUCCEEDED(hr))
    {
        hr = raw->put_ResultingImageType(IMAPI_FORMAT2_RAW_CD_SUBCODE_IS_RAW); //IMAPI_FORMAT2_RAW_CD_SUBCODE_PQ_ONLY);

        if (FAILED(hr))
        {
            printf("FAILED raw->put_ResultingImageType\n");
            PrintHR(hr);
        }
    }

    // Add tracks
    if (SUCCEEDED(hr))
    {
        WCHAR             AppendPath[MAX_PATH];
        WCHAR             FullPath[MAX_PATH];
        DWORD             ReturnCode;
        HANDLE            Files;
        WIN32_FIND_DATAW  FileData;
        memset(&FileData, 0, sizeof(WIN32_FIND_DATA));
        LONG index = 0;


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
                        //Add the track to the stream
                        printf("adding track now...\n");
                        hr = raw->AddTrack(IMAPI_CD_SECTOR_AUDIO, testStream, &index);
                        if (FAILED(hr))
                        {
                            printf("FAILED raw->AddTrack(testStream)\n");
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

    // create the disc image    
    if (SUCCEEDED(hr))
    {
        raw->CreateResultImage(&resultStream);

        if (FAILED(hr))
        {
            printf("FAILED raw->CreateResultImage\n");
            PrintHR(hr);
        }
    }

    // prepare media
    if (SUCCEEDED(hr))
    {
        hr = iDiscFormatRaw->PrepareMedia();

        if (FAILED(hr))
        {
            printf("FAILED iDiscFormatRaw->PrepareMedia\n");
            PrintHR(hr);
        }
    }

    // set up options on disc format
    // NOTE: this will change later to put a different mode when it's fully implemented
    if (SUCCEEDED(hr))
    {
        hr = iDiscFormatRaw->put_RequestedSectorType(IMAPI_FORMAT2_RAW_CD_SUBCODE_IS_RAW); //IMAPI_FORMAT2_RAW_CD_SUBCODE_PQ_ONLY);

        if (FAILED(hr))
        {
            printf("FAILED iDiscFormatRaw->put_RequestedSectorType\n");
            PrintHR(hr);
        }
    }

    // connect events
    //if (SUCCEEDED(hr))
    //{
    //    hr = events->DispEventAdvise(iDiscFormatRaw);

    //    if (FAILED(hr))
    //    {
    //        printf("FAILED events->DispEventAdvise\n");
    //        PrintHR(hr);
    //    }
    //}

    // burn stream
    if (SUCCEEDED(hr))
    {
        hr = iDiscFormatRaw->WriteMedia(resultStream);

        if (FAILED(hr))
        {
            printf("FAILED iDiscFormatRaw->WriteMedia\n");
            PrintHR(hr);
        }
    }

    // unadvise events
    //if (events != NULL)
    //{
    //    events->DispEventUnadvise(iDiscFormatRaw);
    //}

    // release media (even if the burn failed)
    hr = iDiscFormatRaw->ReleaseMedia();

    return hr;
}
