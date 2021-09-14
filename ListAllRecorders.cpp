#include "cdwriter.h"

// using a simple array due to consecutive zero-based values in this enum
CHAR* g_MediaTypeStrings[] = {
    "IMAPI_MEDIA_TYPE_UNKNOWN",
    "IMAPI_MEDIA_TYPE_CDROM",
    "IMAPI_MEDIA_TYPE_CDR",
    "IMAPI_MEDIA_TYPE_CDRW",
    "IMAPI_MEDIA_TYPE_DVDROM",
    "IMAPI_MEDIA_TYPE_DVDRAM",
    "IMAPI_MEDIA_TYPE_DVDPLUSR",
    "IMAPI_MEDIA_TYPE_DVDPLUSRW",
    "IMAPI_MEDIA_TYPE_DVDPLUSR_DUALLAYER",
    "IMAPI_MEDIA_TYPE_DVDDASHR",
    "IMAPI_MEDIA_TYPE_DVDDASHRW",
    "IMAPI_MEDIA_TYPE_DVDDASHR_DUALLAYER",
    "IMAPI_MEDIA_TYPE_DISK",
    "IMAPI_MEDIA_TYPE_DVDPLUSRW_DUALLAYER",
    "IMAPI_MEDIA_TYPE_HDDVDROM",
    "IMAPI_MEDIA_TYPE_HDDVDR",
    "IMAPI_MEDIA_TYPE_HDDVDRAM",
    "IMAPI_MEDIA_TYPE_BDROM",
    "IMAPI_MEDIA_TYPE_BDR",
    "IMAPI_MEDIA_TYPE_BDRE",
    "IMAPI_MEDIA_TYPE_MAX"
};
//typedef enum _STREAM_DATA_SOURCE {
//    DevZeroStream = 0,
//    SmallImageStream = 1,
//    EightGigStream = 2,
//} STREAM_DATA_SOURCE, *PSTREAM_DATA_SOURCE;

HRESULT ListAllRecorders()
{
    HRESULT hr = S_OK;
    LONG          index = 0;
    IDiscMaster2* tmpDiscMaster = NULL;

    // create a disc master object
    if (SUCCEEDED(hr))
    {
        hr = CoCreateInstance(CLSID_MsftDiscMaster2,
            NULL, CLSCTX_ALL,
            IID_PPV_ARGS(&tmpDiscMaster)
        );
        if (FAILED(hr))
        {
            printf("Failed CoCreateInstance\n");
            PrintHR(hr);
        }
    }
    // Get number of recorders
    if (SUCCEEDED(hr))
    {
        hr = tmpDiscMaster->get_Count(&index);

        if (FAILED(hr))
        {
            printf("Failed to get count\n");
            PrintHR(hr);
        }
    }

    // Print each recorder's ID
    for (LONG i = 0; i < index; i++)
    {
        IDiscRecorder2* discRecorder = NULL;

        hr = GetDiscRecorder(i, &discRecorder);

        if (SUCCEEDED(hr))
        {

            BSTR discId;
            BSTR venId;

            // Get the device strings
            if (SUCCEEDED(hr)) { hr = discRecorder->get_VendorId(&venId); }
            if (SUCCEEDED(hr)) { hr = discRecorder->get_ProductId(&discId); }
            if (FAILED(hr))
            {
                printf("Failed to get ID's\n");
                PrintHR(hr);
            }

            if (SUCCEEDED(hr))
            {
                printf("Recorder %d: %ws %ws", i, venId, discId);
            }
            // Get the mount point
            if (SUCCEEDED(hr))
            {
                SAFEARRAY* mountPoints = NULL;
                hr = discRecorder->get_VolumePathNames(&mountPoints);
                if (FAILED(hr))
                {
                    printf("Unable to get mount points, failed\n");
                    PrintHR(hr);
                }
                else if (mountPoints->rgsabound[0].cElements == 0)
                {
                    printf(" (*** NO MOUNT POINTS ***)");
                }
                else
                {
                    VARIANT* tmp = (VARIANT*)(mountPoints->pvData);
                    printf(" (");
                    for (ULONG j = 0; j < mountPoints->rgsabound[0].cElements; j++)
                    {
                        printf(" %ws ", tmp[j].bstrVal);
                    }
                    printf(")");
                }
                SafeArrayDestroyDataAndNull(mountPoints);
            }
            // Get the media type
            if (SUCCEEDED(hr))
            {
                IDiscFormat2Data* dataWriter = NULL;

                // create a DiscFormat2Data object
                if (SUCCEEDED(hr))
                {
                    hr = CoCreateInstance(CLSID_MsftDiscFormat2Data,
                        NULL, CLSCTX_ALL,
                        IID_PPV_ARGS(&dataWriter)
                    );
                    if (FAILED(hr))
                    {
                        printf("Failed CoCreateInstance on dataWriter\n");
                        PrintHR(hr);
                    }
                }

                if (SUCCEEDED(hr))
                {
                    hr = dataWriter->put_Recorder(discRecorder);
                    if (FAILED(hr))
                    {
                        printf("Failed dataWriter->put_Recorder\n");
                        PrintHR(hr);
                    }
                }
                // get the current media in the recorder
                if (SUCCEEDED(hr))
                {
                    IMAPI_MEDIA_PHYSICAL_TYPE mediaType = IMAPI_MEDIA_TYPE_UNKNOWN;
                    hr = dataWriter->get_CurrentPhysicalMediaType(&mediaType);
                    if (SUCCEEDED(hr))
                    {
                        printf(" (%s)", g_MediaTypeStrings[mediaType]);
                    }
                }
                ReleaseAndNull(dataWriter);
            }

            printf("\n");
            FreeSysStringAndNull(venId);
            FreeSysStringAndNull(discId);

        }
        else
        {
            printf("Failed to get drive %d\n", i);
        }

        ReleaseAndNull(discRecorder);

    }

    return hr;
}
