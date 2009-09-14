#include "stdafx.h"
#include "main.h"

LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);

bool initD3D(HWND hWnd, IDirect3D9 **d3d, IDirect3DDevice9 **device)
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
    IDirect3DDevice9 *device = NULL;

    if (!initD3D(hWnd, &d3d, &device))
        return FALSE;


    // SHOWING WINDOW
    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);


    // MAIN MESSAGE LOOP:
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }


    // CLEANING
    d3d->Release();
    device->Release();

    return (int) msg.wParam;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            // TODO: Add any drawing code here...
            EndPaint(hWnd, &ps);
            break;
        }
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}