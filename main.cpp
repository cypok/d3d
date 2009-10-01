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
    D3DXVECTOR3 v;
    DWORD color;
};

const D3DVERTEXELEMENT9 VERTEX_ELEMENT[] =
{
    {0, 0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
    {0, 12, D3DDECLTYPE_D3DCOLOR, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR, 0},
    D3DDECL_END()
};

struct RS
{
    D3DRENDERSTATETYPE state;
    DWORD value;
};
const RS RENDER_STATES[] = {
    { D3DRS_FILLMODE, D3DFILL_WIREFRAME },
    { D3DRS_CULLMODE, D3DCULL_NONE },
};

const unsigned TESSELATE_LEVEL = 3; // <6
const unsigned VERTICES_COUNT = 6 + 8 * ( (1 << 2*TESSELATE_LEVEL) - 1 ); // it's math
const unsigned INDICES_COUNT = 144 * (1 << 2*(TESSELATE_LEVEL-1)); // it's too

const Vertex INITIAL_PYRAMID[] = {
    { D3DXVECTOR3(  1.0f,  1.0f,  0.0f       ),    RED     },
    { D3DXVECTOR3( -1.0f,  1.0f,  0.0f       ),    MAGENTA },
    { D3DXVECTOR3( -1.0f, -1.0f,  0.0f       ),    CYAN    },
    { D3DXVECTOR3(  1.0f, -1.0f,  0.0f       ),    GREEN   },
    { D3DXVECTOR3(  0.0f,  0.0f,  sqrtf(2.0) ),    WHITE   },
    { D3DXVECTOR3(  0.0f,  0.0f, -sqrtf(2.0) ),    BLACK   },
};
const unsigned INITIAL_PYRAMID_VCOUNT = sizeof(INITIAL_PYRAMID)/sizeof(INITIAL_PYRAMID[0]);
const float SPHERA_RADIUS = sqrtf(2.0);

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
    { D3DX_PI/8,    D3DX_PI*7/8,    D3DX_PI/24,     D3DX_PI*11/24 }, // THETA
    { -1e37f,       1e37f,          D3DX_PI/24,     0.0f      }  // PHI
};

const unsigned TIME_VALUE_INDEX = 12;

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

float GetClassFloat(HWND hWnd, SphericalCoords coord) // GetClassFloat
{
    static long res;
    return *reinterpret_cast<float*>(&(res = GetClassLong(hWnd, sizeof(float)*coord)));
}
void SetClassFloat(HWND hWnd, SphericalCoords coord, float value) // SetClassFloat
{
    SetClassLong(hWnd, sizeof(float)*coord, *reinterpret_cast<LONG*>(&value));
}
LONG GetTime(HWND hWnd)
{
    return GetClassLong(hWnd, TIME_VALUE_INDEX);
}
void IncTime(HWND hWnd)
{
    SetClassLong(hWnd, TIME_VALUE_INDEX, 1+GetClassLong(hWnd, TIME_VALUE_INDEX));
}
void ReleaseInterface(IUnknown *x)
{
    if(x != NULL)
        x->Release();
}
DWORD MixColors(D3DCOLOR c1, D3DCOLOR c2)
{
    return D3DCOLOR_XRGB(
                ((c1 & 0xff0000) + (c2 & 0xff0000)) >> 17,
                ((c1 & 0xff00)   + (c2 & 0xff00)) >> 9,
                ((c1 & 0xff)     + (c2 & 0xff)) >> 1
           );
}
DWORD RandColor()
{
    return D3DCOLOR_XRGB(rand()%256, rand()%256, rand()%256);
}

void Tesselate(unsigned i1, unsigned i2, unsigned i3,
               Vertex *vb, unsigned *cv,
               DWORD *ib, unsigned *ci,
               int level)
{
    if (level > TESSELATE_LEVEL)
        return;

    unsigned j = *cv;
    // set vertices
    vb[ j ].v = ( vb[i1].v + vb[i2].v )/2;
    vb[j+1].v = ( vb[i2].v + vb[i3].v )/2;
    vb[j+2].v = ( vb[i3].v + vb[i1].v )/2;
    vb[ j ].color = MixColors(vb[i1].color, vb[i2].color);
    vb[j+1].color = MixColors(vb[i2].color, vb[i3].color);
    vb[j+2].color = MixColors(vb[i3].color, vb[i1].color);

    // set indices
    if (level == TESSELATE_LEVEL)
    {
        // adding indices to index buffer
#define add2ib(x, y) ib[(*ci)++] = x; ib[(*ci)++] = y;
        add2ib(i1, j);
        add2ib(j, j+2);
        add2ib(j+2, i1);

        add2ib(j, i2);
        add2ib(i2, j+1);
        add2ib(j+1, j);

        add2ib(j+2, j+1);
        add2ib(j+1, i3);
        add2ib(i3, j+2);
#undef add2ib
    }

    ++level;
    *cv = j+3;
    // next level
    Tesselate(i1,  j,  j+2,  vb, cv, ib, ci, level);
    Tesselate(j,   i2,  j+1, vb, cv, ib, ci, level);
    Tesselate(j+1, j+2, j,   vb, cv, ib, ci, level);
    Tesselate(j+2, j+1, i3,  vb, cv, ib, ci, level);
}

void InitD3D(HWND hWnd, IDirect3D9 **d3d, Device **device)
{
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

    for(int i = 0; i < sizeof(RENDER_STATES)/sizeof(RENDER_STATES[0]); ++i)
        OK( (*device)->SetRenderState( RENDER_STATES[i].state,
                                       RENDER_STATES[i].value )
        );
}

void InitVIB(Device *device, IDirect3DVertexBuffer9 **vertex_buffer, IDirect3DIndexBuffer9 **index_buffer,
             Vertex *vb_initial, Vertex *vb_final, DWORD *ib)
{
    unsigned cv = 0; // current index in vb
    unsigned ci = 0; // current index in ib

    // copy initial vertices
    memcpy(vb_initial, INITIAL_PYRAMID, INITIAL_PYRAMID_VCOUNT * sizeof(Vertex));
    cv = 6;

    // RUN!
    Tesselate(0, 3, 4, vb_initial, &cv, ib, &ci, 1);
    Tesselate(3, 2, 4, vb_initial, &cv, ib, &ci, 1);
    Tesselate(2, 1, 4, vb_initial, &cv, ib, &ci, 1);
    Tesselate(1, 0, 4, vb_initial, &cv, ib, &ci, 1);
    Tesselate(0, 1, 5, vb_initial, &cv, ib, &ci, 1);
    Tesselate(1, 2, 5, vb_initial, &cv, ib, &ci, 1);
    Tesselate(2, 3, 5, vb_initial, &cv, ib, &ci, 1);
    Tesselate(3, 0, 5, vb_initial, &cv, ib, &ci, 1);

    // generate final state of vb
    unsigned v_size = VERTICES_COUNT*sizeof(Vertex);
    unsigned i_size = INDICES_COUNT*sizeof(DWORD);

    memcpy(vb_final, vb_initial, v_size);
    for(int i = 0; i < VERTICES_COUNT; ++i)
        vb_final[i].v *= (SPHERA_RADIUS / D3DXVec3Length(&vb_final[i].v));

    // initializing vertex and index buffers
    OK( device->CreateVertexBuffer(v_size, 0, 0, D3DPOOL_MANAGED, vertex_buffer, NULL) );
    OK( device->CreateIndexBuffer(i_size, 0, D3DFMT_INDEX32, D3DPOOL_MANAGED, index_buffer, NULL) );

    // We would set vertex_buffer later

    void * buffer = NULL;
    OK( (*index_buffer)->Lock(0, 0, &buffer, 0) );
    memcpy(buffer, ib, i_size);
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

void AnimateVB(IDirect3DVertexBuffer9 *vertex_buffer, Vertex *vb_initial, Vertex *vb_final, LONG time)
{
    static bool first = true;
    static Vertex vb[VERTICES_COUNT];
    unsigned v_size = VERTICES_COUNT*sizeof(Vertex);
    void * buffer = NULL;

    if (first)
    {
        // set colors
        memcpy(vb, vb_initial, v_size);
        first = false;
    }

    // time -> 0..1
    float t = (1.0f + sinf(D3DXToRadian( 4.0f*static_cast<float>(time%90) )))/2;
    for (int i = 0; i < VERTICES_COUNT; ++i)
        vb[i].v = (vb_initial[i].v*(1.0f-t) + vb_final[i].v*t)/2;

    OK( vertex_buffer->Lock(0, 0, &buffer, 0) );
    memcpy(buffer, vb, v_size);
    vertex_buffer->Unlock();
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

    OK( device->SetVertexShaderConstantF(0, projMatrix * viewMatrix, WORLD_DIMENSION + 1) );
}

void Render(Device *device,
            IDirect3DVertexBuffer9 *vertex_buffer, IDirect3DIndexBuffer9 *index_buffer,
            IDirect3DVertexShader9 *vertex_shader, IDirect3DVertexDeclaration9 *vertex_declaration)
{
    OK( device->BeginScene() );

    OK( device->Clear( 0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, GRAY, 1.0f, 0 ) );
    OK( device->SetStreamSource(0, vertex_buffer, 0, sizeof(Vertex)) );
    OK( device->SetIndices(index_buffer) );
    OK( device->SetVertexDeclaration(vertex_declaration) );
    OK( device->SetVertexShader(vertex_shader) );
    OK( device->DrawIndexedPrimitive(D3DPT_LINELIST, 0, 0, VERTICES_COUNT, 0, INDICES_COUNT/2) );

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

    Vertex vb_initial[VERTICES_COUNT];
    Vertex vb_final[VERTICES_COUNT];
    DWORD ib[INDICES_COUNT];

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
        wcex.cbClsExtra     = sizeof(float)*3+sizeof(LONG); // here would be stored view coordinates
        wcex.cbWndExtra     = 0;
        wcex.hInstance      = hInstance;
        wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_MAIN));
        wcex.hCursor        = LoadCursor(NULL, IDC_ARROW);
        wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
        wcex.lpszMenuName   = 0;
        wcex.lpszClassName  = window_class_name;
        wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));
        RegisterClassEx(&wcex);

        HWND hWnd;
        if (NULL == (hWnd = CreateWindow(window_class_name, window_title, WS_CAPTION | WS_SYSMENU,
                                         CW_USEDEFAULT, 0, WINDOW_WIDTH, WINDOW_HEIGHT, NULL, NULL, hInstance, NULL)))
        {
            throw std::exception();
        }

        SetClassFloat(hWnd, RHO, COORDS[RHO].initial);
        SetClassFloat(hWnd, THETA, COORDS[THETA].initial);
        SetClassFloat(hWnd, PHI, COORDS[PHI].initial);
        SetClassLong(hWnd, TIME_VALUE_INDEX, 0);

        // INITIALIZING D3D
        InitD3D(hWnd, &d3d, &device);
        InitVIB(device, &vertex_buffer, &index_buffer, vb_initial, vb_final, ib);
        InitVDeclAndShader(device, &vertex_declaration, &vertex_shader);

        // SHOWING WINDOW
        ShowWindow(hWnd, nCmdShow);
        UpdateWindow(hWnd);

        // CREATE TIMER FOR ANIMATION
        SetTimer(hWnd, 0, 50, NULL);

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
                AnimateVB(vertex_buffer, vb_initial, vb_final, GetTime(hWnd));
                CalcMatrix(device,
                    GetClassFloat(hWnd, RHO),
                    GetClassFloat(hWnd, THETA),
                    GetClassFloat(hWnd, PHI)
                );
                Render(device, vertex_buffer, index_buffer, vertex_shader, vertex_declaration);
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
        }
        break;

    case WM_TIMER:      IncTime(hWnd);        break;

    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}
