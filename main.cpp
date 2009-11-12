#include "stdafx.h"
#include "main.h"
#include "helper.h"
#include "models.h"
#include "shader_reg.h"

struct RS
{
    D3DRENDERSTATETYPE state;
    DWORD value;
};
const RS RENDER_STATES[] = {
    { D3DRS_FILLMODE, D3DFILL_SOLID },
    { D3DRS_CULLMODE, D3DCULL_NONE },
};

const TCHAR         PYRAMID_SHADER[]        = _T("pyramid.vsh");
const unsigned      PYRAMID_GRANULARITY     = 100;
const D3DXVECTOR3   PYRAMID_POSITION        = D3DXVECTOR3(0.0f, 0.0f, 0.0f);
const float         PYRAMID_RADIUS_1        = sqrtf(2.0f);
const float         PYRAMID_RADIUS_2        = sqrtf(2.0f);
const float         PYRAMID_ORBIT           = 2.0f;
const float         PYRAMID_MORPHING_SPEED  = 0.01f;
const DWORD         PYRAMID_COLOR           = WHITE;

const TCHAR         CYLINDER_SHADER[]               = _T("cylinder.vsh");
const D3DXVECTOR3   CYLINDER_POSITION               = D3DXVECTOR3(0.0f, 0.0f, 1.0f);
const unsigned      CYLINDER_VERTICAL_GRANULARITY   = 100;
const unsigned      CYLINDER_HORIZONTAL_GRANULARIRY = 100;
const float         CYLINDER_HEIGHT                 = 3.0f;
const float         CYLINDER_RADIUS                 = 0.5f;
const float         CYLINDER_OSCILLATION_SPEED      = 0.015f;
const float         CYLINDER_ROTATION_ANGLE         = D3DX_PI/3;
const DWORD         CYLINDER_COLOR                  = WHITE;

const unsigned TIMER_FREQ = 10;

const int WINDOW_WIDTH = 700;
const int WINDOW_HEIGHT = 700;

// Light sources!
const D3DXCOLOR     SCENE_COLOR_AMBIENT(0.2f, 0.2f, 0.2f, 0.0f);

const D3DXVECTOR3   DIRECTIONAL_VECTOR( sinf(D3DX_PI/6)*cosf(D3DX_PI/4),
                                        sinf(D3DX_PI/6)*sinf(D3DX_PI/4),
                                       -cosf(D3DX_PI/6));
const D3DXCOLOR     DIRECTIONAL_COLOR_DIFFUSE(0.7f, 0.0f, 0.0f, 0.0f);
const D3DXCOLOR     DIRECTIONAL_COLOR_SPECULAR(0.8f, 0.8f, 0.8f, 0.0f);

const D3DXVECTOR3   POINT_POSITION(2.0f, -3.0f, -1.5f);
const D3DXCOLOR     POINT_COLOR_DIFFUSE(0.7f, 0.7f, 0.7f, 0.0f);
const D3DXCOLOR     POINT_COLOR_SPECULAR(0.5f, 0.5f, 0.5f, 0.0f);
const D3DXVECTOR3   POINT_ATTENUATION_FACTOR(1.0f, 0.5f, 0.2f);

const D3DXVECTOR3   SPOT_POSITION(-4.0f, 4.0f, -1.0f);
const D3DXVECTOR3   SPOT_VECTOR( sinf(D3DX_PI/3.0f)*cosf(D3DX_PI/4),
                                -sinf(D3DX_PI/3.0f)*sinf(D3DX_PI/4),
                                 cosf(D3DX_PI/3.0f));
const D3DXCOLOR     SPOT_COLOR_DIFFUSE(0.0f, 0.0f, 0.7f, 0.0f);
const D3DXCOLOR     SPOT_COLOR_SPECULAR(0.0f, 0.0f, 0.5f, 0.0f);
const D3DXVECTOR3   SPOT_ATTENUATION_FACTOR(1.0f, 0.5f, 0.2f);
const D3DXVECTOR2   SPOT_RANGE_FACTOR(0.99f, 0.97f);

const float SPECULAR_DEGRADATION = 0.1f;

// COLORS COUNT SHOULD BE < 16
const D3DXCOLOR ANISOTROPIC_DIFFUSE_COLORS[] = {
    D3DXCOLOR( 0.00f, 0.00f, 0.00f,   0.00f),
    D3DXCOLOR( 0.20f, 0.00f, 0.00f,   0.00f),
    D3DXCOLOR( 0.40f, 0.00f, 0.00f,   0.00f),
    D3DXCOLOR( 0.60f, 0.00f, 0.00f,   0.00f),
    D3DXCOLOR( 0.40f, 0.20f, 0.00f,   0.00f),
    D3DXCOLOR( 0.20f, 0.40f, 0.00f,   0.00f),
    D3DXCOLOR( 0.00f, 0.60f, 0.00f,   0.00f),
    D3DXCOLOR( 0.00f, 0.40f, 0.20f,   0.00f),
    D3DXCOLOR( 0.00f, 0.20f, 0.40f,   0.00f),
    D3DXCOLOR( 0.20f, 0.00f, 0.40f,   0.00f),
    D3DXCOLOR( 0.40f, 0.00f, 0.20f,   0.00f),
    D3DXCOLOR( 0.60f, 0.00f, 0.00f,   0.00f),
    D3DXCOLOR( 0.40f, 0.20f, 0.00f,   0.00f),
    D3DXCOLOR( 0.20f, 0.40f, 0.00f,   0.00f),
    D3DXCOLOR( 0.00f, 0.60f, 0.00f,   0.00f),
};
const unsigned ANISOTROPIC_DIFFUSE_COLORS_COUNT = sizeof(ANISOTROPIC_DIFFUSE_COLORS)/sizeof(ANISOTROPIC_DIFFUSE_COLORS[0]);

// COLORS COUNT SHOULD BE < 16
const D3DXCOLOR ANISOTROPIC_SPECULAR_COLORS[] = {
    D3DXCOLOR( 0.00f, 0.00f, 0.00f,   0.00f),
    D3DXCOLOR( 0.20f, 0.20f, 0.20f,   0.00f),
    D3DXCOLOR( 0.40f, 0.40f, 0.40f,   0.00f),
    D3DXCOLOR( 0.60f, 0.60f, 0.60f,   0.00f),
    D3DXCOLOR( 0.70f, 0.70f, 0.70f,   0.00f),
    D3DXCOLOR( 0.80f, 0.80f, 0.80f,   0.00f),
    D3DXCOLOR( 0.90f, 0.90f, 0.90f,   0.00f),
    D3DXCOLOR( 1.00f, 1.00f, 1.00f,   0.00f),
    D3DXCOLOR( 0.90f, 0.90f, 0.90f,   0.00f),
    D3DXCOLOR( 0.80f, 0.80f, 0.80f,   0.00f),
    D3DXCOLOR( 0.70f, 0.70f, 0.70f,   0.00f),
    D3DXCOLOR( 0.60f, 0.60f, 0.60f,   0.00f),
    D3DXCOLOR( 0.40f, 0.40f, 0.40f,   0.00f),
    D3DXCOLOR( 0.20f, 0.20f, 0.20f,   0.00f),
    D3DXCOLOR( 0.00f, 0.00f, 0.00f,   0.00f),
};
const unsigned ANISOTROPIC_SPECULAR_COLORS_COUNT = sizeof(ANISOTROPIC_SPECULAR_COLORS)/sizeof(ANISOTROPIC_SPECULAR_COLORS[0]);

const unsigned RHO = 0;
const unsigned THETA = 1;
const unsigned PHI = 2;
const unsigned PYRAMID_PHI = 3;
const unsigned PYRAMID_ORBIT_PHI = 4;
const unsigned CYLINDER_PHI = 5;

const unsigned TIME_VALUE_INDEX = 24; // in window class memory

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
    { -1e37f,       1e37f,          D3DX_PI/24,     D3DX_PI*3/2 },     // PHI
    { -1e37f,       1e37f,          D3DX_PI/36,     0.0f      },      // PYRAMID PHI
    { -1e37f,       1e37f,          D3DX_PI/36,     0.0f      },      // PYRAMID ORBIT PHI
    { -1e37f,       1e37f,          D3DX_PI/36,     D3DX_PI/4 },      // CYLINDER PHI
};


LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

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

void InitD3D(HWND hWnd, IDirect3D9 **d3d, IDirect3DDevice9 **device)
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
                                    D3DDEVTYPE_HAL,
                                    hWnd,
                                    D3DCREATE_HARDWARE_VERTEXPROCESSING,
                                    &params,
                                    device) );

    for(int i = 0; i < sizeof(RENDER_STATES)/sizeof(RENDER_STATES[0]); ++i)
        OK( (*device)->SetRenderState( RENDER_STATES[i].state,
                                       RENDER_STATES[i].value )
        );
}

void SetViewMatrix(IDirect3DDevice9 *device, float rho, float tetha, float phi)
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

    OK( device->SetVertexShaderConstantF(EYE_REG, eye, 1) );
    OK( device->SetVertexShaderConstantF(VIEW_MATRIX_REG, projMatrix * viewMatrix, WORLD_DIMENSION + 1) );
}

void SetLights(IDirect3DDevice9 *device)
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
    v.x = 1/(SPOT_RANGE_FACTOR.x-SPOT_RANGE_FACTOR.y);
    v.y = SPOT_RANGE_FACTOR.y*v.x;
    v.z = v.w = 0;
    OK( device->SetVertexShaderConstantF(SPOT_RANGE_FACTOR_REG, v, 1) );


    v.x = v.y = v.z = v.w = -1 + static_cast<float>(ANISOTROPIC_DIFFUSE_COLORS_COUNT);
    OK( device->SetVertexShaderConstantF(ANISOTROPIC_DIFFUSE_COLORS_REG, v, 1) );
    OK( device->SetVertexShaderConstantF(   ANISOTROPIC_DIFFUSE_COLORS_REG+1,
                                            (float *)(ANISOTROPIC_DIFFUSE_COLORS),
                                            ANISOTROPIC_DIFFUSE_COLORS_COUNT) );

    v.x = v.y = v.z = v.w = -1 + static_cast<float>(ANISOTROPIC_SPECULAR_COLORS_COUNT);
    OK( device->SetVertexShaderConstantF(ANISOTROPIC_SPECULAR_COLORS_REG, v, 1) );
    OK( device->SetVertexShaderConstantF(   ANISOTROPIC_SPECULAR_COLORS_REG+1,
                                            (float *)(ANISOTROPIC_SPECULAR_COLORS),
                                            ANISOTROPIC_SPECULAR_COLORS_COUNT) );
}

void Render(IDirect3DDevice9 *device, std::vector<Model*> models)
{
    OK( device->BeginScene() );
    OK( device->Clear( 0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, GRAY, 1.0f, 0 ) );

    for(unsigned i = 0; i < models.size(); ++i)
        models[i]->Render(device);

    OK( device->EndScene() );
    OK( device->Present(NULL, NULL, NULL, NULL) );
}

int APIENTRY _tWinMain(HINSTANCE hInstance, HINSTANCE /*hPrevInstance*/, LPTSTR /*lpCmdLine*/, int nCmdShow)
{    
    IDirect3D9 *d3d = NULL;
    IDirect3DDevice9 *device = NULL;
    Pyramid *pyramid1 = NULL;
    Pyramid *pyramid2 = NULL;
    Cylinder *cylinder = NULL;

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
        wcex.cbClsExtra     = sizeof(float)*3+sizeof(LONG)+sizeof(float)*3; // here would be stored view coordinates
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
        SetClassFloat(hWnd, PYRAMID_PHI, COORDS[PYRAMID_PHI].initial);
        SetClassFloat(hWnd, PYRAMID_ORBIT_PHI, COORDS[PYRAMID_ORBIT_PHI].initial);

        // INITIALIZING D3D
        InitD3D(hWnd, &d3d, &device);
        pyramid1 = new Pyramid(device, PYRAMID_COLOR, PYRAMID_SHADER, PYRAMID_POSITION, PYRAMID_MORPHING_SPEED,
            PYRAMID_GRANULARITY, PYRAMID_RADIUS_1);
        pyramid2 = new Pyramid(device, PYRAMID_COLOR, PYRAMID_SHADER, PYRAMID_POSITION, PYRAMID_MORPHING_SPEED,
            PYRAMID_GRANULARITY, PYRAMID_RADIUS_2);
        cylinder = new Cylinder(device, CYLINDER_COLOR, CYLINDER_SHADER, CYLINDER_POSITION, CYLINDER_OSCILLATION_SPEED,
                                CYLINDER_VERTICAL_GRANULARITY, CYLINDER_HORIZONTAL_GRANULARIRY,
                                CYLINDER_HEIGHT, CYLINDER_RADIUS, CYLINDER_ROTATION_ANGLE);
        std::vector<Model*> models;
        models.push_back(pyramid1);
        models.push_back(pyramid2);
        models.push_back(cylinder);

        // SHOWING WINDOW
        ShowWindow(hWnd, nCmdShow);
        UpdateWindow(hWnd);

        // CREATE TIMER FOR ANIMATION
        SetTimer(hWnd, 0, TIMER_FREQ, NULL);

        SetLights(device);

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
                Model::SetTime(GetTime(hWnd));
                SetViewMatrix(device,
                    GetClassFloat(hWnd, RHO),
                    GetClassFloat(hWnd, THETA),
                    GetClassFloat(hWnd, PHI)
                );

                pyramid1->SetRotation(GetClassFloat(hWnd, PYRAMID_PHI));
                pyramid2->SetRotation(GetClassFloat(hWnd, PYRAMID_PHI));
                cylinder->SetRotation(GetClassFloat(hWnd, CYLINDER_PHI));
                float angle = GetClassFloat(hWnd, PYRAMID_ORBIT_PHI);
                pyramid1->SetPosition(PYRAMID_POSITION + D3DXVECTOR3(PYRAMID_ORBIT*cosf(angle),
                                                                     PYRAMID_ORBIT*sinf(angle),
                                                                     0));
                pyramid2->SetPosition(PYRAMID_POSITION + D3DXVECTOR3(-PYRAMID_ORBIT*cosf(angle),
                                                                     -PYRAMID_ORBIT*sinf(angle),
                                                                     0));
                Render(device, models);
            }
        }
    }
    catch(std::exception &e)
    {
        MessageBoxA(NULL, e.what(), "Error", MB_ICONERROR);
    }

    // CLEANING
    ReleaseInterface(d3d);
    ReleaseInterface(device);

    delete pyramid1;
    delete pyramid2;
    delete cylinder;

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

        case 'A': IncCoord(hWnd, PYRAMID_PHI); break;
        case 'D': DecCoord(hWnd, PYRAMID_PHI); break;

        case 'Q': IncCoord(hWnd, PYRAMID_ORBIT_PHI); break;
        case 'E': DecCoord(hWnd, PYRAMID_ORBIT_PHI); break;

        case 'Z': IncCoord(hWnd, CYLINDER_PHI); break;
        case 'C': DecCoord(hWnd, CYLINDER_PHI); break;
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
