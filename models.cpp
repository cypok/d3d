#include "stdafx.h"
#include "models.h"
#include "shader_reg.h"


const D3DXVECTOR3 INITIAL_PYRAMID[] = {       // it is generated for radius = sqrt(2)
    D3DXVECTOR3(  1.0f,  1.0f,  0.0f       ),
    D3DXVECTOR3(  1.0f, -1.0f,  0.0f       ),
    D3DXVECTOR3( -1.0f, -1.0f,  0.0f       ),
    D3DXVECTOR3( -1.0f,  1.0f,  0.0f       ),
    D3DXVECTOR3(  0.0f,  0.0f,  sqrtf(2.0) ),
    D3DXVECTOR3(  0.0f,  0.0f, -sqrtf(2.0) ),
};
const TCHAR PYRAMID_SHADER_FILE[] = _T("pyramid.vsh");
const unsigned INITIAL_PYRAMID_VCOUNT = sizeof(INITIAL_PYRAMID)/sizeof(INITIAL_PYRAMID[0]);


void Model::InitVIB(IDirect3DDevice9 *device)
{
    unsigned v_size = vcount*sizeof_vertex;
    unsigned i_size = icount*sizeof(DWORD);

    // initializing vertex and index buffers
    OK( device->CreateVertexBuffer(v_size, 0, 0, D3DPOOL_MANAGED, &vertex_buffer, NULL) );
    OK( device->CreateIndexBuffer(i_size, 0, D3DFMT_INDEX32, D3DPOOL_MANAGED, &index_buffer, NULL) );

    void * buffer = NULL;
    OK( vertex_buffer->Lock(0, 0, &buffer, 0) );
    memcpy(buffer, vb, v_size);
    vertex_buffer->Unlock();

    OK( index_buffer->Lock(0, 0, &buffer, 0) );
    memcpy(buffer, ib, i_size);
    index_buffer->Unlock();
}

void Model::InitVDeclAndShader(IDirect3DDevice9 *device)
{
    // initializing vertex declaration and shader

    OK( device->CreateVertexDeclaration(vertex_element, &vertex_declaration) );

    ID3DXBuffer *code = NULL;
    OK( D3DXAssembleShaderFromFile(shader_file, NULL, NULL, D3DXSHADER_DEBUG, &code, NULL) );
    OK( device->CreateVertexShader(static_cast<DWORD*>(code->GetBufferPointer()), &vertex_shader) );

    ReleaseInterface(code);
}

Model::~Model()
{
    delete[] vb;
    delete[] ib;
    ReleaseInterface(vertex_buffer);
    ReleaseInterface(index_buffer);
    ReleaseInterface(vertex_shader);
    ReleaseInterface(vertex_declaration);
}

void Model::Render(IDirect3DDevice9 *device)
{
    SetShaderConstants(device);
    OK( device->SetStreamSource(0, vertex_buffer, 0, sizeof_vertex) );
    OK( device->SetIndices(index_buffer) );
    OK( device->SetVertexDeclaration(vertex_declaration) );
    OK( device->SetVertexShader(vertex_shader) );
    Draw(device);
}

void Model::SetShaderConstants(IDirect3DDevice9 *device)
{
    D3DXMATRIX rotation(
        cosf(angle_phi),  -sinf(angle_phi), 0, 0,
        sinf(angle_phi),  cosf(angle_phi),  0, 0,
        0,                0,                1, 0,
        0,                0,                0, 1
    );
    D3DXMATRIX position(
        1.0f, 0.0f, 0.0f, position.x,
        0.0f, 1.0f, 0.0f, position.y,
        0.0f, 0.0f, 1.0f, position.z,
        0.0f, 0.0f, 0.0f, 1.0f
    );

    OK( device->SetVertexShaderConstantF(ROTATION_MATRIX_REG, rotation, WORLD_DIMENSION + 1) );
    OK( device->SetVertexShaderConstantF(POSITION_MATRIX_REG, position, WORLD_DIMENSION + 1) );
}

void Model::SetRotation(float angle)
{
    angle_phi = angle;
}

void Model::SetPosition(D3DXVECTOR3 position)
{
    this->position = position;
}

Pyramid::Pyramid(IDirect3DDevice9 *device, DWORD color, unsigned tesselation_level, const TCHAR *shader_file,
                 D3DXVECTOR3 position, float radius)
{
    this->position = position;
    this->radius = radius;

    sizeof_vertex = sizeof(Vertex);
    vertex_element = const_cast<D3DVERTEXELEMENT9 *>(PYRAMID_VERTEX_ELEMENT);
    _tcscpy_s(this->shader_file, MAX_SHADER_FILENAME_LENGTH, shader_file);

    vcount = 4 * (tesselation_level + 1) * (tesselation_level + 2);
    icount = 8 * 3 * tesselation_level * tesselation_level;
    vb = new Vertex[vcount];
    ib = new DWORD[icount];

    Tesselate(tesselation_level, color);

    InitVIB(device);
    InitVDeclAndShader(device);
}

void Pyramid::SetShaderConstants(IDirect3DDevice9 *device)
{
    Model::SetShaderConstants(device);

    D3DXVECTOR4 v;
    v.x = v.y = v.z = v.w = radius;
    OK( device->SetVertexShaderConstantF(PYRAMID_RADIUS_REG, v, 1) );
}

void Pyramid::Draw(IDirect3DDevice9 *device)
{
    OK( device->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, 0, vcount, 0, icount/3) );
}

// PYRAMID TESSELATION ----------------------------------------------------------------------------------
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
    if (vb[abs_index].color == 0) // unitialized vertex
        vb[abs_index] = Vertex(v,
                               norm,
                               color ? color : ALL_COLORS[(up?0:1)*4+quarter]);
    return abs_index;
}

void Pyramid::Tesselate(unsigned tesselation_level, DWORD color)
{
    unsigned ci = 0; // current index

    memset(vb, 0, vcount * sizeof_vertex);

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
        D3DXVECTOR3 top = INITIAL_PYRAMID[sides[j][0]]*radius/sqrtf(2);
        D3DXVECTOR3 to_left = (INITIAL_PYRAMID[sides[j][1]]*radius/sqrtf(2) - top)/tesselation_level;
        D3DXVECTOR3 to_right = (INITIAL_PYRAMID[sides[j][2]]*radius/sqrtf(2) - top)/tesselation_level;
        D3DXVECTOR3 norm;
        D3DXVec3Cross(&norm, &to_right, &to_left);
        D3DXVec3Normalize(&norm, &norm);

        FindOrCreate(up, 0, q, 0, top, norm, color);

        for (unsigned l = 1; l <= tesselation_level; ++l)
        {
            unsigned a = AbsIndex(up, l-1, q, 0);
            FindOrCreate(up, l, q, 0, vb[AbsIndex(up, l-1, q, 0)].v + to_left, norm, color);
            for (unsigned i = 1; i < l; ++i)
            {
                a = AbsIndex(up, l-1, q, i-1);
                FindOrCreate(up, l, q, i, vb[AbsIndex(up, l-1, q, i-1)].v + to_right, norm, color);
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
            FindOrCreate(up, l, q, l, vb[AbsIndex(up, l-1, q, l-1)].v + to_right, norm, color);
            Add3Indices(&ci,
                AbsIndex(up, l, q, l),
                AbsIndex(up, l, q, l-1),
                AbsIndex(up, l-1, q, l-1));
        }
    }
}