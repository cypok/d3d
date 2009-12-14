#include "stdafx.h"
#include "models.h"
#include "shader_reg.h"

unsigned Model::time = 0;

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

void Model::InitVDeclAndShader(IDirect3DDevice9 *device, const TCHAR * shader_file)
{
    OK( device->CreateVertexDeclaration(vertex_element, &vertex_declaration) );

    ID3DXBuffer *code = NULL;
    OK( D3DXAssembleShaderFromFile(shader_file, NULL, NULL, D3DXSHADER_DEBUG, &code, NULL) );
    OK( device->CreateVertexShader(static_cast<DWORD*>(code->GetBufferPointer()), &vertex_shader) );

    ReleaseInterface(code);
}

void Model::InitTextureAndPixelShader(IDirect3DDevice9 *device, const TCHAR *texture_file, const TCHAR *pixel_shader_file)
{
    if (texture_file)
        D3DXCreateTextureFromFile(device, texture_file, &texture);
    if (pixel_shader_file)
    {
        ID3DXBuffer *code = NULL;
        OK( D3DXAssembleShaderFromFile(pixel_shader_file, NULL, NULL, D3DXSHADER_DEBUG, &code, NULL) );
        OK( device->CreatePixelShader(static_cast<DWORD*>(code->GetBufferPointer()), &pixel_shader) );

        ReleaseInterface(code);
    }
}

Model::Model(const unsigned sizeof_vertex, const D3DVERTEXELEMENT9 *vertex_element,
             const unsigned vcount, const unsigned icount,
             D3DXVECTOR3 position, float time_speed) : 
        sizeof_vertex(sizeof_vertex),
        vertex_element(vertex_element),
        vcount(vcount), icount(icount),
        position(position), time_speed(time_speed)
{
    vb = new char[sizeof_vertex*vcount];
    ib = new DWORD[icount];
    angle_phi = 0;

    device = NULL;
    vertex_buffer = NULL;
    index_buffer = NULL;
    vertex_shader = NULL;
    vertex_declaration = NULL;
    texture = NULL;
    pixel_shader = NULL;
}

Model::~Model()
{
    delete[] vb;
    delete[] ib;
    ReleaseInterface(vertex_buffer);
    ReleaseInterface(index_buffer);
    ReleaseInterface(vertex_shader);
    ReleaseInterface(vertex_declaration);
    ReleaseInterface(texture);
    ReleaseInterface(pixel_shader);
}

void Model::Render(IDirect3DDevice9 *device)
{
    SetShaderConstants(device);
    OK( device->SetStreamSource(0, vertex_buffer, 0, sizeof_vertex) );
    OK( device->SetIndices(index_buffer) );
    OK( device->SetVertexDeclaration(vertex_declaration) );
    OK( device->SetVertexShader(vertex_shader) );
    OK( device->SetTexture(0, texture) ); // it could be NULL
    OK( device->SetPixelShader(pixel_shader) ); // it could be NULL
    Draw(device);
}

void Model::SetShaderConstants(IDirect3DDevice9 *device)
{
    D3DXMATRIX rotation_matrix(
        cosf(angle_phi),  -sinf(angle_phi), 0, 0,
        sinf(angle_phi),  cosf(angle_phi),  0, 0,
        0,                0,                1, 0,
        0,                0,                0, 1
    );
    OK( device->SetVertexShaderConstantF(ROTATION_MATRIX_REG, rotation_matrix, WORLD_DIMENSION + 1) );

    D3DXMATRIX position_matrix(
        1.0f, 0.0f, 0.0f, position.x,
        0.0f, 1.0f, 0.0f, position.y,
        0.0f, 0.0f, 1.0f, position.z,
        0.0f, 0.0f, 0.0f, 1.0f
    );
    OK( device->SetVertexShaderConstantF(POSITION_MATRIX_REG, position_matrix, WORLD_DIMENSION + 1) );
}

void Model::SetRotation(float angle)
{
    angle_phi = angle;
}

void Model::SetPosition(D3DXVECTOR3 position)
{
    this->position = position;
}

void Model::SetTime(unsigned time)
{
    Model::time = time;
}

ModelWithShadow::ModelWithShadow(const unsigned int sizeof_vertex, const D3DVERTEXELEMENT9 *vertex_element,
                                 const unsigned int vcount, const unsigned int icount,
                                 D3DXVECTOR3 position, float time_speed) :
                    Model(sizeof_vertex, vertex_element, vcount, icount, position, time_speed)
{
    shadow_vertex_shader = NULL;
}

void ModelWithShadow::SetShadowMatrix(D3DXMATRIX m)
{
    shadow_matrix = m;
}

void ModelWithShadow::SetShaderConstants(IDirect3DDevice9 *device)
{
    Model::SetShaderConstants(device);

    OK( device->SetVertexShaderConstantF(SHADOW_MATRIX_REG, shadow_matrix, WORLD_DIMENSION + 1) );
    OK( device->SetVertexShaderConstantF(SHADOW_COLOR_REG, SHADOW_COLOR, 1) );
}

void ModelWithShadow::InitVDeclAndShader(IDirect3DDevice9 *device, const TCHAR * shader_file, const TCHAR * shadow_shader_file)
{
    OK( device->CreateVertexDeclaration(vertex_element, &vertex_declaration) );

    ID3DXBuffer *code = NULL;

    OK( D3DXAssembleShaderFromFile(shader_file, NULL, NULL, D3DXSHADER_DEBUG, &code, NULL) );
    OK( device->CreateVertexShader(static_cast<DWORD*>(code->GetBufferPointer()), &vertex_shader) );

    if(shadow_shader_file != NULL)
    {
        OK( D3DXAssembleShaderFromFile(shadow_shader_file, NULL, NULL, D3DXSHADER_DEBUG, &code, NULL) );
        OK( device->CreateVertexShader(static_cast<DWORD*>(code->GetBufferPointer()), &shadow_vertex_shader) );
    }

    ReleaseInterface(code);
}

void ModelWithShadow::Render(IDirect3DDevice9 *device)
{
    Model::Render(device);

    if(shadow_vertex_shader != NULL)
    {
        OK( device->SetRenderState(D3DRS_STENCILFUNC, D3DCMP_EQUAL) );
        OK( device->SetRenderState(D3DRS_ZFUNC, D3DCMP_ALWAYS) );
        OK( device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA) );
        OK( device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA) );
        OK( device->SetVertexShader(shadow_vertex_shader) );
        OK( device->SetTexture(0, NULL) );
        OK( device->SetPixelShader(NULL) );
        Draw(device);
        OK( device->SetRenderState(D3DRS_STENCILFUNC, D3DCMP_ALWAYS) );
        OK( device->SetRenderState(D3DRS_ZFUNC, D3DCMP_LESSEQUAL) );
        OK( device->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_ONE) );
        OK( device->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ZERO) );
    }
}

ModelWithShadow::~ModelWithShadow()
{
    ReleaseInterface(shadow_vertex_shader);
}