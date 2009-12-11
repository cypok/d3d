#include "stdafx.h"
#include "models.h"

Plane::Plane(IDirect3DDevice9 *device, DWORD color, const TCHAR *shader_file,
             const TCHAR *texture_file, const TCHAR *pixel_shader_file,
             D3DXVECTOR3 position, D3DXVECTOR3 normal,
             unsigned granularity, float size) :
        Model(sizeof(Vertex), VERTEX_ELEMENT,
            (granularity + 1) * (granularity + 1),
            6 * granularity * granularity,
            position, 0.0f),
        normal(normal), size(size)
{
    Tesselate(granularity, color);

    InitVIB(device);
    InitVDeclAndShader(device, shader_file);
    InitTextureAndPixelShader(device, texture_file, pixel_shader_file);
}

void Plane::Tesselate(unsigned granularity, DWORD color)
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
    u *= size / granularity;
    v *= size / granularity;

    unsigned ci = 0; // current index index for adding

    D3DXVECTOR3 start;
    start = - (u+v) * (static_cast<float>(granularity)/2);

    for (unsigned i = 0; i < granularity; ++i)
        for (unsigned j = 0; j < granularity+1; ++j)
        {
            vertices[i*(granularity+1)+j] = Vertex(start + static_cast<float>(j)*u + static_cast<float>(i)*v, normal, color,
                static_cast<float>(j)/granularity, static_cast<float>(i)/granularity);
            if (i == granularity-1)
                vertices[(i+1)*(granularity+1)+j] = Vertex(start + static_cast<float>(j)*u + static_cast<float>(i+1)*v, normal, color,
                    static_cast<float>(j)/granularity, static_cast<float>(i+1)/granularity);

            if (j > 0)
            {
                ib[ci++] = (i+0)*(granularity+1)+(j-1);
                ib[ci++] = (i+0)*(granularity+1)+(j-0);
                ib[ci++] = (i+1)*(granularity+1)+(j-1);

                ib[ci++] = (i+1)*(granularity+1)+(j-1);
                ib[ci++] = (i+0)*(granularity+1)+(j-0);
                ib[ci++] = (i+1)*(granularity+1)+(j-0);
            }
        }
}

void Plane::Draw(IDirect3DDevice9 *device)
{
    OK( device->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, 0, vcount, 0, icount/3) );
}
