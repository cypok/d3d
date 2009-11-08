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
const DWORD ALL_COLORS[] = {
    BLACK, BLUE, GREEN, CYAN,
    RED, MAGENTA, YELLOW, WHITE,
};

struct Vertex
{
    D3DXVECTOR3 v;
    D3DXVECTOR3 norm;
    DWORD color;

    Vertex( D3DXVECTOR3 v = D3DXVECTOR3(),
            D3DXVECTOR3 norm = D3DXVECTOR3(1, 0, 0),
            DWORD color = BLACK              ) : v(v), norm(norm), color(color) {}
};

const D3DVERTEXELEMENT9 VERTEX_ELEMENT[] =
{
    {0, 0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
    {0, 12, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_NORMAL, 0},
    {0, 24, D3DDECLTYPE_D3DCOLOR, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR, 0},
    D3DDECL_END()
};

struct RS
{
    D3DRENDERSTATETYPE state;
    DWORD value;
};
const RS RENDER_STATES[] = {
    { D3DRS_FILLMODE, D3DFILL_SOLID },
    // { D3DRS_CULLMODE, D3DCULL_NONE },
};

const int WINDOW_WIDTH = 700;
const int WINDOW_HEIGHT = 700;

// Light sources!
const D3DXCOLOR     SCENE_COLOR_AMBIENT(0.2f, 0.2f, 0.2f, 0.0f);

const D3DXVECTOR3   DIRECTIONAL_VECTOR( sinf(D3DX_PI/6)*cosf(D3DX_PI/4),
                                        sinf(D3DX_PI/6)*sinf(D3DX_PI/4),
                                       -cosf(D3DX_PI/6));
const D3DXCOLOR     DIRECTIONAL_COLOR_DIFFUSE(0.7f, 0.2f, 0.2f, 0.0f);
const D3DXCOLOR     DIRECTIONAL_COLOR_SPECULAR(0.7f, 0.2f, 0.2f, 0.0f);

const D3DXVECTOR3   POINT_POSITION(2.0f, 0.0f, -1.0f);
const D3DXCOLOR     POINT_COLOR_DIFFUSE(0.2f, 0.7f, 0.2f, 0.0f);
const D3DXCOLOR     POINT_COLOR_SPECULAR(0.2f, 0.7f, 0.2f, 0.0f);
const D3DXVECTOR3   POINT_ATTENUATION_FACTOR(1.0f, 0.5f, 0.2f);

const D3DXVECTOR3   SPOT_POSITION(-3.0f, 6.0f, 1.0f);
const D3DXVECTOR3   SPOT_VECTOR( sinf(D3DX_PI/2.5)*cosf(D3DX_PI/4),
                                -sinf(D3DX_PI/2.5)*sinf(D3DX_PI/4),
                                -cosf(D3DX_PI/2.5));
const D3DXCOLOR     SPOT_COLOR_DIFFUSE(0.0f, 0.0f, 1.0f, 0.0f);
const D3DXCOLOR     SPOT_COLOR_SPECULAR(0.0f, 0.0f, 1.0f, 0.0f);
const D3DXVECTOR3   SPOT_ATTENUATION_FACTOR(1.0f, 0.5f, 0.2f);
const D3DXVECTOR2   SPOT_RANGE_FACTOR(0.99f, 0.97f);


const float MORPHING_SPEED = 0.0f;
const unsigned MORPHING_TIMER_SPEED = 25;
const unsigned TESSELATE_LEVEL = 50;
const unsigned PYRAMID_VERTICES_COUNT = 4 * (TESSELATE_LEVEL + 1) * (TESSELATE_LEVEL + 2); // it's math
const unsigned PYRAMID_INDICES_COUNT = 8 * 3 * TESSELATE_LEVEL * TESSELATE_LEVEL; // it's too

const DWORD PYRAMID_COLOR = WHITE; // set it to 0 to get eight-colored pyramid
const float SPECULAR_DEGRADATION = 0.1f;

const D3DXVECTOR3 PYRAMID_POSITION(0.0f, 3.0f, 0.0f);

const D3DXVECTOR3 INITIAL_PYRAMID[] = {
    D3DXVECTOR3(  1.0f,  1.0f,  0.0f       ),
    D3DXVECTOR3(  1.0f, -1.0f,  0.0f       ),
    D3DXVECTOR3( -1.0f, -1.0f,  0.0f       ),
    D3DXVECTOR3( -1.0f,  1.0f,  0.0f       ),
    D3DXVECTOR3(  0.0f,  0.0f,  sqrtf(2.0) ),
    D3DXVECTOR3(  0.0f,  0.0f, -sqrtf(2.0) ),
};
const unsigned INITIAL_PYRAMID_VCOUNT = sizeof(INITIAL_PYRAMID)/sizeof(INITIAL_PYRAMID[0]);
const float SPHERA_RADIUS = sqrtf(2.0);

enum {
    // matrices
    EYE_REG = 3,
    VIEW_MATRIX_REG = 4,
    PYRAMID_ROTATION_MATRIX_REG = 8,
    PYRAMID_POSITION_MATRIX_REG = 12,

    // morphing
    SPECULAR_DEGRADATION_REG = 32,
    TIME_REG = 33,
    RADIUS_REG = 34,

    // light sources
    SCENE_COLOR_AMBIENT_REG = 64,

    DIRECTIONAL_VECTOR_REG = 65,
    DIRECTIONAL_COLOR_DIFFUSE_REG = 66,
    DIRECTIONAL_COLOR_SPECULAR_REG = 67,

    POINT_POSITION_REG = 68,
    POINT_COLOR_DIFFUSE_REG = 69,
    POINT_COLOR_SPECULAR_REG = 70,
    POINT_ATTENUATION_FACTOR_REG = 71,

    SPOT_POSITION_REG = 72,
    SPOT_VECTOR_REG = 73,
    SPOT_COLOR_DIFFUSE_REG = 74,
    SPOT_COLOR_SPECULAR_REG = 75,
    SPOT_ATTENUATION_FACTOR_REG = 76,
    SPOT_RANGE_FACTOR_REG = 77,
};

unsigned WORLD_DIMENSION = 3;

const unsigned RHO = 0;
const unsigned THETA = 1;
const unsigned PHI = 2;
const unsigned PYRAMID_PHI = 3;

const unsigned TIME_VALUE_INDEX = 16; // in window class memory

struct Coord
{
    float min;
    float max;
    float delta;
    float initial;
};

const Coord COORDS[] = {
    /* MIN */       /* MAX */       /* DELTA */     /* INITIAL */
    { 3.0f,         20.0f,          0.25f,          7.0f      },     // RHO
    { D3DX_PI/24,   D3DX_PI*23/24,  D3DX_PI/24,     D3DX_PI*11/24 }, // THETA
    { -1e37f,       1e37f,          D3DX_PI/24,     -D3DX_PI*3/2 },     // PHI
    { -1e37f,       1e37f,          D3DX_PI/36,     0.0f      }      // PYRAMID PHI
};

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

float GetClassFloat(HWND hWnd, unsigned index) // GetClassFloat
{
    static long res;
    return *reinterpret_cast<float*>(&(res = GetClassLong(hWnd, sizeof(float)*index)));
}
void SetClassFloat(HWND hWnd, unsigned index, float value) // SetClassFloat
{
    SetClassLong(hWnd, sizeof(float)*index, *reinterpret_cast<LONG*>(&value));
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

// PYRAMID TESSELATION ----------------------------------------------------------------------------------
void Add3Indices(DWORD *ib, unsigned *ci, DWORD i1, DWORD i2, DWORD i3)
{
    ib[(*ci)++] = i1;
    ib[(*ci)++] = i2;
    ib[(*ci)++] = i3;
}

DWORD AbsIndex(bool up, unsigned level, unsigned quarter, unsigned index)
{
    const unsigned one_side_v_count = PYRAMID_VERTICES_COUNT / 8;
    return ((up?0:1)*4 + quarter) * one_side_v_count + level*(level+1)/2 + index;
}

DWORD FindOrCreate(Vertex *vb, bool up, unsigned level, unsigned quarter, unsigned index, D3DXVECTOR3 v, D3DXVECTOR3 norm)
{
    DWORD abs_index = AbsIndex(up, level, quarter, index);
    if (vb[abs_index].color == 0) // unitialized vertex
        vb[abs_index] = Vertex(v,
                               norm,
                               PYRAMID_COLOR ? PYRAMID_COLOR : ALL_COLORS[(up?0:1)*4+quarter]);
    return abs_index;
}

void Tesselate(Vertex *vb, DWORD *ib)
{
    unsigned ci = 0; // current index

    memset(vb, 0, PYRAMID_VERTICES_COUNT * sizeof(vb[0]));

    const unsigned sides[][3] = {
        {4, 0, 1},
        {4, 1, 2},
        {4, 2, 3},
        {4, 3, 0},
        {5, 0, 3},
        {5, 3, 2},
        {5, 2, 1},
        {5, 1, 0}
    };
    for(int j = 0; j < sizeof(sides)/sizeof(sides[0]); ++j)
    {
        bool up = ( sides[j][0] == 4 );
        unsigned q = sides[j][up ? 1 : 2];
        D3DXVECTOR3 top = INITIAL_PYRAMID[sides[j][0]];
        D3DXVECTOR3 to_left = (INITIAL_PYRAMID[sides[j][1]] - top)/TESSELATE_LEVEL;
        D3DXVECTOR3 to_right = (INITIAL_PYRAMID[sides[j][2]] - top)/TESSELATE_LEVEL;
        D3DXVECTOR3 norm;
        D3DXVec3Cross(&norm, &to_right, &to_left);
        D3DXVec3Normalize(&norm, &norm);

        FindOrCreate(vb, up, 0, q, 0, top, norm);

        for (unsigned l = 1; l <= TESSELATE_LEVEL; ++l)
        {
            unsigned a = AbsIndex(up, l-1, q, 0);
            FindOrCreate(vb, up, l, q, 0, vb[AbsIndex(up, l-1, q, 0)].v + to_left, norm);
            for (unsigned i = 1; i < l; ++i)
            {
                a = AbsIndex(up, l-1, q, i-1);
                FindOrCreate(vb, up, l, q, i, vb[AbsIndex(up, l-1, q, i-1)].v + to_right, norm);
                Add3Indices(ib, &ci,
                    AbsIndex(up, l, q, i),
                    AbsIndex(up, l, q, i-1),
                    AbsIndex(up, l-1, q, i-1));
                Add3Indices(ib, &ci,
                    AbsIndex(up, l, q, i),
                    AbsIndex(up, l-1, q, i-1), 
                    AbsIndex(up, l-1, q, i));
            }
            a = AbsIndex(up, l-1, q, l-1);
            FindOrCreate(vb, up, l, q, l, vb[AbsIndex(up, l-1, q, l-1)].v + to_right, norm);
            Add3Indices(ib, &ci,
                AbsIndex(up, l, q, l),
                AbsIndex(up, l, q, l-1),
                AbsIndex(up, l-1, q, l-1));
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
    Tesselate(vb, ib);

    unsigned v_size = PYRAMID_VERTICES_COUNT*sizeof(Vertex);
    unsigned i_size = PYRAMID_INDICES_COUNT*sizeof(DWORD);

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

void SetTimeToShader(Device *device, LONG time)
{
    // time: 0..inf -> 0..1
    D3DXVECTOR4 v;
    v.x = v.y = v.z = v.w = (1.0f + sinf(D3DX_PI/2 + D3DXToRadian( static_cast<float>(time*MORPHING_SPEED) )))/2;
    OK( device->SetVertexShaderConstantF(TIME_REG, v, 1) );
    v.x = v.y = v.z = v.w = SPHERA_RADIUS;
    OK( device->SetVertexShaderConstantF(RADIUS_REG, v, 1) );
}

void CalcMatrix(Device *device, float rho, float tetha, float phi, float pyramid_phi)
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

    // Pyramid rotation
    D3DXMATRIX pyramid_rotation(
        cosf(pyramid_phi),  sinf(pyramid_phi), 0, 0,
        -sinf(pyramid_phi), cosf(pyramid_phi), 0, 0,
        0,                  0,                 1, 0,
        0,                  0,                 0, 1
    );
    // Pyramid position
    D3DXMATRIX pyramid_position(
        1.0f, 0.0f, 0.0f, PYRAMID_POSITION.x,
        0.0f, 1.0f, 0.0f, PYRAMID_POSITION.y,
        0.0f, 0.0f, 1.0f, PYRAMID_POSITION.z,
        0.0f, 0.0f, 0.0f, 1.0f
    );

    OK( device->SetVertexShaderConstantF(EYE_REG, eye, 1) );
    OK( device->SetVertexShaderConstantF(VIEW_MATRIX_REG, projMatrix * viewMatrix, WORLD_DIMENSION + 1) );
    OK( device->SetVertexShaderConstantF(PYRAMID_ROTATION_MATRIX_REG, pyramid_rotation, WORLD_DIMENSION + 1) );
    OK( device->SetVertexShaderConstantF(PYRAMID_POSITION_MATRIX_REG, pyramid_position, WORLD_DIMENSION + 1) );
}

void SetLightsToShader(Device *device)
{
    D3DXVECTOR4 v;
    v.x = v.y = v.z = v.w = 1 / SPECULAR_DEGRADATION;
    OK( device->SetVertexShaderConstantF(SPECULAR_DEGRADATION_REG, v, 1) );

    OK( device->SetVertexShaderConstantF(SCENE_COLOR_AMBIENT_REG, SCENE_COLOR_AMBIENT, 1) );

    OK( device->SetVertexShaderConstantF(DIRECTIONAL_VECTOR_REG, DIRECTIONAL_VECTOR, 1) );
    OK( device->SetVertexShaderConstantF(DIRECTIONAL_COLOR_DIFFUSE_REG, DIRECTIONAL_COLOR_DIFFUSE, 1) );
    OK( device->SetVertexShaderConstantF(DIRECTIONAL_COLOR_SPECULAR_REG, DIRECTIONAL_COLOR_SPECULAR, 1) );

    OK( device->SetVertexShaderConstantF(POINT_POSITION_REG, POINT_POSITION, 1) );
    OK( device->SetVertexShaderConstantF(POINT_COLOR_DIFFUSE_REG, POINT_COLOR_DIFFUSE, 1) );
    OK( device->SetVertexShaderConstantF(POINT_COLOR_SPECULAR_REG, POINT_COLOR_SPECULAR, 1) );
    OK( device->SetVertexShaderConstantF(POINT_ATTENUATION_FACTOR_REG, POINT_ATTENUATION_FACTOR, 1) );

    OK( device->SetVertexShaderConstantF(SPOT_POSITION_REG, SPOT_POSITION, 1) );
    OK( device->SetVertexShaderConstantF(SPOT_VECTOR_REG, SPOT_VECTOR, 1) );
    OK( device->SetVertexShaderConstantF(SPOT_COLOR_DIFFUSE_REG, SPOT_COLOR_DIFFUSE, 1) );
    OK( device->SetVertexShaderConstantF(SPOT_COLOR_SPECULAR_REG, SPOT_COLOR_SPECULAR, 1) );
    OK( device->SetVertexShaderConstantF(SPOT_ATTENUATION_FACTOR_REG, SPOT_ATTENUATION_FACTOR, 1) );
    OK( device->SetVertexShaderConstantF(SPOT_RANGE_FACTOR_REG, SPOT_RANGE_FACTOR, 1) );
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
    OK( device->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, 0, PYRAMID_VERTICES_COUNT, 0, PYRAMID_INDICES_COUNT/3) );

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

    Vertex *vb = new Vertex[PYRAMID_VERTICES_COUNT];
    DWORD *ib = new DWORD[PYRAMID_INDICES_COUNT];

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
        wcex.cbClsExtra     = sizeof(float)*3+sizeof(LONG)+sizeof(float); // here would be stored view coordinates
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
        SetClassFloat(hWnd, PYRAMID_PHI, 0.0f);

        // INITIALIZING D3D
        InitD3D(hWnd, &d3d, &device);
        InitVIB(device, &vertex_buffer, &index_buffer, vb, ib);
        InitVDeclAndShader(device, &vertex_declaration, &vertex_shader);

        // SHOWING WINDOW
        ShowWindow(hWnd, nCmdShow);
        UpdateWindow(hWnd);

        // CREATE TIMER FOR ANIMATION
        SetTimer(hWnd, 0, MORPHING_TIMER_SPEED, NULL);

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
                SetTimeToShader(device, GetTime(hWnd));
                CalcMatrix(device,
                    GetClassFloat(hWnd, RHO),
                    GetClassFloat(hWnd, THETA),
                    GetClassFloat(hWnd, PHI),
                    GetClassFloat(hWnd, PYRAMID_PHI)
                );
                SetLightsToShader(device);
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

    delete[] vb;
    delete[] ib;

    return (int) msg.wParam;
}

void IncCoord(HWND hWnd, unsigned coord)
{
    float var = GetClassFloat(hWnd, coord) + COORDS[coord].delta;
    SetClassFloat(hWnd, coord, min(var, COORDS[coord].max));
}

void DecCoord(HWND hWnd, unsigned coord)
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

        case 'a':
        case 'A':
            IncCoord(hWnd, PYRAMID_PHI);
            break;
        case 'd':
        case 'D':
            DecCoord(hWnd, PYRAMID_PHI);
            break;
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
