#include "stdafx.h"
#include "models.h"

const unsigned TEXTURE_REPEATITION = 2;

const D3DXVECTOR3 INITIAL_PYRAMID[] = {       // it is generated for radius = 1
    D3DXVECTOR3(  1.0f,  0.0f,  0.0f       ),
    D3DXVECTOR3(  0.0f, -1.0f,  0.0f       ),
    D3DXVECTOR3( -1.0f,  0.0f,  0.0f       ),
    D3DXVECTOR3(  0.0f,  1.0f,  0.0f       ),
    D3DXVECTOR3(  0.0f,  0.0f,  1.0f       ),
    D3DXVECTOR3(  0.0f,  0.0f, -1.0f       ),
};
const unsigned INITIAL_PYRAMID_VCOUNT = sizeof(INITIAL_PYRAMID)/sizeof(INITIAL_PYRAMID[0]);

Pyramid::Pyramid(IDirect3DDevice9 *device, DWORD color, const TCHAR *shader_file, const TCHAR * shadow_shader_file,
                 const TCHAR *texture_file, const TCHAR *pixel_shader_file,
                 D3DXVECTOR3 position, float time_speed,
                 unsigned granularity, float radius) :
        ModelWithShadow(sizeof(Vertex), VERTEX_ELEMENT,
            4 * (granularity + 1) * (granularity + 2),
            8 * 3 * granularity * granularity,
            position, time_speed),
        radius(radius)
{
    Tesselate(granularity, color);

    InitVIB(device);
    InitVDeclAndShader(device, shader_file, shadow_shader_file);
    InitTextureAndPixelShader(device, texture_file, pixel_shader_file);
}

void Pyramid::SetShaderConstants(IDirect3DDevice9 *device)
{
    ModelWithShadow::SetShaderConstants(device);

    D3DXVECTOR4 v;
    v.x = v.y = v.z = v.w = radius;
    OK( device->SetVertexShaderConstantF(PYRAMID_RADIUS_REG, v, 1) );

    v.x = v.y = v.z = v.w = (1.0f + cosf( static_cast<float>(time)*time_speed ))/2;
    OK( device->SetVertexShaderConstantF(TIME_REG, v, 1) );
}

void Pyramid::Draw(IDirect3DDevice9 *device)
{
    OK( device->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, 0, vcount, 0, icount/3) );
}

void Pyramid::Add3Indices(unsigned *ci, DWORD i1, DWORD i2, DWORD i3)
{
    ib[(*ci)++] = i1;
    ib[(*ci)++] = i2;
    ib[(*ci)++] = i3;
}

DWORD Pyramid::AbsIndex(bool up, unsigned level, unsigned quarter, unsigned index)
{
    const unsigned one_side_v_count = vcount / 8;
    return ((up?0:1)*4 + quarter) * one_side_v_count + level*(level+1)/2 + index;
}

DWORD Pyramid::FindOrCreate(bool up, unsigned level, unsigned quarter, unsigned index, D3DXVECTOR3 v, D3DXVECTOR3 norm, DWORD color)
{
    DWORD abs_index = AbsIndex(up, level, quarter, index);
    Vertex *vertices = reinterpret_cast<Vertex*>(vb);
    if (vertices[abs_index].color == 0) // unitialized vertex
    {
        float tu, tv;
        if (quarter % 2 == 0)
            tu = atan2(fabs(v.y), fabs(v.x))/(D3DX_PI/2)/4 + 0.25f*(quarter);
        else
            tu = -atan2(fabs(v.y), fabs(v.x))/(D3DX_PI/2)/4 + 0.25f*(quarter+1);
        tv = atan2(sqrtf(v.x*v.x+v.y*v.y), v.z)/D3DX_PI;
        tu *= TEXTURE_REPEATITION;
        tv *= TEXTURE_REPEATITION;
        vertices[abs_index] = Vertex(v,
                               norm,
                               color ? color : ALL_COLORS[(up?0:1)*4+quarter],
                               tu, tv);
    }
    return abs_index;
}

void Pyramid::Tesselate(unsigned granularity, DWORD color)
{
    unsigned ci = 0; // current index

    Vertex *vertices = reinterpret_cast<Vertex*>(vb);
    memset(vertices, 0, vcount * sizeof_vertex);

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
        D3DXVECTOR3 top = INITIAL_PYRAMID[sides[j][0]]*radius;
        D3DXVECTOR3 to_left = (INITIAL_PYRAMID[sides[j][1]]*radius - top)/static_cast<float>(granularity);
        D3DXVECTOR3 to_right = (INITIAL_PYRAMID[sides[j][2]]*radius - top)/static_cast<float>(granularity);
        D3DXVECTOR3 norm;
        D3DXVec3Cross(&norm, &to_right, &to_left);
        D3DXVec3Normalize(&norm, &norm);

        FindOrCreate(up, 0, q, 0, top, norm, color);

        for (unsigned l = 1; l <= granularity; ++l)
        {
            unsigned a = AbsIndex(up, l-1, q, 0);
            FindOrCreate(up, l, q, 0, vertices[AbsIndex(up, l-1, q, 0)].v + to_left, norm, color);
            for (unsigned i = 1; i < l; ++i)
            {
                a = AbsIndex(up, l-1, q, i-1);
                FindOrCreate(up, l, q, i, vertices[AbsIndex(up, l-1, q, i-1)].v + to_right, norm, color);
                Add3Indices(&ci,
                    AbsIndex(up, l, q, i),
                    AbsIndex(up, l, q, i-1),
                    AbsIndex(up, l-1, q, i-1));
                Add3Indices(&ci,
                    AbsIndex(up, l, q, i),
                    AbsIndex(up, l-1, q, i-1), 
                    AbsIndex(up, l-1, q, i));
            }
            a = AbsIndex(up, l-1, q, l-1);
            FindOrCreate(up, l, q, l, vertices[AbsIndex(up, l-1, q, l-1)].v + to_right, norm, color);
            Add3Indices(&ci,
                AbsIndex(up, l, q, l),
                AbsIndex(up, l, q, l-1),
                AbsIndex(up, l-1, q, l-1));
        }
    }
}
