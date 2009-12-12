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
    //{ D3DRS_FILLMODE, D3DFILL_WIREFRAME },
    //{ D3DRS_CULLMODE, D3DCULL_NONE },
    { D3DRS_STENCILENABLE, true },
    { D3DRS_STENCILREF, 1 },
    { D3DRS_ALPHABLENDENABLE, true },
};

const TCHAR         PYRAMID_SHADER[]        = _T("pyramid.vsh");
const TCHAR         PYRAMID_SHADOW_SHADER[] = _T("pyramid_shadow.vsh");
const unsigned      PYRAMID_GRANULARITY     = 300;
const D3DXVECTOR3   PYRAMID_POSITION        = D3DXVECTOR3(0.0f, 0.0f, -0.5f);
const float         PYRAMID_RADIUS          = sqrtf(1.5f);
const float         PYRAMID_ORBIT           = 1.5f;
const float         PYRAMID_MORPHING_SPEED  = 0.00f;
const DWORD         PYRAMID_COLOR           = D3DCOLOR_XRGB(40, 200, 200);
const TCHAR         PYRAMID_TEXTURE[]       = _T("earth.png");
const TCHAR         PYRAMID_PIXEL_SHADER[]  = _T("simple.psh");

const DWORD         PLANE_COLOR             = D3DCOLOR_XRGB(170, 170, 170);
const TCHAR         PLANE_SHADER[]          = _T("plane.vsh");
const D3DXVECTOR3   PLANE_POSITION          = D3DXVECTOR3(0.0f, 0.0f, -2.0f);
const D3DXVECTOR3   PLANE_NORMAL            = D3DXVECTOR3(0.0f, 0.0f, 1.0f);
const unsigned      PLANE_GRANULARITY       = 1;
const float         PLANE_SIZE              = 50.0f;
const TCHAR         PLANE_TEXTURE[]         = _T("goliath.jpg");
const TCHAR         PLANE_PIXEL_SHADER[]    = _T("simple.psh");

const TCHAR         TARGET_SHADER[]          = _T("target.vsh");
const TCHAR         TARGET_PIXEL_SHADER[]    = _T("target.psh");

const unsigned TIMER_FREQ = 10;

const int WINDOW_WIDTH = 750;
const int WINDOW_HEIGHT = 750;

// Light sources!
const D3DXCOLOR     SCENE_COLOR_AMBIENT(0.2f, 0.2f, 0.2f, 0.0f);

const D3DXVECTOR3   POINT_POSITION(0.5f, -1.5f, 0.0f); // z-coord could be variated
const D3DXCOLOR     POINT_COLOR_DIFFUSE(0.7f, 0.7f, 0.7f, 0.0f);
const D3DXCOLOR     POINT_COLOR_SPECULAR(0.5f, 0.5f, 0.5f, 0.0f);
const D3DXVECTOR3   POINT_ATTENUATION_FACTOR(0.5f, 0.0f, 0.1f);

const TCHAR         BULB_SHADER[] = _T("bulb.vsh");
const unsigned      BULB_GRANULARITY = 5;
const float         BULB_RADIUS = 0.1f;

const float SPECULAR_DEGRADATION = 0.1f;

const unsigned RHO = 0;
const unsigned THETA = 1;
const unsigned PHI = 2;
const unsigned PYRAMID_PHI = 3;
const unsigned PYRAMID_ORBIT_PHI = 4;
const unsigned LIGHT_POS = 5;
const unsigned FILTER = 6;
const unsigned TIME_VALUE_INDEX = 7;

const unsigned CLASS_VARIABLES = 8;

struct Coord
{
    float min;
    float max;
    float delta;
    float initial;
};

const Coord COORDS[] = {
    /* MIN */       /* MAX */       /* DELTA */     /* INITIAL */
    { 3.0f,         20.0f,          0.25f,          4.0f      },     // RHO
    { D3DX_PI/24,   D3DX_PI*23/24,  D3DX_PI/24,     D3DX_PI*11/24 }, // THETA
    { -1e37f,       1e37f,          D3DX_PI/24,     D3DX_PI*3/2 },     // PHI
    { -1e37f,       1e37f,          D3DX_PI/36,     0.0f      },      // PYRAMID PHI
    { -1e37f,       1e37f,          D3DX_PI/36,     0.0f      },      // PYRAMID ORBIT PHI
    { -1.80f,       1e37f,          0.05f,          1.5f      },      // LIGHT POSITION
};


LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

float GetClassFloat(HWND hWnd, unsigned index) // GetClassFloat
{
    static long res;
    return *reinterpret_cast<float*>(&(res = GetClassLong(hWnd, index)));
}
void SetClassFloat(HWND hWnd, unsigned index, float value) // SetClassFloat
{
    SetClassLong(hWnd, index, *reinterpret_cast<LONG*>(&value));
}
LONG GetTime(HWND hWnd)
{
    return GetClassLong(hWnd, TIME_VALUE_INDEX);
}
void IncTime(HWND hWnd)
{
    SetClassLong(hWnd, sizeof(float)*TIME_VALUE_INDEX, 1+GetClassLong(hWnd, sizeof(float)*TIME_VALUE_INDEX));
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
    params.AutoDepthStencilFormat = D3DFMT_D24S8;

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

    OK( (*device)->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR) );
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

void SetLights(IDirect3DDevice9 *device, float light_pos)
{
    D3DXVECTOR4 v;
    v.x = v.y = v.z = v.w = 1 / SPECULAR_DEGRADATION;
    OK( device->SetVertexShaderConstantF(SPECULAR_DEGRADATION_REG, v, 1) );

    OK( device->SetVertexShaderConstantF(SCENE_COLOR_AMBIENT_REG, SCENE_COLOR_AMBIENT, 1) );

    OK( device->SetVertexShaderConstantF(POINT_POSITION_REG, POINT_POSITION+D3DXVECTOR3(0, 0, light_pos), 1) );
    OK( device->SetVertexShaderConstantF(POINT_COLOR_DIFFUSE_REG, POINT_COLOR_DIFFUSE, 1) );
    OK( device->SetVertexShaderConstantF(POINT_COLOR_SPECULAR_REG, POINT_COLOR_SPECULAR, 1) );
    OK( device->SetVertexShaderConstantF(POINT_ATTENUATION_FACTOR_REG, POINT_ATTENUATION_FACTOR, 1) );

}

D3DXMATRIX CreateShadowMatrix(D3DXVECTOR3 light_pos, D3DXVECTOR3 plane_pos, D3DXVECTOR3 plane_norm)
{
    D3DXVECTOR3 &l = light_pos;
    D3DXVECTOR3 &n = plane_norm;
    float d = D3DXVec3Dot(&plane_pos, &n);

    D3DXMATRIX M1( d, 0, 0, -d*l.x,
                   0, d, 0, -d*l.y,
                   0, 0, d, -d*l.z,
                   0, 0, 0,  0 );

    float pn = D3DXVec3Dot(&l, &n);
    D3DXMATRIX M2( pn, 0,  0,  0,
                   0,  pn, 0,  0,
                   0,  0,  pn, 0,
                   0,  0,  0,  0 );

    D3DXMATRIX M3( l.x*n.x, l.x*n.y, l.x*n.z, 0,
                   l.y*n.x, l.y*n.y, l.y*n.z, 0,
                   l.z*n.x, l.z*n.y, l.z*n.z, 0,
                   0,       0,       0,       0 );

    D3DXMATRIX M4( 0,   0,   0,  0,
                   0,   0,   0,  0,
                   0,   0,   0,  0,
                   n.x, n.y, n.z, -pn );

    return -(M1-M2+M3+M4);
}

void Render(IDirect3DDevice9 *device, Model * plane, Model * bulb, std::vector<Model*> models, Model *target)
{
    IDirect3DSurface9 *old_surface, *surface;
    surface = dynamic_cast<TargetPlane*>(target)->GetSurface();

    OK( device->BeginScene() );

        OK( device->GetRenderTarget(0, &old_surface) );

        // render to texture
        OK( device->SetRenderTarget(0, surface) );
        OK( device->Clear( 0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER | D3DCLEAR_STENCIL, GRAY, 1.0f, 0 ) );
        
        bulb->Render(device);

        OK( device->SetRenderState(D3DRS_STENCILPASS, D3DSTENCILOP_REPLACE) );
        plane->Render(device);

        OK( device->SetRenderState(D3DRS_STENCILPASS, D3DSTENCILOP_ZERO) );
        for(unsigned i = 0; i < models.size(); ++i)
            models[i]->Render(device);

        //dynamic_cast<TargetPlane*>(target)->SaveTexture();

        // render to display
        OK( device->SetRenderTarget(0, old_surface) );
        OK( device->Clear( 0, NULL, D3DCLEAR_TARGET, GREEN, 1.0f, 0 ) );

        target->Render(device);

    OK( device->EndScene() );
    OK( device->Present(NULL, NULL, NULL, NULL) );

    ReleaseInterface(old_surface);
    ReleaseInterface(surface);
}

int APIENTRY _tWinMain(HINSTANCE hInstance, HINSTANCE /*hPrevInstance*/, LPTSTR /*lpCmdLine*/, int nCmdShow)
{    
    IDirect3D9 *d3d = NULL;
    IDirect3DDevice9 *device = NULL;
    Pyramid *pyramid = NULL;
    Pyramid *bulb = NULL;
    Plane *plane = NULL;
    TargetPlane *target = NULL;

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
        wcex.cbClsExtra     = sizeof(float)*CLASS_VARIABLES; // here would be stored view coordinates
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

        RECT window;
        GetClientRect(hWnd, &window);

        SetClassFloat(hWnd, sizeof(float)*RHO, COORDS[RHO].initial);
        SetClassFloat(hWnd, sizeof(float)*THETA, COORDS[THETA].initial);
        SetClassFloat(hWnd, sizeof(float)*PHI, COORDS[PHI].initial);
        SetClassLong(hWnd, sizeof(float)*TIME_VALUE_INDEX, 0);
        SetClassFloat(hWnd, sizeof(float)*PYRAMID_PHI, COORDS[PYRAMID_PHI].initial);
        SetClassFloat(hWnd, sizeof(float)*PYRAMID_ORBIT_PHI, COORDS[PYRAMID_ORBIT_PHI].initial);
        SetClassFloat(hWnd, sizeof(float)*LIGHT_POS, COORDS[LIGHT_POS].initial);
        SetClassLong(hWnd, sizeof(float)*FILTER, 0);

        // INITIALIZING D3D
        InitD3D(hWnd, &d3d, &device);
        pyramid = new Pyramid(device, PYRAMID_COLOR, PYRAMID_SHADER, PYRAMID_SHADOW_SHADER, PYRAMID_TEXTURE, PYRAMID_PIXEL_SHADER,
            PYRAMID_POSITION, PYRAMID_MORPHING_SPEED, PYRAMID_GRANULARITY, PYRAMID_RADIUS);
        bulb = new Pyramid(device, POINT_COLOR_DIFFUSE, BULB_SHADER, NULL, NULL, NULL, 
            POINT_POSITION, 0, BULB_GRANULARITY, BULB_RADIUS);
        plane = new Plane(device, PLANE_COLOR, PLANE_SHADER, PLANE_TEXTURE, PLANE_PIXEL_SHADER, PLANE_POSITION, PLANE_NORMAL, PLANE_GRANULARITY, PLANE_SIZE);
        target = new TargetPlane(device, TARGET_SHADER, TARGET_PIXEL_SHADER, window.right-window.left, window.bottom - window.top);

        std::vector<Model*> models;
        models.push_back(pyramid);

        // SHOWING WINDOW
        ShowWindow(hWnd, nCmdShow);
        UpdateWindow(hWnd);

        // CREATE TIMER FOR ANIMATION
        SetTimer(hWnd, 0, TIMER_FREQ, NULL);

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
                SetLights(device, GetClassFloat(hWnd, sizeof(float)*LIGHT_POS));
                SetViewMatrix(device,
                    GetClassFloat(hWnd, sizeof(float)*RHO),
                    GetClassFloat(hWnd, sizeof(float)*THETA),
                    GetClassFloat(hWnd, sizeof(float)*PHI)
                );
                D3DXMATRIX m = CreateShadowMatrix(POINT_POSITION+D3DXVECTOR3(0,0,GetClassFloat(hWnd, sizeof(float)*LIGHT_POS)), PLANE_POSITION, PLANE_NORMAL);
                pyramid->SetShadowMatrix(m);

                pyramid->SetRotation(GetClassFloat(hWnd, sizeof(float)*PYRAMID_PHI));
                float angle = GetClassFloat(hWnd, sizeof(float)*PYRAMID_ORBIT_PHI);
                pyramid->SetPosition(PYRAMID_POSITION + D3DXVECTOR3(PYRAMID_ORBIT*cosf(angle),
                                                                     PYRAMID_ORBIT*sinf(angle),
                                                                     0));
                bulb->SetPosition(POINT_POSITION+D3DXVECTOR3(0,0,GetClassFloat(hWnd, sizeof(float)*LIGHT_POS)));
                target->SetCurrentFilter(GetClassLong(hWnd, sizeof(float)*FILTER));
                Render(device, plane, bulb, models, target);
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

    delete pyramid;
    delete plane;
    delete bulb;
    delete target;

    return (int) msg.wParam;
}

void IncCoord(HWND hWnd, unsigned coord)
{
    float var = GetClassFloat(hWnd, sizeof(float)*coord) + COORDS[coord].delta;
    SetClassFloat(hWnd, sizeof(float)*coord, min(var, COORDS[coord].max));
}

void DecCoord(HWND hWnd, unsigned coord)
{
    float var = GetClassFloat(hWnd, sizeof(float)*coord) - COORDS[coord].delta;
    SetClassFloat(hWnd, sizeof(float)*coord, max(var, COORDS[coord].min));
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

        case 'W': IncCoord(hWnd, LIGHT_POS); break;
        case 'S': DecCoord(hWnd, LIGHT_POS); break;
        
        case VK_SPACE:
            SetClassLong(hWnd, sizeof(float)*FILTER, GetClassLong(hWnd, sizeof(float)*FILTER) + 1);
            break;

        case VK_ESCAPE:
            PostQuitMessage(0);
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
