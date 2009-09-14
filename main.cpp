#include "stdafx.h"
#include "main.h"

typedef IDirect3DDevice9 Device;

#define RELEASE_IFACE(x) if(x!=NULL) x->Release();

struct VERTEX
{
    FLOAT x, y, z, w;
    DWORD color;
};
#define D3DFVF_VERTEX (D3DFVF_XYZW|D3DFVF_DIFFUSE)

LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);

bool InitD3D(HWND hWnd, IDirect3D9 **d3d, Device **device)
{
    if (NULL == (*d3d = Direct3DCreate9(D3D_SDK_VERSION)))
    {
        return false;
    }

    D3DPRESENT_PARAMETERS params;
    memset(&params, 0, sizeof(params));
    params.Windowed = true;
    params.SwapEffect = D3DSWAPEFFECT_FLIP;
    params.BackBufferFormat = D3DFMT_A8R8G8B8;
    params.BackBufferCount = 1;
    params.hDeviceWindow = hWnd;

    if (D3D_OK != (*d3d)->CreateDevice(D3DADAPTER_DEFAULT,
                                    D3DDEVTYPE_HAL,
                                    hWnd,
                                    D3DCREATE_HARDWARE_VERTEXPROCESSING,
                                    &params,
                                    device))
    {
        return false;
    }

    return true;
}

void Render(Device *device)
{
    ;
}

int APIENTRY _tWinMain(HINSTANCE hInstance, HINSTANCE /*hPrevInstance*/, LPTSTR /*lpCmdLine*/, int nCmdShow)
{
    TCHAR window_title[] = _T("D3D window");
    TCHAR window_class_name[] = _T("D3D_APP");

    // REGISTERING AND CREATING WINDOW
    WNDCLASSEX wcex;
    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style			= CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc	= WndProc;
    wcex.cbClsExtra		= 0;
    wcex.cbWndExtra		= 0;
    wcex.hInstance		= hInstance;
    wcex.hIcon			= LoadIcon(hInstance, MAKEINTRESOURCE(IDI_MAIN));
    wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground	= (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName	= 0;
    wcex.lpszClassName	= window_class_name;
    wcex.hIconSm		= LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));
    RegisterClassEx(&wcex);

    HWND hWnd = CreateWindow(window_class_name, window_title, WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, NULL, NULL, hInstance, NULL);

    if (!hWnd)
        return FALSE;


    // INITIALIZING D3D
    IDirect3D9 *d3d = NULL;
    Device *device = NULL;

    if (!InitD3D(hWnd, &d3d, &device))
        return FALSE;


    // SHOWING WINDOW
    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);


    // MAIN MESSAGE LOOP:
    MSG msg = {0};
    while (msg.message != WM_QUIT)
    {
        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        else
        {
            Render(device);
        }
    }


    // CLEANING
    RELEASE_IFACE(d3d);
    RELEASE_IFACE(device);

    return (int) msg.wParam;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}