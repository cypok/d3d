#pragma once
#include "stdafx.h"
#include "helper.h"

const unsigned BONES_COUNT = 2;

const D3DVERTEXELEMENT9 PYRAMID_VERTEX_ELEMENT[] =
{
    {0, 0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
    {0, 12, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_NORMAL, 0},
    {0, 24, D3DDECLTYPE_D3DCOLOR, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR, 0},
    D3DDECL_END()
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

const D3DVERTEXELEMENT9 CYLINDER_VERTEX_ELEMENT[] =
{
    {0, 0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
    {0, 12, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_NORMAL, 0},
    {0, 24, D3DDECLTYPE_D3DCOLOR, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR, 0},
    {0, 28, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0},
    D3DDECL_END()
};

struct VertexWithWeights : public Vertex
{
    float weights[BONES_COUNT];

    VertexWithWeights(  D3DXVECTOR3 v = D3DXVECTOR3(),
                        D3DXVECTOR3 norm = D3DXVECTOR3(1, 0, 0),
                        DWORD color = BLACK,
                        float w = 1) : Vertex(v, norm, color)
    {
        weights[0] = w;
        weights[1] = 1-w;
    }
};

class Model
{
protected:
    IDirect3DDevice9 *device;
    IDirect3DVertexBuffer9 *vertex_buffer;
    IDirect3DIndexBuffer9 *index_buffer;
    IDirect3DVertexShader9 *vertex_shader;
    IDirect3DVertexDeclaration9 *vertex_declaration;

    void *vb;
    DWORD *ib;
    const unsigned vcount;
    const unsigned icount;

    static unsigned time;
    const float time_speed;

    const unsigned sizeof_vertex;
    const D3DVERTEXELEMENT9 *vertex_element;
    const TCHAR *shader_file;

    float angle_phi;
    D3DXVECTOR3 position;

    void InitVIB(IDirect3DDevice9 *device);
    void InitVDeclAndShader(IDirect3DDevice9 *device);

    virtual void SetShaderConstants(IDirect3DDevice9 *device);
    virtual void Draw(IDirect3DDevice9 *device) = 0;

public:
    Model(const unsigned sizeof_vertex, const D3DVERTEXELEMENT9 *vertex_element,
             const TCHAR * shader_file, const unsigned vcount, const unsigned icount,
             D3DXVECTOR3 position, float time_speed);
    void Render(IDirect3DDevice9 *device);
    void SetRotation(float angle);
    void SetPosition(D3DXVECTOR3 position);
    static void SetTime(unsigned time);
    virtual ~Model();
private:
    // no copying
    Model(const Model &);
    Model & operator=(const Model &);
};

class Pyramid : public Model
{
protected:
    const float radius;

    void Add3Indices(unsigned *ci, DWORD i1, DWORD i2, DWORD i3);
    DWORD AbsIndex(bool up, unsigned level, unsigned quarter, unsigned index);
    DWORD FindOrCreate(bool up, unsigned level, unsigned quarter, unsigned index, D3DXVECTOR3 v, D3DXVECTOR3 norm, DWORD color);
    void Tesselate(unsigned granularity, DWORD color);

    virtual void SetShaderConstants(IDirect3DDevice9 *device);
    void Draw(IDirect3DDevice9 *device);
public:
    Pyramid(IDirect3DDevice9 *device, DWORD color, const TCHAR *shader_file,
            D3DXVECTOR3 position, float time_speed,
            unsigned granularity, float radius);
};

class Cylinder : public Model
{
protected:
    const float height;
    const float radius;
    const float rotation_angle;


    void Tesselate(unsigned vertical_granularity, unsigned horizontal_granularity, DWORD color);

    virtual void SetShaderConstants(IDirect3DDevice9 *device);
    void Draw(IDirect3DDevice9 *device);
public:
    Cylinder(IDirect3DDevice9 *device, DWORD color, const TCHAR *shader_file,
             D3DXVECTOR3 position, float time_speed,
             unsigned vertical_granularity, unsigned horizontal_granularity,
             float height, float radius, float rotation_angle);
};
