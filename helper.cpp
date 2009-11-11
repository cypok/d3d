#include "stdafx.h"
#include "helper.h"

void OK(HRESULT result)
{
    if (result != D3D_OK)
    {
        TCHAR buffer[] = _T("DirectX error occured: 0x00000000");
        // all constants are calculated from the previous line
        #ifdef UNICODE
            swprintf(buffer+sizeof(buffer)/sizeof(buffer[0])-9, 8+1, L"%08x", result);
        #else
            sprintf(buffer+sizeof(buffer)/sizeof(buffer[0])-9, 8+1, "%08x", result);
        #endif
        MessageBox(NULL, buffer, _T("ERROR"), MB_ICONERROR | MB_OK);
        throw std::exception();
    }
}

void ReleaseInterface(IUnknown *x)
{
    if(x != NULL)
        x->Release();
}