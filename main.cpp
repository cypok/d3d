#include "stdafx.h"
#include "main.h"

typedef IDirect3DDevice9 Device;

const DWORD BLACK = D3DCOLOR_XRGB( 0, 0, 0);
const DWORD BLUE = D3DCOLOR_XRGB( 0, 0, 255);
const DWORD GREEN = D3DCOLOR_XRGB( 0, 255, 0);
const DWORD CYAN = D3DCOLOR_XRGB( 0, 255, 255);
const DWORD RED = D3DCOLOR_XRGB( 255, 0, 0);
const DWORD MAGENTA = D3DCOLOR_XRGB( 255, 0, 255);
const DWORD YELLOW = D3DCOLOR_XRGB( 255, 255, 0);
const DWORD WHITE = D3DCOLOR_XRGB( 255, 255, 255);
const DWORD GRAY = D3DCOLOR_XRGB( 128, 128, 128);

struct Vertex
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

const Vertex VERTICES[] = {
    // colored cube sides
    {  1.0f,  1.0f, -1.0f, WHITE   }, // 0
    {  1.0f,  1.0f,  1.0f, CYAN    }, // 1
    { -1.0f,  1.0f, -1.0f, MAGENTA }, // 2
    { -1.0f,  1.0f,  1.0f, BLUE    }, // 3
    { -1.0f, -1.0f, -1.0f, RED     }, // 4
    { -1.0f, -1.0f,  1.0f, BLACK   }, // 5
    {  1.0f, -1.0f, -1.0f, YELLOW  }, // 6
    {  1.0f, -1.0f,  1.0f, GREEN   }, // 7
};
const DWORD INDICES[] = {
    0, 2, 3,    0, 3, 1, // +y
    2, 4, 5,    2, 5, 3, // -x
    4, 6, 7,    4, 7, 5, // -y
    6, 0, 1,    6, 1, 7, // +x
    1, 3, 5,    1, 5, 7, // +z
    4, 2, 0,    4, 0, 6, // -z
};
const unsigned VERTICES_COUNT = 8;
const unsigned INDICES_COUNT = 36;
const unsigned TRIANGLES_COUNT = 12;

unsigned WORLD_DIMENSION = 3;

enum SphericalCoords
{
    RHO = 0,
    THETA = 1,
    PHI = 2
};

struct Coord
{
    float min;
    float max;
    float delta;
    float initial;
};

const Coord COORDS[] = {
    /* MIN */       /* MAX */       /* DELTA */     /* INITIAL */
    { 3.0f,         10.0f,          0.25f,          3.0f      }, // RHO
    { D3DX_PI/8,    D3DX_PI*7/8,    D3DX_PI/24,     D3DX_PI/3 }, // THETA
    { -1e37f,       1e37f,          D3DX_PI/24,     D3DX_PI/6 }  // PHI
};

const unsigned SIDE_EDGE_TOGGLE = 12;
const LONG INITIAL_EDGE_TOGGLE = 0;

const TCHAR SHADER_FILE[] = _T("shader.vsh");

const int WINDOW_WIDTH = 700;
const int WINDOW_HEIGHT = 700;

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

// HELPERS
void OK(HRESULT result)
{
    if (result != D3D_OK)
    {
        TCHAR buffer[] = _T("DirectX error occured: 0x00000000");
        // all constants are calculated from the previous line
        #ifdef UNICODE
            swprintf(buffer+25, 8+1, L"%08x", result);
        #else
            sprintf(buffer+25, 8+1, "%08x", result);
        #endif
        MessageBox(NULL, buffer, L"ERROR", MB_ICONERROR | MB_OK);
        throw std::exception();
    }
}

float GetClassFloat(HWND hWnd, SphericalCoords coord)
{
    static long res;
    return *reinterpret_cast<float*>(&(res = GetClassLong(hWnd, sizeof(float)*coord)));
}
void SetClassFloat(HWND hWnd, SphericalCoords coord, float value)
{
    SetClassLong(hWnd, sizeof(float)*coord, *reinterpret_cast<LONG*>(&value));
}
void ReleaseInterface(IUnknown *x)
{
    if(x != NULL)
        x->Release();
}

void InitD3D(HWND hWnd, IDirect3D9 **d3d, Device **device)
{
    // initializing device

    *d3d = Direct3DCreate9(D3D_SDK_VERSION);
    if (*d3d == NULL)
        throw std::exception();

    D3DPRESENT_PARAMETERS params;
    memset(&params, 0, sizeof(params));
    params.Windowed = true;
    params.SwapEffect = D3DSWAPEFFECT_DISCARD;
    params.BackBufferFormat = D3DFMT_UNKNOWN;
    params.hDeviceWindow = hWnd;
    params.EnableAutoDepthStencil = TRUE;
    params.AutoDepthStencilFormat = D3DFMT_D16;

    OK( (*d3d)->CreateDevice(D3DADAPTER_DEFAULT,
                                    D3DDEVTYPE_REF,
                                    hWnd,
                                    D3DCREATE_SOFTWARE_VERTEXPROCESSING,
                                    &params,
                                    device) );

    //(*device)->SetRenderState( D3DRS_CULLMODE, D3DCULL_NONE );
    //(*device)->SetRenderState( D3DRS_ZENABLE, D3DZB_TRUE );
}

void InitVIB(Device *device, IDirect3DVertexBuffer9 **vertex_buffer, IDirect3DIndexBuffer9 **index_buffer)
{
    // initializing vertex and index buffers

    unsigned v_size = VERTICES_COUNT*sizeof(VERTICES[0]);
    unsigned i_size = INDICES_COUNT*sizeof(INDICES[0]);

    OK( device->CreateVertexBuffer(v_size, 0, 0, D3DPOOL_MANAGED, vertex_buffer, NULL) );
    OK( device->CreateIndexBuffer(i_size, 0, D3DFMT_INDEX32, D3DPOOL_MANAGED, index_buffer, NULL) );

    void * buffer = NULL;
    OK( (*vertex_buffer)->Lock(0, 0, &buffer, 0) );
    memcpy(buffer, VERTICES, v_size);
    (*vertex_buffer)->Unlock();

    OK( (*index_buffer)->Lock(0, 0, &buffer, 0) );
    memcpy(buffer, INDICES, i_size);
    (*index_buffer)->Unlock();
}

void InitVDeclAndShader(Device *device, IDirect3DVertexDeclaration9 **vertex_declaration, IDirect3DVertexShader9 **vertex_shader)
{
    // initializing vertex declaration and shader

    OK( device->CreateVertexDeclaration(VERTEX_ELEMENT, vertex_declaration) );

    ID3DXBuffer *code = NULL;
    OK( D3DXAssembleShaderFromFile(SHADER_FILE, NULL, NULL, D3DXSHADER_DEBUG, &code, NULL) );
    OK( device->CreateVertexShader(static_cast<DWORD*>(code->GetBufferPointer()), vertex_shader) );

    ReleaseInterface(code);
}

void CalcMatrix(Device *device, float rho, float tetha, float phi)
{   
    // View Matrix
    D3DXVECTOR3 eye(
        rho*sin(tetha)*cos(phi),
        rho*sin(tetha)*sin(phi),
        rho*cos(tetha)
    );
    D3DXVECTOR3 at(0, 0, 0);
    D3DXVECTOR3 up(0, 0, 1);

    D3DXVECTOR3 z = at - eye;
    D3DXVec3Normalize(&z, &z);
    D3DXVECTOR3 x;
    D3DXVec3Cross(&x, &up, &z);
    D3DXVec3Normalize(&x, &x);
    D3DXVECTOR3 y;
    D3DXVec3Cross(&y, &z, &x);
    
    D3DXMATRIX viewMatrix(
        x.x, x.y, x.z, -D3DXVec3Dot(&eye, &x),
        y.x, y.y, y.z, -D3DXVec3Dot(&eye, &y),
        z.x, z.y, z.z, -D3DXVec3Dot(&eye, &z),
        0,   0,   0,   1
    );

    // Projective
    float front = COORDS[RHO].min * 0.35f;
    float back = COORDS[RHO].max * 2.0f;
    float a = back / (back - front);
    float b = - front * a;

    D3DXMATRIX projMatrix(
        front,  0,      0, 0,
        0,      front,  0, 0,
        0,      0,      a, b,
        0,      0,      1, 0
    );

    OK( device->SetVertexShaderConstantF(0, projMatrix * viewMatrix, WORLD_DIMENSION+1) );
}

void Render(Device *device,
            IDirect3DVertexBuffer9 *vertex_buffer, IDirect3DIndexBuffer9 *index_buffer,
            IDirect3DVertexShader9 *vertex_shader, IDirect3DVertexDeclaration9 *vertex_declaration,
            int render_sides)
{
    OK( device->BeginScene() );

    OK( device->Clear( 0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, GRAY, 1.0f, 0 ) );
    OK( device->SetStreamSource(0, vertex_buffer, 0, sizeof(Vertex)) );
    OK( device->SetIndices(index_buffer) );
    OK( device->SetVertexDeclaration(vertex_declaration) );
    OK( device->SetVertexShader(vertex_shader) );
    OK( device->SetRenderState(D3DRS_FILLMODE, render_sides ? D3DFILL_SOLID : D3DFILL_WIREFRAME) );
    OK( device->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, 0, VERTICES_COUNT, 0, TRIANGLES_COUNT) );

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
        wcex.cbSize         = sizeof(WNDCLASSEX);
        wcex.style          = CS_HREDRAW | CS_VREDRAW;
        wcex.lpfnWndProc    = WndProc;
        wcex.cbClsExtra     = sizeof(float)*WORLD_DIMENSION+sizeof(LONG); // here would be stored view coordinates
        wcex.cbWndExtra     = 0;
        wcex.hInstance      = hInstance;
        wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_MAIN));
        wcex.hCursor        = LoadCursor(NULL, IDC_ARROW);
        wcex.hbrBackground  = reinterpret_cast<HBRUSH>(COLOR_WINDOW+1);
        wcex.lpszMenuName   = 0;
        wcex.lpszClassName  = window_class_name;
        wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));
        RegisterClassEx(&wcex);

        HWND hWnd = CreateWindow(window_class_name, window_title, WS_CAPTION | WS_SYSMENU,
                                         CW_USEDEFAULT, 0, WINDOW_WIDTH, WINDOW_HEIGHT, NULL, NULL, hInstance, NULL);
        if (hWnd == NULL)
            throw std::exception();

        SetClassFloat(hWnd, RHO, COORDS[RHO].initial);
        SetClassFloat(hWnd, THETA, COORDS[THETA].initial);
        SetClassFloat(hWnd, PHI, COORDS[PHI].initial);
        SetClassLong(hWnd, SIDE_EDGE_TOGGLE, INITIAL_EDGE_TOGGLE);

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
                CalcMatrix(device,
                    GetClassFloat(hWnd, RHO),
                    GetClassFloat(hWnd, THETA),
                    GetClassFloat(hWnd, PHI)
                );
                Render(device, vertex_buffer, index_buffer, vertex_shader, vertex_declaration,
                    static_cast<int>(GetClassLong(hWnd, SIDE_EDGE_TOGGLE)));
            }
        }
    }
    catch(...)
    {
    }

    // CLEANING
    ReleaseInterface(d3d);
    ReleaseInterface(device);
    ReleaseInterface(vertex_buffer);
    ReleaseInterface(index_buffer);
    ReleaseInterface(vertex_shader);
    ReleaseInterface(vertex_declaration);

    return (int) msg.wParam;
}

void IncCoord(HWND hWnd, SphericalCoords coord)
{
    float var = GetClassFloat(hWnd, coord) + COORDS[coord].delta;
    SetClassFloat(hWnd, coord, min(var, COORDS[coord].max));
}

void DecCoord(HWND hWnd, SphericalCoords coord)
{
    float var = GetClassFloat(hWnd, coord) - COORDS[coord].delta;
    SetClassFloat(hWnd, coord, max(var, COORDS[coord].min));
}

void ToggleSideEdge(HWND hWnd)
{
    SetClassLong(hWnd, SIDE_EDGE_TOGGLE, !GetClassLong(hWnd, SIDE_EDGE_TOGGLE));
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_KEYDOWN:
        switch(wParam)
        {
        case VK_PRIOR:  DecCoord(hWnd, RHO);   break;
        case VK_NEXT:   IncCoord(hWnd, RHO);   break;
        case VK_UP:     DecCoord(hWnd, THETA); break;
        case VK_DOWN:   IncCoord(hWnd, THETA); break;
        case VK_RIGHT:  DecCoord(hWnd, PHI);   break;
        case VK_LEFT:   IncCoord(hWnd, PHI);   break;
        case VK_SPACE:  ToggleSideEdge(hWnd);  break;
        }
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}
