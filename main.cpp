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
const DWORD indices[] = {
    0, 2, 3,    0, 3, 1, // +y
    2, 4, 5,    2, 5, 3, // -x
    4, 6, 7,    4, 7, 5, // -y
    6, 0, 1,    6, 1, 7, // +x
    1, 3, 5,    1, 5, 7, // +z
    4, 2, 0,    4, 0, 6, // -z
    0, 1,   2, 3,
    4, 5,   6, 7,
    0, 2,   1, 3,
    4, 6,   5, 7,
    1, 7,   3, 5,
    2, 4,   0, 6,
};
const unsigned vertices_count = 8;
const unsigned indices_count = 36 + 24;
const unsigned triangles_count = 12;
const unsigned lines_count = 12;

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

const unsigned side_edge_toggle = 12;

const TCHAR SHADER_FILE[] = _T("shader.vsh");

const int WINDOW_WIDTH = 600;
const int WINDOW_HEIGHT = 600;

LRESULT CALLBACK	WndProc(HWND, UINT, WPARAM, LPARAM);

// HELPERS
void OK(HRESULT result)
{
    if (result != D3D_OK)
        throw std::exception();
}

float GCF(HWND hWnd, SphericalCoords coord) // GetClassFloat
{
    static long res;
    return *(float*)(&(res = GetClassLong(hWnd, 4*coord)));
}
void SCF(HWND hWnd, SphericalCoords coord, float value) // SetClassFloat
{
    SetClassLong(hWnd, 4*coord, *(LONG*)(&value));
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
    params.EnableAutoDepthStencil = TRUE;
    params.AutoDepthStencilFormat = D3DFMT_D16;

    OK( (*d3d)->CreateDevice(D3DADAPTER_DEFAULT,
                                    D3DDEVTYPE_HAL,
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

    OK( device->SetVertexShaderConstantF(0, projMatrix * viewMatrix, 4) );
}

void Render(Device *device,
            IDirect3DVertexBuffer9 *vertex_buffer, IDirect3DIndexBuffer9 *index_buffer,
            IDirect3DVertexShader9 *vertex_shader, IDirect3DVertexDeclaration9 *vertex_declaration,
            int render_sides)
{
    OK( device->BeginScene() );

    OK( device->Clear( 0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, GRAY, 1.0f, 0 ) );
    OK( device->SetStreamSource(0, vertex_buffer, 0, sizeof(VERTEX)) );
    OK( device->SetIndices(index_buffer) );
    OK( device->SetVertexDeclaration(vertex_declaration) );
    OK( device->SetVertexShader(vertex_shader) );
    if (render_sides)
        OK( device->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, 0, vertices_count, 0, triangles_count) );
    else
        OK( device->DrawIndexedPrimitive(D3DPT_LINELIST, 0, 0, vertices_count, triangles_count*3, lines_count) );

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
        wcex.cbClsExtra		= sizeof(float)*3+sizeof(LONG); // here would be stored view coordinates
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

        SCF(hWnd, RHO, COORDS[RHO].initial);
        SCF(hWnd, THETA, COORDS[THETA].initial);
        SCF(hWnd, PHI, COORDS[PHI].initial);
        SetClassLong(hWnd, side_edge_toggle, 1);

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
                    GCF(hWnd, RHO),
                    GCF(hWnd, THETA),
                    GCF(hWnd, PHI)
                );
                Render(device, vertex_buffer, index_buffer, vertex_shader, vertex_declaration,
                    static_cast<int>(GetClassLong(hWnd, side_edge_toggle)));
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

void IncCoord(HWND hWnd, SphericalCoords coord)
{
    float var = GCF(hWnd, coord) + COORDS[coord].delta;
    SCF(hWnd, coord, min(var, COORDS[coord].max));
}

void DecCoord(HWND hWnd, SphericalCoords coord)
{
    float var = GCF(hWnd, coord) - COORDS[coord].delta;
    SCF(hWnd, coord, max(var, COORDS[coord].min));
}

void ToggleSideEdge(HWND hWnd)
{
    SetClassLong(hWnd, side_edge_toggle, !GetClassLong(hWnd, side_edge_toggle));
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
