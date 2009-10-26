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

DWORD RandColor();

const unsigned BONES_COUNT = 2;
const D3DDECLTYPE BONES_WEIGHTS_TYPE = D3DDECLTYPE_FLOAT2;

struct Vertex
{
    D3DXVECTOR3 v;
    DWORD color;
    float weights[BONES_COUNT];

    Vertex(D3DXVECTOR3 v = D3DXVECTOR3(0, 0, 0)) : v(v)
    {
        color = RandColor();
        weights[0] = 1;
        weights[1] = 0;
    }
    void set_weight(float w)
    {
        weights[0] = w;
        weights[1] = 1 - w;
    }
    Vertex(D3DXVECTOR3 v, DWORD color) : v(v), color(color)
    {
        weights[0] = 1;
        weights[1] = 0;
    }
};

const D3DVERTEXELEMENT9 VERTEX_ELEMENT[] =
{
    {0, 0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
    {0, 12, D3DDECLTYPE_D3DCOLOR, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR, 0},
    {0, 16, BONES_WEIGHTS_TYPE, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0},
    D3DDECL_END()
};

struct RS
{
    D3DRENDERSTATETYPE state;
    DWORD value;
};
const RS RENDER_STATES[] = {
    //{ D3DRS_FILLMODE, D3DFILL_WIREFRAME },
    { D3DRS_CULLMODE, D3DCULL_NONE },
};

const int WINDOW_WIDTH = 700;
const int WINDOW_HEIGHT = 700;

const unsigned SPEED = 5;
const float ROTATING_ANGLE = D3DX_PI/8;

const float CYLINDER_HEIGHT = 4.0f;
const float CYLINDER_RADIUS = 1.0f;
const unsigned VERTICAL_GRANULARITY = 15;
const unsigned HORIZONTAL_GRANULARITY = 10;
const unsigned VERTICES_COUNT = (VERTICAL_GRANULARITY + 1) * HORIZONTAL_GRANULARITY;
const unsigned INDICES_COUNT = (HORIZONTAL_GRANULARITY + 1) * 2 * VERTICAL_GRANULARITY;

enum {
    MATRIX_REG = 0,
    BONE_MATRIX1_REG = 4,
    BONE_MATRIX2_REG = 8,
};

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
    { 3.0f,         10.0f,          0.25f,          10.0f      }, // RHO
    { D3DX_PI/8,    D3DX_PI*7/8,    D3DX_PI/24,     D3DX_PI*11/24 }, // THETA
    { -1e37f,       1e37f,          D3DX_PI/24,     D3DX_PI/2     }  // PHI
};

const unsigned TIME_VALUE_INDEX = 12;

const TCHAR SHADER_FILE[] = _T("shader.vsh");

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

// HELPERS
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
DWORD RandColor()
{
    return D3DCOLOR_XRGB(rand()%256, rand()%256, rand()%256);
}

void CreateCylinder(Vertex *vb, DWORD *ib)
{
    float delta_z = CYLINDER_HEIGHT/VERTICAL_GRANULARITY;
    float delta_phi = 2*D3DX_PI/HORIZONTAL_GRANULARITY;

    unsigned ci = 0; // current index index for adding

    for (unsigned i = 0; i < VERTICAL_GRANULARITY; ++i)
    {
        for (unsigned j = 0; j < HORIZONTAL_GRANULARITY+1; ++j)
        {
            unsigned shift = 0;
            if (j < HORIZONTAL_GRANULARITY)
                shift = i*HORIZONTAL_GRANULARITY + j;
            else
                shift = i*HORIZONTAL_GRANULARITY;

            vb[shift] =   D3DXVECTOR3(CYLINDER_RADIUS*cosf(delta_phi*j),
                                      CYLINDER_RADIUS*sinf(delta_phi*j),
                                      delta_z*i);
            vb[shift+HORIZONTAL_GRANULARITY] = D3DXVECTOR3(CYLINDER_RADIUS*cosf(delta_phi*j),
                                      CYLINDER_RADIUS*sinf(delta_phi*j),
                                      delta_z*(i+1));

            vb[shift].set_weight(1.0f-static_cast<float>(i)/VERTICAL_GRANULARITY);
            vb[shift+HORIZONTAL_GRANULARITY].set_weight(1.0f-static_cast<float>(i+1)/VERTICAL_GRANULARITY);
            
            ib[ci++] = shift;
            ib[ci++] = shift+HORIZONTAL_GRANULARITY;
        }

    }
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
             Vertex *vb, DWORD *ib)
{
    // RUN!
    CreateCylinder(vb, ib);

    unsigned v_size = VERTICES_COUNT*sizeof(Vertex);
    unsigned i_size = INDICES_COUNT*sizeof(DWORD);

    // initializing vertex and index buffers
    OK( device->CreateVertexBuffer(v_size, 0, 0, D3DPOOL_MANAGED, vertex_buffer, NULL) );
    OK( device->CreateIndexBuffer(i_size, 0, D3DFMT_INDEX32, D3DPOOL_MANAGED, index_buffer, NULL) );

    void * buffer = NULL;
    OK( (*vertex_buffer)->Lock(0, 0, &buffer, 0) );
    memcpy(buffer, vb, v_size);
    (*vertex_buffer)->Unlock();

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

void CalcMatrix(Device *device, float rho, float tetha, float phi, LONG time)
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

    OK( device->SetVertexShaderConstantF(MATRIX_REG, projMatrix * viewMatrix, WORLD_DIMENSION + 1) );

    D3DXMATRIX static_bone(
        1, 0, 0, 0,
        0, 1, 0, 0,
        0, 0, 1, 0,
        0, 0, 0, 1
    );
    OK( device->SetVertexShaderConstantF(BONE_MATRIX1_REG, static_bone, WORLD_DIMENSION + 1) );

    float alpha = sinf(D3DX_PI*static_cast<float>(SPEED*time % 200)/100)*ROTATING_ANGLE;
    D3DXMATRIX rotating_bone(
        cosf(alpha), 0, -sinf(alpha), 0,
        0, 1, 0, 0,
        sinf(alpha), 0, cosf(alpha), 0,
        0, 0, 0, 1
    );
    OK( device->SetVertexShaderConstantF(BONE_MATRIX2_REG, rotating_bone, WORLD_DIMENSION + 1) );
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
    OK( device->DrawIndexedPrimitive(D3DPT_TRIANGLESTRIP, 0, 0, VERTICES_COUNT, 0, INDICES_COUNT-2) );

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

    Vertex vb[VERTICES_COUNT];
    DWORD ib[INDICES_COUNT];

    MSG msg = {0};
    try
    {
        srand( static_cast<unsigned>(time(NULL)) );

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

        HWND hWnd = CreateWindow(window_class_name, window_title, WS_CAPTION | WS_SYSMENU,
                                         CW_USEDEFAULT, 0, WINDOW_WIDTH, WINDOW_HEIGHT, NULL, NULL, hInstance, NULL);
        if (hWnd == NULL)
            throw std::exception();

        SetClassFloat(hWnd, RHO, COORDS[RHO].initial);
        SetClassFloat(hWnd, THETA, COORDS[THETA].initial);
        SetClassFloat(hWnd, PHI, COORDS[PHI].initial);
        SetClassLong(hWnd, TIME_VALUE_INDEX, 0);

        // INITIALIZING D3D
        InitD3D(hWnd, &d3d, &device);
        InitVIB(device, &vertex_buffer, &index_buffer, vb, ib);
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
                CalcMatrix(device,
                    GetClassFloat(hWnd, RHO),
                    GetClassFloat(hWnd, THETA),
                    GetClassFloat(hWnd, PHI),
                    GetTime(hWnd)
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
