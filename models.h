#pragma once
#include "stdafx.h"
#include "helper.h"


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
    Model::Model(const unsigned sizeof_vertex, const D3DVERTEXELEMENT9 *vertex_element,
             const TCHAR * shader_file, const unsigned vcount, const unsigned icount);
    void Render(IDirect3DDevice9 *device);
    void SetRotation(float angle);
    void SetPosition(D3DXVECTOR3 position);
    virtual ~Model();
};

class Pyramid : public Model
{
protected:
    const float radius;

    void Add3Indices(unsigned *ci, DWORD i1, DWORD i2, DWORD i3);
    DWORD AbsIndex(bool up, unsigned level, unsigned quarter, unsigned index);
    DWORD FindOrCreate(bool up, unsigned level, unsigned quarter, unsigned index, D3DXVECTOR3 v, D3DXVECTOR3 norm, DWORD color);
    void Tesselate(unsigned tesselation_level, DWORD color);

    virtual void SetShaderConstants(IDirect3DDevice9 *device);
    void Draw(IDirect3DDevice9 *device);
public:
    Pyramid(IDirect3DDevice9 *device, DWORD color, unsigned tesselation_level, const TCHAR *shader_file,
            D3DXVECTOR3 position = D3DXVECTOR3(), float radius = 1.0f);
};