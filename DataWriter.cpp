#include "cdwriter.h"

// Function for writing a dir to disc
HRESULT DataWriter(PROGRAM_OPTIONS options)
{
    HRESULT           hr = S_OK;
    IDiscRecorder2* discRecorder = NULL;
    IDiscFormat2Data* dataWriter = NULL;
    IStream* dataStream = NULL;
    BOOLEAN           dualLayerDvdMedia = FALSE;
    ULONG             index = options.WriterIndex;
    VARIANT_BOOL      isBlank = FALSE;

    SYSTEMTIME startTime;
    SYSTEMTIME endTime;
    SYSTEMTIME elapsedTime;

    IFileSystemImage* image = NULL;
    IFileSystemImageResult* fsiresult = NULL;
    IFsiDirectoryItem* root = NULL;
    BSTR                     dir = ::SysAllocString(options.FileName);
    IBootOptions* pBootOptions = NULL;
    IStream* bootStream = NULL;

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
            printf("FAILED CoCreateInstance\n");
            PrintHR(hr);
        }
    }
    // create a DiscRecorder object
    if (SUCCEEDED(hr))
    {
        hr = GetDiscRecorder(index, &discRecorder);
        if (FAILED(hr))
        {
            printf("FAILED GetDiscRecorder\n");
            PrintHR(hr);
        }
    }

    // Set the recorder as the recorder for the data writer
    if (SUCCEEDED(hr))
    {
        hr = dataWriter->put_Recorder(discRecorder);
        if (FAILED(hr))
        {
            printf("FAILED to put_Recorder\n");
            PrintHR(hr);
        }
    }

    // Set the app name for use with exclusive access
    // THIS IS REQUIRED
    if (SUCCEEDED(hr))
    {
        BSTR appName = ::SysAllocString(L"Imapi2Sample");

        hr = dataWriter->put_ClientName(appName);
        if (FAILED(hr))
        {
            printf("FAILED to set client name for erase!\n");
            PrintHR(hr);
        }
        FreeSysStringAndNull(appName);
    }

    // verify the SupportedMediaTypes property gets
    if (SUCCEEDED(hr))
    {
        SAFEARRAY* value = NULL;
        hr = dataWriter->get_SupportedMediaTypes(&value);
        if (FAILED(hr))
        {
            printf("FAILED to get_SupportedMediaTypes\n");
            PrintHR(hr);
        }
        SafeArrayDestroyDataAndNull(value);
    }

    // Close the disc if specified
    if (SUCCEEDED(hr) && options.CloseDisc)
    {
        printf("Disc will be closed\n");
        hr = dataWriter->put_ForceMediaToBeClosed(VARIANT_TRUE);
        if (FAILED(hr))
        {
            printf("FAILED to put_ForceMediaToBeClosed\n");
            PrintHR(hr);
        }
    }

    // verify the StartAddressOfPreviousSession property 
    // ALSO -- for DVD+R DL, if from sector zero, set to finalize media
    //if (SUCCEEDED(hr))
    //{
    //    LONG value = 0;
    //    hr = dataWriter->get_StartAddressOfPreviousSession(&value);
    //    if (FAILED(hr))
    //    {
    //        printf("FAILED to get_StartAddressOfPreviousSession\n");
    //        PrintHR(hr);
    //    }
    //    else if (value == ((ULONG)-1))
    //    {
    //        hr = dataWriter->put_ForceMediaToBeClosed(VARIANT_TRUE);
    //        if (FAILED(hr))
    //        {
    //            printf("FAILED to put_ForceMediaToBeClosed\n");
    //            PrintHR(hr);
    //        }
    //    }
    //}   
    //   
    // get a stream to write to the disc
    // create a ID_IFileSystemImage object
    if (SUCCEEDED(hr))
    {
        hr = CoCreateInstance(CLSID_MsftFileSystemImage,
            NULL, CLSCTX_ALL,
            IID_PPV_ARGS(&image)
        );
        if (FAILED(hr))
        {
            printf("Failed CoCreate for filesystem\n");
            PrintHR(hr);
        }
    }

    // Set the filesystems to use if specified
    if (SUCCEEDED(hr) && (options.Iso || options.Joliet || options.UDF))
    {
        FsiFileSystems fileSystem = FsiFileSystemNone;
        if (options.Iso)
        {
            fileSystem = (FsiFileSystems)(fileSystem | FsiFileSystemISO9660);
        }
        if (options.Joliet)
        {
            fileSystem = (FsiFileSystems)(fileSystem | FsiFileSystemJoliet);
            fileSystem = (FsiFileSystems)(fileSystem | FsiFileSystemISO9660);
        }
        if (options.UDF)
        {
            fileSystem = (FsiFileSystems)(fileSystem | FsiFileSystemUDF);
        }

        hr = image->put_FileSystemsToCreate(fileSystem);
        if (FAILED(hr))
        {
            printf("Failed to put PileSystemsToCreate\n");
            PrintHR(hr);
        }
    }

    // Get the root dir
    if (SUCCEEDED(hr))
    {
        hr = image->get_Root(&root);
        if (FAILED(hr))
        {
            printf("Failed to get root directory\n");
            PrintHR(hr);
        }
    }
    // create the BootImageOptions object
    if (SUCCEEDED(hr) && (NULL != options.BootFileName))
    {
        hr = CoCreateInstance(CLSID_BootOptions,
            NULL, CLSCTX_ALL,
            IID_PPV_ARGS(&pBootOptions)
        );
        if (FAILED(hr))
        {
            printf("FAILED cocreate bootoptions\n");
            PrintHR(hr);
        }

        if (SUCCEEDED(hr))
        {
            hr = SHCreateStreamOnFileW(options.BootFileName,
                STGM_READ | STGM_SHARE_DENY_WRITE,
                &bootStream
            );

            if (FAILED(hr))
            {
                printf("Failed SHCreateStreamOnFileW for BootImage\n");
                PrintHR(hr);
            }
        }

        if (SUCCEEDED(hr))
        {
            hr = pBootOptions->AssignBootImage(bootStream);
            if (FAILED(hr))
            {
                printf("Failed BootImage put_BootImage\n");
                PrintHR(hr);
            }
        }

        if (SUCCEEDED(hr))
        {
            hr = image->put_BootImageOptions(pBootOptions);
            if (FAILED(hr))
            {
                printf("Failed BootImage put_BootImageOptions\n");
                PrintHR(hr);
            }
        }
    }

    // Check if media is blank
    if (SUCCEEDED(hr))
    {
        hr = dataWriter->get_MediaHeuristicallyBlank(&isBlank);
        if (FAILED(hr))
        {
            printf("Failed to get_MediaHeuristicallyBlank\n");
            PrintHR(hr);
        }
    }

    if (SUCCEEDED(hr) && !options.Multi && !isBlank)
    {
        printf("*** WRITING TO NON-BLANK MEDIA WITHOUT IMPORT! ***\n");
    }

    // ImportFileSystem - Import file data from disc
    if (SUCCEEDED(hr) && options.Multi)
    {
        FsiFileSystems fileSystems;
        SAFEARRAY* multiSession = NULL;

        // Get mutlisession interface to set in image
        if (SUCCEEDED(hr))
        {
            hr = dataWriter->get_MultisessionInterfaces(&multiSession);

            if (FAILED(hr))
            {
                printf("Failed dataWriter->MultisessionInterfaces\n");
                PrintHR(hr);
            }
        }

        // Set the multisession interface in the image
        if (SUCCEEDED(hr))
        {
            hr = image->put_MultisessionInterfaces(multiSession);

            if (FAILED(hr))
            {
                printf("Failed image->put_MultisessionInterfaces\n");
                PrintHR(hr);
                if (multiSession != NULL)
                {
                    SafeArrayDestroy(multiSession);
                }
            }
            multiSession = NULL;
        }

        if (SUCCEEDED(hr))
        {
            hr = image->ImportFileSystem(&fileSystems);
            if (FAILED(hr))
            {
                if (hr == IMAPI_E_EMPTY_DISC)
                {
                    printf("Empty Disc\n");
                    hr = S_OK;
                }
                else
                {
                    printf("Failed to ImportFileSystem\n");
                    PrintHR(hr);
                }
            }
        }
    }

    // Get free media blocks
    if (SUCCEEDED(hr))
    {
        LONG freeBlocks;

        hr = dataWriter->get_FreeSectorsOnMedia(&freeBlocks);
        if (FAILED(hr))
        {
            printf("Failed to get Free Media Blocks\n");
            PrintHR(hr);
        }
        else
        {
            hr = image->put_FreeMediaBlocks(freeBlocks);
            if (FAILED(hr))
            {
                printf("Failed to put Free Media Blocks\n");
                PrintHR(hr);
            }
        }
    }

    // Add a dir to the image
    if (SUCCEEDED(hr))
    {
        printf("Adding %ws", dir);
        GetSystemTime(&startTime);       // gets current time
        hr = root->AddTree(dir, false);
        GetSystemTime(&endTime);       // gets current time
        if (FAILED(hr))
        {
            printf("\nFailed to add %ws to root dir using AddTree\n", dir);
            PrintHR(hr);
        }
        else
        {
            CalcElapsedTime(&startTime, &endTime, &elapsedTime);
            printf(" - Time: %02d:%02d:%02d\n", elapsedTime.wHour,
                elapsedTime.wMinute,
                elapsedTime.wSecond);
        }
    }

    // Free our bstr
    FreeSysStringAndNull(dir);

    // Check what file systems are being used
    if (SUCCEEDED(hr))
    {
        FsiFileSystems fileSystems;
        hr = image->get_FileSystemsToCreate(&fileSystems);
        if (FAILED(hr))
        {
            printf("Failed image->get_FileSystemsToCreate\n");
            PrintHR(hr);
        }
        else
        {
            printf("Supported file systems: ");
            if (fileSystems & FsiFileSystemISO9660)
            {
                printf("ISO9660 ");
            }
            if (fileSystems & FsiFileSystemJoliet)
            {
                printf("Joliet ");
            }
            if (fileSystems & FsiFileSystemUDF)
            {
                printf("UDF ");
            }
            printf("\n");
        }
    }

    // Get count
    if (SUCCEEDED(hr))
    {
        LONG numFiles = 0;
        LONG numDirs = 0;
        if (SUCCEEDED(hr)) { hr = image->get_FileCount(&numFiles); }
        if (SUCCEEDED(hr)) { hr = image->get_DirectoryCount(&numDirs); }
        if (FAILED(hr))
        {
            printf("Failed image->get_FileCount\n");
            PrintHR(hr);
        }
        else
        {
            printf("Number of Files: %d\n", numFiles);
            printf("Number of Directories: %d\n", numDirs);
        }
    }

    //Set the volume name
    if (SUCCEEDED(hr) && (NULL != options.VolumeName))
    {
        BSTR volName = options.VolumeName;
        hr = image->put_VolumeName(volName);
        if (FAILED(hr))
        {
            printf("Failed setting Volume Name\n");
            PrintHR(hr);
        }

        FreeSysStringAndNull(volName);
    }

    // Create the result image
    if (SUCCEEDED(hr))
    {
        hr = image->CreateResultImage(&fsiresult);
        if (FAILED(hr))
        {
            printf("Failed to get result image, returned %08x\n", hr);
        }
    }

    // Get the stream
    if (SUCCEEDED(hr))
    {
        hr = fsiresult->get_ImageStream(&dataStream);
        if (FAILED(hr))
        {
            printf("Failed to get stream, returned %08x\n", hr);
        }
        else
        {
            printf("Image ready to write\n");
        }
    }

    // Create event sink
    if (SUCCEEDED(hr))
    {
        hr = CComObject<CTestDataWriter2Event>::CreateInstance(&eventSink);
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

    // hookup event sink
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
        GetSystemTime(&startTime);
        hr = dataWriter->Write(dataStream);
        GetSystemTime(&endTime);
        if (FAILED(hr))
        {
            printf("Failed to write stream\n");
            PrintHR(hr);
        }
        else
        {
            CalcElapsedTime(&startTime, &endTime, &elapsedTime);
            printf(" - Time to write: %02d:%02d:%02d\n", elapsedTime.wHour,
                elapsedTime.wMinute,
                elapsedTime.wSecond);
        }
    }
    // unhook events
    if (NULL != eventSink)
    {
        eventSink->DispEventUnadvise(dataWriter);
    }

    // verify the WriteProtectStatus property gets
    if (SUCCEEDED(hr))
    {
        IMAPI_MEDIA_WRITE_PROTECT_STATE value = (IMAPI_MEDIA_WRITE_PROTECT_STATE)0;
        hr = dataWriter->get_WriteProtectStatus(&value);
        if (FAILED(hr))
        {
            printf("Failed get_WriteProtectStatus()\n");
            PrintHR(hr);
        }
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
    ReleaseAndNull(image);
    ReleaseAndNull(fsiresult);
    ReleaseAndNull(dataWriter);
    ReleaseAndNull(dataStream);
    ReleaseAndNull(discRecorder);
    ReleaseAndNull(pBootOptions);
    ReleaseAndNull(bootStream);

    if (SUCCEEDED(hr))
    {
        printf("DataWriter succeeded for drive index %d\n",
            index
        );
    }
    else
    {
        printf("DataWriter FAILED for drive index %d\n",
            index
        );
        PrintHR(hr);
    }
    return hr;
}

