#include "cdwriter.h"

// Get a disc recorder given a disc index
HRESULT GetDiscRecorder(__in ULONG index, __out IDiscRecorder2** recorder)
{
    HRESULT hr = S_OK;
    IDiscMaster2* tmpDiscMaster = NULL;
    BSTR tmpUniqueId;
    IDiscRecorder2* tmpRecorder = NULL;

    *recorder = NULL;

    // create the disc master object
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

    // get the unique id string
    if (SUCCEEDED(hr))
    {
        hr = tmpDiscMaster->get_Item(index, &tmpUniqueId);
        if (FAILED(hr))
        {
            printf("Failed tmpDiscMaster->get_Item\n");
            PrintHR(hr);
        }
    }

    // Create a new IDiscRecorder2
    if (SUCCEEDED(hr))
    {
        hr = CoCreateInstance(CLSID_MsftDiscRecorder2,
            NULL, CLSCTX_ALL,
            IID_PPV_ARGS(&tmpRecorder)
        );
        if (FAILED(hr))
        {
            printf("Failed CoCreateInstance\n");
            PrintHR(hr);
        }
    }
    // Initialize it with the provided BSTR
    if (SUCCEEDED(hr))
    {
        hr = tmpRecorder->InitializeDiscRecorder(tmpUniqueId);
        if (FAILED(hr))
        {
            printf("Failed to init disc recorder\n");
            PrintHR(hr);
        }
    }
    // copy to caller or release recorder
    if (SUCCEEDED(hr))
    {
        *recorder = tmpRecorder;
    }
    else
    {
        ReleaseAndNull(tmpRecorder);
    }
    // all other cleanup
    ReleaseAndNull(tmpDiscMaster);
    FreeSysStringAndNull(tmpUniqueId);
    return hr;
}