#include "stdafx.h"
#include "main.h"

typedef IDirect3DDevice9 Device;
#define RELEASE_IFACE(x) if(x!=NULL) x->Release();
#define COLOR_DEFINE(name, r, g, b) \
	const DWORD name = D3DCOLOR_XRGB( (r), (g), (b) );

COLOR_DEFINE( BLACK,       0,   0,   0 )
COLOR_DEFINE( BLUE,        0,   0, 255 )
COLOR_DEFINE( GREEN,       0, 255,   0 )
COLOR_DEFINE( CYAN,        0, 255, 255 )
COLOR_DEFINE( RED,       255,   0,   0 )
COLOR_DEFINE( MAGENTA,   255,   0, 255 )
COLOR_DEFINE( YELLOW,    255, 255,   0 )
COLOR_DEFINE( WHITE,     255, 255, 255 )
COLOR_DEFINE( GRAY,      128, 128, 128 )

struct VERTEX
{
    FLOAT x, y, z;
    DWORD color;
};

const D3DVERTEXELEMENT9 VERTEX_ELEMENT[] =
{
    {0, 0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
    {0, 12, D3DDECLTYPE_D3DCOLOR, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR, 0},
    D3DDECL_END()
};

const VERTEX vertices[] = {
    {  1.0f,  1.0f,   0.0f, WHITE   }, // 0
    {  1.0f,  1.0f,   2.0f, CYAN    }, // 1
    { -1.0f,  1.0f,   0.0f, MAGENTA }, // 2
    { -1.0f,  1.0f,   2.0f, BLUE    }, // 3
    { -1.0f, -1.0f,   0.0f, RED     }, // 4
    { -1.0f, -1.0f,   2.0f, BLACK   }, // 5
    {  1.0f, -1.0f,   0.0f, YELLOW  }, // 6
    {  1.0f, -1.0f,   2.0f, GREEN   }, // 7
};
const DWORD indices[] = {
    0, 2, 3,    0, 3, 1, // +y
    2, 4, 5,    2, 5, 3, // -x
    4, 6, 7,    4, 7, 5, // -y
    6, 0, 1,    6, 1, 7, // +x
    1, 3, 5,    1, 5, 7, // +z
    4, 2, 0,    4, 0, 6, // -z
};
const unsigned vertices_count = 8;
const unsigned indices_count = 36;

const wchar_t SHADER_FILE[] = _T("shader.vsh");

const int WINDOW_WIDTH = 600;
const int WINDOW_HEIGHT = 600;

LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);

void OK(HRESULT result)
{
    if (result != D3D_OK)
        throw std::exception();
}

void InitD3D(HWND hWnd, IDirect3D9 **d3d, Device **device)
{
    // initializing device

    if (NULL == (*d3d = Direct3DCreate9(D3D_SDK_VERSION)))
        throw std::exception();

    D3DPRESENT_PARAMETERS params;
    memset(&params, 0, sizeof(params));
    params.Windowed = true;
    params.SwapEffect = D3DSWAPEFFECT_DISCARD;
    params.BackBufferFormat = D3DFMT_UNKNOWN;
    params.hDeviceWindow = hWnd;

    OK( (*d3d)->CreateDevice(D3DADAPTER_DEFAULT,
                                    D3DDEVTYPE_HAL,
                                    hWnd,
                                    D3DCREATE_SOFTWARE_VERTEXPROCESSING,
                                    &params,
                                    device) );
}

void InitVIB(Device *device, IDirect3DVertexBuffer9 **vertex_buffer, IDirect3DIndexBuffer9 **index_buffer)
{
    // initializing vertex and index buffers

    unsigned v_size = vertices_count*sizeof(vertices[0]);
    unsigned i_size = indices_count*sizeof(indices[0]);

    OK( device->CreateVertexBuffer(v_size, 0, 0, D3DPOOL_MANAGED, vertex_buffer, NULL) );
    OK( device->CreateIndexBuffer(i_size, 0, D3DFMT_INDEX32, D3DPOOL_MANAGED, index_buffer, NULL) );

    void * buffer = NULL;
    OK( (*vertex_buffer)->Lock(0, 0, &buffer, 0) );
    memcpy(buffer, vertices, v_size);
    (*vertex_buffer)->Unlock();

    OK( (*index_buffer)->Lock(0, 0, &buffer, 0) );
    memcpy(buffer, indices, i_size);
    (*index_buffer)->Unlock();
}

void InitVDeclAndShader(Device *device, IDirect3DVertexDeclaration9 **vertex_declaration, IDirect3DVertexShader9 **vertex_shader)
{
    // initializing vertex declaration and shader

    OK( device->CreateVertexDeclaration(VERTEX_ELEMENT, vertex_declaration) );

    ID3DXBuffer *code = NULL;
    OK( D3DXAssembleShaderFromFile(SHADER_FILE, NULL, NULL, D3DXSHADER_DEBUG, &code, NULL) );
    OK( device->CreateVertexShader(static_cast<DWORD*>(code->GetBufferPointer()), vertex_shader) );

    RELEASE_IFACE(code);
}

void Render(Device *device,
            IDirect3DVertexBuffer9 *vertex_buffer, IDirect3DIndexBuffer9 *index_buffer,
            IDirect3DVertexShader9 *vertex_shader, IDirect3DVertexDeclaration9 *vertex_declaration)
{
    FLOAT matrix[] = {
         0.5f,  0.0f,  0.0f,  0.0f,
         0.0f,  0.5f,  0.0f,  0.0f,
         0.0f,  0.0f,  0.5f,  0.0f,
         0.0f,  0.0f,  0.0f,  1.0f,
    };
    OK( device->BeginScene() );

    OK( device->Clear( 0, NULL, D3DCLEAR_TARGET, GRAY, 1.0f, 0 ) );
    OK( device->SetStreamSource(0, vertex_buffer, 0, sizeof(VERTEX)) );
    OK( device->SetIndices(index_buffer) );
    OK( device->SetVertexDeclaration(vertex_declaration) );
    OK( device->SetVertexShader(vertex_shader) );
    OK( device->SetVertexShaderConstantF(0, matrix, 4) );
    OK( device->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, 0, vertices_count, 0, indices_count/3) );

    OK( device->EndScene() );

    OK( device->Present(NULL, NULL, NULL, NULL) );
}

int APIENTRY _tWinMain(HINSTANCE hInstance, HINSTANCE /*hPrevInstance*/, LPTSTR /*lpCmdLine*/, int nCmdShow)
{    
    IDirect3D9 *d3d = NULL;
    Device *device = NULL;
    IDirect3DVertexBuffer9 *vertex_buffer = NULL;
    IDirect3DIndexBuffer9 *index_buffer = NULL;
    IDirect3DVertexShader9 *vertex_shader = NULL;
    IDirect3DVertexDeclaration9 *vertex_declaration = NULL;

    MSG msg = {0};
    try
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

        HWND hWnd;
        if (NULL == (hWnd = CreateWindow(window_class_name, window_title, WS_OVERLAPPEDWINDOW,
                                         CW_USEDEFAULT, 0, WINDOW_WIDTH, WINDOW_HEIGHT, NULL, NULL, hInstance, NULL)))
        {
            throw std::exception();
        }


        // INITIALIZING D3D
        InitD3D(hWnd, &d3d, &device);
        InitVIB(device, &vertex_buffer, &index_buffer);
        InitVDeclAndShader(device, &vertex_declaration, &vertex_shader);

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
                Render(device, vertex_buffer, index_buffer, vertex_shader, vertex_declaration);
            }
        }
    }
    catch(...)
    {
    }

    // CLEANING
    RELEASE_IFACE(d3d);
    RELEASE_IFACE(device);
    RELEASE_IFACE(vertex_buffer);
    RELEASE_IFACE(index_buffer);
    RELEASE_IFACE(vertex_shader);
    RELEASE_IFACE(vertex_declaration);

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
