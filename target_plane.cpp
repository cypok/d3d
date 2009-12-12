#include "stdafx.h"
#include "models.h"

const unsigned FILTER_SIZE = 3;
const unsigned PIXEL_SHADER_FILTER_REG = 1;
const unsigned FILTERING_SCALE = 8;

TargetPlane::TargetPlane(IDirect3DDevice9 *device, const TCHAR *shader_file,
          const TCHAR * pixel_shader_file,
          unsigned width, unsigned height):
        Model(sizeof(Vertex), VERTEX_ELEMENT,
            (width + 1) * (height + 1),
            6 * width * height,
            D3DXVECTOR3(0.0f, 0.0f, 0.0f), 0.0f),
        normal(D3DXVECTOR3(0.0f, 0.0f, 1.0f)), width(width), height(height), current_filter(0)
{
    Tesselate();

    InitVIB(device);
    InitVDeclAndShader(device, shader_file);
    InitTextureAndPixelShader(device, width, height, pixel_shader_file);
}

void TargetPlane::Tesselate()
{
    Vertex *vertices = reinterpret_cast<Vertex*>(vb);

    // we need to get to vectors (u and v) such as: (u,v)=0, (u,n)=0, (v,n)=0
    D3DXVECTOR3 u = normal, v;
    if (-0.5f < u.x && u.x < 0.5f)
        u.x += 1.0f;
    else
        u.y += 1.0f;
    D3DXVec3Cross(&u, &normal, &u); // now u is ortoganal to normal
    D3DXVec3Cross(&v, &normal, &u); // now v is ortoganal to normal and u
    D3DXVec3Normalize(&u, &u);
    D3DXVec3Normalize(&v, &v);

    u *= 2.0f/static_cast<float>(width);
    v *= 2.0f/static_cast<float>(height);

    unsigned ci = 0; // current index index for adding

    D3DXVECTOR3 start = - (u*static_cast<float>(width)+v*static_cast<float>(height)) / 2;

    for (unsigned i = 0; i < height; ++i)
        for (unsigned j = 0; j <= width; ++j)
        {
            vertices[i*(width+1)+j] = Vertex(start + static_cast<float>(j)*u + static_cast<float>(i)*v, normal, BLACK,
                static_cast<float>(j)/width, static_cast<float>(i)/height);
            if (i == height-1)
                vertices[(i+1)*(width+1)+j] = Vertex(start + static_cast<float>(j)*u + static_cast<float>(i+1)*v, normal, BLACK,
                    static_cast<float>(j)/width, static_cast<float>(i+1)/height);

            if (j > 0)
            {
                ib[ci++] = (i+0)*(width+1)+(j-1);
                ib[ci++] = (i+0)*(width+1)+(j-0);
                ib[ci++] = (i+1)*(width+1)+(j-1);

                ib[ci++] = (i+1)*(width+1)+(j-1);
                ib[ci++] = (i+0)*(width+1)+(j-0);
                ib[ci++] = (i+1)*(width+1)+(j-0);
            }
        }
}

void TargetPlane::Draw(IDirect3DDevice9 *device)
{
    OK( device->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, 0, vcount, 0, icount/3) );
}

IDirect3DSurface9 * TargetPlane::GetSurface()
{
    IDirect3DSurface9 *surface;
    OK( texture->GetSurfaceLevel(0, &surface) );
    return surface;
}

void TargetPlane::SaveTexture()
{
    D3DXSaveTextureToFile(_T("render.bmp"), D3DXIFF_BMP, texture, NULL);
}

D3DXMATRIX * TargetPlane::GetFilter(unsigned n)
{
    static float x;
    x = 1.0f/FILTERING_SCALE;
    static D3DXMATRIX nothing(
        0.0f,   0*x,    0.0f,   0.0f,
        0*x,    x,      0*x,    0.0f,
        0.0f,   0*x,    0.0f,   0.0f,
        0.0f,   0.0f,   0.0f,   0.0f
    );

    x = 1.0f/FILTERING_SCALE;
    static D3DXMATRIX sharpen(
        0.0f,   -x,     0.0f,   0.0f,
        -x,     5*x,    -x,     0.0f,
        0.0f,   -x,     0.0f,   0.0f,
        0.0f,   0.0f,   0.0f,   0.0f
    );

    x = 1.0f/6.0f/FILTERING_SCALE;
    static D3DXMATRIX blur(
        0.0f,   x,      0.0f,   0.0f,
        x,      2*x,    x,      0.0f,
        0.0f,   x,      0.0f,   0.0f,
        0.0f,   0.0f,   0.0f,   0.0f
    );

    x = 0.2f/FILTERING_SCALE;
    static D3DXMATRIX edges(
        0.0f,   -x,     0.0f,   0.0f,
        -x,     4*x,    -x,     0.0f,
        0.0f,   -x,     0.0f,   0.0f,
        0.0f,   0.0f,   0.0f,   0.0f
    );

    x = 1.0f/FILTERING_SCALE;
    static D3DXMATRIX embossing(
        0.0f,   x,      0.0f,   0.0f,
        -x,     0*x,    x,      0.0f,
        0.0f,   -x,     0.0f,   0.0f,
        0.0f,   0.0f,   0.0f,   0.0f
    );

    static D3DXMATRIX *filters[] = {&nothing, &blur, &edges, &embossing, &sharpen};
    return filters[n % (sizeof(filters)/sizeof(filters[0]))];
}

void TargetPlane::SetShaderConstants(IDirect3DDevice9 *device)
{
    float du = 1.0f / static_cast<float>(width);
    float dv = 1.0f / static_cast<float>(height);
    D3DXVECTOR2 center(du/2, -dv/2);
    D3DXVECTOR2 right (du,   0);
    D3DXVECTOR2 up    (0,    dv);
    OK( device->SetVertexShaderConstantF(CENTER_PIXEL_REG, center, 1) );
    OK( device->SetVertexShaderConstantF(UP_PIXEL_REG,     center+up, 1) );
    OK( device->SetVertexShaderConstantF(RIGHT_PIXEL_REG,  center+right, 1) );
    OK( device->SetVertexShaderConstantF(DOWN_PIXEL_REG,   center-up, 1) );
    OK( device->SetVertexShaderConstantF(LEFT_PIXEL_REG,   center-right, 1) );

    OK( device->SetPixelShaderConstantF(PIXEL_SHADER_FILTER_REG, *GetFilter(current_filter), FILTER_SIZE) );
}

void TargetPlane::Render(IDirect3DDevice9 *device)
{
    DWORD cull;
    OK( device->SetRenderState(D3DRS_ZENABLE, FALSE) );
    OK( device->SetRenderState(D3DRS_STENCILENABLE, FALSE) );
    OK( device->GetRenderState(D3DRS_CULLMODE, &cull) );
    OK( device->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE) );
    Model::Render(device);
    OK( device->SetRenderState(D3DRS_ZENABLE, TRUE) );
    OK( device->SetRenderState(D3DRS_STENCILENABLE, TRUE) );
    OK( device->SetRenderState(D3DRS_CULLMODE, cull) );
}

void TargetPlane::SetCurrentFilter(unsigned n)
{
    current_filter = n;
}
