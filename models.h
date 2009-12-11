#pragma once
#include "stdafx.h"
#include "shader_reg.h"
#include "helper.h"

const unsigned BONES_COUNT = 2;
const D3DXCOLOR SHADOW_COLOR(0.0f, 0.0f, 0.05f, 1.0f);

const D3DVERTEXELEMENT9 VERTEX_ELEMENT[] =
{
    {0, 0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
    {0, 12, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_NORMAL, 0},
    {0, 24, D3DDECLTYPE_D3DCOLOR, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_COLOR, 0},
    {0, 28, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0},
    D3DDECL_END()
};

struct Vertex
{
    D3DXVECTOR3 v;
    D3DXVECTOR3 norm;
    DWORD color;
    FLOAT tu, tv;

    Vertex( D3DXVECTOR3 v = D3DXVECTOR3(),
            D3DXVECTOR3 norm = D3DXVECTOR3(1, 0, 0),
            DWORD color = BLACK,
            FLOAT tu=0, FLOAT tv=0) : v(v), norm(norm), color(color), tu(tu), tv(tv) {}
};

class Model
{
protected:
    IDirect3DDevice9 *device;
    IDirect3DVertexBuffer9 *vertex_buffer;
    IDirect3DIndexBuffer9 *index_buffer;
    IDirect3DVertexShader9 *vertex_shader;
    IDirect3DVertexDeclaration9 *vertex_declaration;
    IDirect3DTexture9 *texture;
    IDirect3DPixelShader9 *pixel_shader;

    void *vb;
    DWORD *ib;
    const unsigned vcount;
    const unsigned icount;

    static unsigned time;
    const float time_speed;

    const unsigned sizeof_vertex;
    const D3DVERTEXELEMENT9 *vertex_element;

    float angle_phi;
    D3DXVECTOR3 position;

    void InitVIB(IDirect3DDevice9 *device);
    void InitVDeclAndShader(IDirect3DDevice9 *device, const TCHAR *shader_file);
    void InitTextureAndPixelShader(IDirect3DDevice9 *device, const TCHAR *texture_file, const TCHAR *pixel_shader);
    void InitTextureAndPixelShader(IDirect3DDevice9 *device, unsigned width, unsigned height, const TCHAR *pixel_shader);

    virtual void SetShaderConstants(IDirect3DDevice9 *device);
    virtual void Draw(IDirect3DDevice9 *device) = 0;

public:
    Model(const unsigned sizeof_vertex, const D3DVERTEXELEMENT9 *vertex_element,
             const unsigned vcount, const unsigned icount,
             D3DXVECTOR3 position, float time_speed);
    virtual void Render(IDirect3DDevice9 *device);
    void SetRotation(float angle);
    void SetPosition(D3DXVECTOR3 position);
    static void SetTime(unsigned time);
    virtual ~Model();
private:
    // no copying
    Model(const Model &);
    Model & operator=(const Model &);
};

class ModelWithShadow : public Model
{
protected:
    D3DXMATRIX shadow_matrix;
    IDirect3DVertexShader9 *shadow_vertex_shader;

    void InitVDeclAndShader(IDirect3DDevice9 *device, const TCHAR * shader_file, const TCHAR * shadow_shader_file);

    virtual void SetShaderConstants(IDirect3DDevice9 *device);

public:
    ModelWithShadow(const unsigned sizeof_vertex, const D3DVERTEXELEMENT9 *vertex_element,
             const unsigned vcount, const unsigned icount,
             D3DXVECTOR3 position, float time_speed);
    virtual void Render(IDirect3DDevice9 *device);

    void SetShadowMatrix(D3DXMATRIX m);
    virtual ~ModelWithShadow();
};

class Pyramid : public ModelWithShadow
{
protected:
    const float radius;

    void Add3Indices(unsigned *ci, DWORD i1, DWORD i2, DWORD i3);
    DWORD AbsIndex(bool up, unsigned level, unsigned quarter, unsigned index);
    DWORD FindOrCreate(bool up, unsigned level, unsigned quarter, unsigned index, D3DXVECTOR3 v, D3DXVECTOR3 norm, DWORD color);
    void Tesselate(unsigned granularity, DWORD color);

    virtual void SetShaderConstants(IDirect3DDevice9 *device);
    virtual void Draw(IDirect3DDevice9 *device);
public:
    Pyramid(IDirect3DDevice9 *device, DWORD color, const TCHAR *shader_file, const TCHAR * shadow_shader_file,
            const TCHAR * texture_file, const TCHAR * pixel_shader_file,
            D3DXVECTOR3 position, float time_speed,
            unsigned granularity, float radius);
};

class Plane : public Model
{
protected:
    const D3DXVECTOR3 normal;
    const float size;

    void Tesselate(unsigned granularity, DWORD color);

    void Draw(IDirect3DDevice9 *device);

public:
    Plane(IDirect3DDevice9 *device, DWORD color, const TCHAR *shader_file,
          const TCHAR * texture_file, const TCHAR * pixel_shader_file,
          D3DXVECTOR3 position, D3DXVECTOR3 normal,
          unsigned granularity, float size);
};

class TargetPlane : public Model
{
protected:
    const D3DXVECTOR3 normal;

    void Tesselate(unsigned width, unsigned height);

    void Draw(IDirect3DDevice9 *device);

public:
    TargetPlane(IDirect3DDevice9 *device, const TCHAR *shader_file,
          const TCHAR * pixel_shader_file,
          unsigned width, unsigned height);

    virtual void Render(IDirect3DDevice9 *device);
    virtual void SetShaderConstants(IDirect3DDevice9 *device);

    IDirect3DSurface9 * GetSurface();
    void SaveTexture();
};