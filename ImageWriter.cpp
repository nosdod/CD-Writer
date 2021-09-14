#include "common.h"

HRESULT GetIsoStreamForDataWriting(__out IStream** result, __out ULONG* sectors2, WCHAR* shortStreamFilename)
{
    HRESULT hr = S_OK;
    *result = NULL;
    *sectors2 = 0;

    IStream* data = NULL;
    ULONG tmpSectors = 0;

    {
        STATSTG stat; RtlZeroMemory(&stat, sizeof(STATSTG));
        // open an ISO image for the stream
        if (SUCCEEDED(hr))
        {
            hr = SHCreateStreamOnFileW(shortStreamFilename,
                STGM_READ | STGM_SHARE_DENY_WRITE,
                &data
            );
            if (FAILED(hr))
            {
                printf("Failed to open file %S\n",
                    shortStreamFilename);
                PrintHR(hr);
            }
        }
        // validate size and save # of blocks for use later
        if (SUCCEEDED(hr))
        {
            hr = data->Stat(&stat, STATFLAG_NONAME);
            if (FAILED(hr))
            {
                printf("Failed to get stats for file\n");
                PrintHR(hr);
            }
        }
        // validate size and save # of blocks for use later
        if (SUCCEEDED(hr))
        {
            if (stat.cbSize.QuadPart % 2048 != 0)
            {
                printf("File is not multiple of 2048 bytes.  File size is %I64d (%I64x)\n",
                    stat.cbSize.QuadPart, stat.cbSize.QuadPart);
                hr = E_FAIL;
            }
            else if (stat.cbSize.QuadPart / 2048 > 0x7FFFFFFF)
            {
                printf("File is too large, # of sectors won't fit a LONG.  File size is %I64d (%I64x)\n",
                    stat.cbSize.QuadPart, stat.cbSize.QuadPart);
                hr = E_FAIL;
            }
            else
            {
                tmpSectors = (ULONG)(stat.cbSize.QuadPart / 2048);
            }
        }
    }

    if (SUCCEEDED(hr))
    {
        *result = data;
        *sectors2 = tmpSectors;
    }
    else
    {
        ReleaseAndNull(data);
    }
    return hr;
}

HRESULT ImageWriter(PROGRAM_OPTIONS options)
{
    HRESULT hr = S_OK;
    IDiscRecorder2* recorder = NULL;
    IDiscFormat2Data* dataWriter = NULL;
    IStream* dataStream = NULL;
    ULONG sectorsInDataStream = 0;
    BOOLEAN dualLayerDvdMedia = FALSE;
    ULONG index = options.WriterIndex;
    CComObject<CTestDataWriter2Event>* eventSink = NULL;

    // create a DiscFormat2Data object
    if (SUCCEEDED(hr))
    {
        hr = CoCreateInstance(CLSID_MsftDiscFormat2Data,
            NULL, CLSCTX_ALL,
            IID_PPV_ARGS(&dataWriter)
        );
        if (FAILED(hr))
        {
            printf("CoCreateInstance failed\n");
            PrintHR(hr);
        }
    }
    // create a DiscRecorder object
    if (SUCCEEDED(hr))
    {
        hr = GetDiscRecorder(index, &recorder);
        if (FAILED(hr))
        {
            printf("CoCreateInstance failed\n");
            PrintHR(hr);
        }
    }

    // Set the recorder as the recorder for the data writer
    if (SUCCEEDED(hr))
    {
        hr = dataWriter->put_Recorder(recorder);
        if (FAILED(hr))
        {
            printf("Failed put_Recorder()\n");
            PrintHR(hr);
        }
    }
    // release the recorder early...
    ReleaseAndNull(recorder);

    // Set the app name for use with exclusive access
    // THIS IS REQUIRED
    if (SUCCEEDED(hr))
    {
        BSTR appName = ::SysAllocString(L"Imapi2Sample");

        hr = dataWriter->put_ClientName(appName);
        if (FAILED(hr))
        {
            printf("FAILED to set client name for ISO write!\n");
            PrintHR(hr);
        }
        FreeSysStringAndNull(appName);
    }

    // verify the Current media write speed property gets
    //if (SUCCEEDED(hr))
    //{
    //    LONG value = 0;
    //    hr = dataWriter->get_CurrentMediaWriteSpeed(&value);
    //    if (FAILED(hr))
    //    {
    //        DoTrace((TRACE_LEVEL_INFORMATION, DebugGeneral,
    //                 "Failed get_CurrentMediaWriteSpeed() %!HRESULT!",
    //                 hr
    //                 ));
    //       printf("Ignoring failed get_CurrentWriteSpeed() for now\n");
    //        hr = S_OK;
    //    }
    //}


    // verify the SupportedMediaTypes property gets
    if (SUCCEEDED(hr))
    {
        SAFEARRAY* value = NULL;
        hr = dataWriter->get_SupportedMediaTypes(&value);
        if (FAILED(hr))
        {
            printf("Failed get_SupportedMediaTypes()\n");
            PrintHR(hr);
        }
        SafeArrayDestroyDataAndNull(value);
    }

    // get a stream to write to the disc
    if (SUCCEEDED(hr))
    {
        hr = GetIsoStreamForDataWriting(&dataStream, &sectorsInDataStream, options.FileName);
        if (FAILED(hr))
        {
            printf("Failed to create data stream for writing\n");
            PrintHR(hr);
        }
    }

    // Create the event sink
    if (SUCCEEDED(hr))
    {
        hr = CComObject<CTestDataWriter2Event>::CreateInstance(&eventSink);
        if (FAILED(hr))
        {
            printf("Failed to create event sink\n");
            PrintHR(hr);
        }
    }

    // need to keep a reference to this eventSink
    if (SUCCEEDED(hr))
    {
        hr = eventSink->AddRef();
        if (FAILED(hr))
        {
            printf("FAILED to addref\n");
            PrintHR(hr);
        }
    }

    // Hookup the event sink
    if (SUCCEEDED(hr))
    {
        hr = eventSink->DispEventAdvise(dataWriter);
        if (FAILED(hr))
        {
            printf("Failed to hookup event sink\n");
            PrintHR(hr);
        }
    }

    // write the stream
    if (SUCCEEDED(hr))
    {
        hr = dataWriter->Write(dataStream);
        if (FAILED(hr))
        {
            printf("Failed to write stream\n");
            PrintHR(hr);
        }
    }

    // unhook events
        // unhook events
    if (NULL != eventSink)
    {
        eventSink->DispEventUnadvise(dataWriter);
    }

    // verify that clearing the disc recorder works
    if (SUCCEEDED(hr))
    {
        hr = dataWriter->put_Recorder(NULL);
        if (FAILED(hr))
        {
            printf("Failed put_Recorder(NULL)\n");
            PrintHR(hr);
        }
    }

    ReleaseAndNull(eventSink);
    ReleaseAndNull(dataWriter);
    ReleaseAndNull(dataStream);

    if (SUCCEEDED(hr))
    {
        printf("ImageWriter succeeded for drive index %d\n",
            index
        );
    }
    else
    {
        printf("ImageWriter FAILED for drive index %d\n", index);
        PrintHR(hr);
    }
    return hr;
}