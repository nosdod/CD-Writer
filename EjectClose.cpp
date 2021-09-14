#include "common.h"

HRESULT EjectClose(PROGRAM_OPTIONS options, BOOLEAN close)
{
    HRESULT hr = S_OK;
    IDiscRecorder2* recorder = NULL;
    ULONG index = options.WriterIndex;
    BOOL disableMCN = FALSE;

    // create a DiscRecorder object
    if (SUCCEEDED(hr))
    {
        hr = GetDiscRecorder(index, &recorder);
        if (FAILED(hr))
        {
            printf("FAILED to get disc recorder for eject/close\n");
            PrintHR(hr);
        }
    }

    // Try and prevent shell pop-ups
    if (SUCCEEDED(hr))
    {
        hr = recorder->DisableMcn();
        if (FAILED(hr))
        {
            printf("FAILED to enable MCN after Eject/Close\n");
            PrintHR(hr);
        }
        else
        {
            // Will use this flag instead of HR, in case there is a failure later on
            // we still want to enable MCN
            disableMCN = TRUE;
        }
    }

    if (SUCCEEDED(hr))
    {
        VARIANT_BOOL canLoad = VARIANT_FALSE;
        hr = recorder->get_DeviceCanLoadMedia(&canLoad);
        if (FAILED(hr))
        {
            printf("FAILED recorder->get_DeviceCanLoadMedia\n");
            PrintHR(hr);
        }
        if (canLoad && close)
        {
            hr = recorder->CloseTray();
            if (FAILED(hr))
            {
                printf("FAILED recorder->CloseTray()\n");
                PrintHR(hr);
            }
        }
        else if (canLoad)
        {
            hr = recorder->EjectMedia();
            if (FAILED(hr))
            {
                printf("FAILED recorder->EjectMedia()\n");
                PrintHR(hr);
            }
        }
    }

    // re-enable MCN
    if (disableMCN)
    {
        hr = recorder->EnableMcn();
        if (FAILED(hr))
        {
            printf("FAILED to enable MCN after Eject/Close\n");
            PrintHR(hr);
        }
    }

    ReleaseAndNull(recorder);

    if (SUCCEEDED(hr))
    {

    }
    else
    {
        printf("EjectClose FAILED for drive index %d\n",
            index
        );
        PrintHR(hr);
    }
    return hr;
}
