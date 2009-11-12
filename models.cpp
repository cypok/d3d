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

void Model::InitVDeclAndShader(IDirect3DDevice9 *device)
{
    // initializing vertex declaration and shader

    OK( device->CreateVertexDeclaration(vertex_element, &vertex_declaration) );

    ID3DXBuffer *code = NULL;
    OK( D3DXAssembleShaderFromFile(shader_file, NULL, NULL, D3DXSHADER_DEBUG, &code, NULL) );
    OK( device->CreateVertexShader(static_cast<DWORD*>(code->GetBufferPointer()), &vertex_shader) );

    ReleaseInterface(code);
}

Model::Model(const unsigned sizeof_vertex, const D3DVERTEXELEMENT9 *vertex_element,
             const TCHAR * shader_file, const unsigned vcount, const unsigned icount,
             D3DXVECTOR3 position, float time_speed) : 
        sizeof_vertex(sizeof_vertex),
        vertex_element(vertex_element),
        shader_file(shader_file),
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

Pyramid::Pyramid(IDirect3DDevice9 *device, DWORD color, const TCHAR *shader_file,
                 D3DXVECTOR3 position, float time_speed,
                 unsigned granularity, float radius) :
        Model(sizeof(Vertex), VERTEX_ELEMENT, shader_file,
            4 * (granularity + 1) * (granularity + 2),
            8 * 3 * granularity * granularity,
            position, time_speed),
        radius(radius)
{
    Tesselate(granularity, color);

    InitVIB(device);
    InitVDeclAndShader(device);
}

void Pyramid::SetShaderConstants(IDirect3DDevice9 *device)
{
    Model::SetShaderConstants(device);

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
        vertices[abs_index] = Vertex(v,
                               norm,
                               color ? color : ALL_COLORS[(up?0:1)*4+quarter]);
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
        D3DXVECTOR3 top = INITIAL_PYRAMID[sides[j][0]]*radius/sqrtf(2);
        D3DXVECTOR3 to_left = (INITIAL_PYRAMID[sides[j][1]]*radius/sqrtf(2) - top)/static_cast<float>(granularity);
        D3DXVECTOR3 to_right = (INITIAL_PYRAMID[sides[j][2]]*radius/sqrtf(2) - top)/static_cast<float>(granularity);
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

Cylinder::Cylinder(IDirect3DDevice9 *device, DWORD color, const TCHAR *shader_file,
                   D3DXVECTOR3 position, float time_speed,
                   unsigned vertical_granularity, unsigned horizontal_granularity,
                   float height, float radius,
                   float rotation_angle) :
        Model(sizeof(VertexWithWeights), VERTEX_WITH_WEIGHTS_ELEMENT, shader_file,
            (vertical_granularity+1)*horizontal_granularity,
            2*(horizontal_granularity+1)*vertical_granularity,
            position, time_speed),
        height(height), radius(radius), rotation_angle(rotation_angle)
{
    Tesselate(vertical_granularity, horizontal_granularity, color);

    InitVIB(device);
    InitVDeclAndShader(device);
}

void Cylinder::SetShaderConstants(IDirect3DDevice9 *device)
{
    Model::SetShaderConstants(device);

    D3DXMATRIX static_bone(
        1, 0, 0, 0,
        0, 1, 0, 0,
        0, 0, 1, 0,
        0, 0, 0, 1
    );
    OK( device->SetVertexShaderConstantF(BONE1_MATRIX_REG, static_bone, WORLD_DIMENSION + 1) );

    float alpha = rotation_angle*sinf( static_cast<float>(time)*time_speed );
    D3DXMATRIX rotation_bone(
        cosf(alpha), 0, -sinf(alpha), 0,
        0, 1, 0, 0,
        sinf(alpha), 0, cosf(alpha), 0,
        0, 0, 0, 1
    );
    OK( device->SetVertexShaderConstantF(BONE2_MATRIX_REG, rotation_bone, WORLD_DIMENSION + 1) );
}

void Cylinder::Draw(IDirect3DDevice9 *device)
{
    OK( device->DrawIndexedPrimitive(D3DPT_TRIANGLESTRIP, 0, 0, vcount, 0, icount-2) );
}

void Cylinder::Tesselate(unsigned int vertical_granularity, unsigned int horizontal_granularity, DWORD color)
{
    float delta_z = height/vertical_granularity;
    float delta_phi = 2*D3DX_PI/horizontal_granularity;

    VertexWithWeights *vertices = reinterpret_cast<VertexWithWeights*>(vb);

    unsigned ci = 0; // current index index for adding

    for (unsigned i = 0; i < vertical_granularity; ++i)
    {
        for (unsigned j = 0; j < horizontal_granularity+1; ++j)
        {
            unsigned shift = 0;
            shift = i*horizontal_granularity;
            if (j < horizontal_granularity)
            {
                shift += j;

                vertices[shift] = VertexWithWeights(D3DXVECTOR3(radius*cosf(delta_phi*j),
                                                                radius*sinf(delta_phi*j),
                                                                delta_z*i),
                                                    D3DXVECTOR3(cosf(delta_phi*j),
                                                                sinf(delta_phi*j),
                                                                0.0f),
                                                    color,
                                                    1.0f - static_cast<float>(i)/vertical_granularity);
                if (i + 1 == vertical_granularity)
                {
                    vertices[shift+horizontal_granularity] = VertexWithWeights(D3DXVECTOR3(radius*cosf(delta_phi*j),
                                                                                           radius*sinf(delta_phi*j),
                                                                                           delta_z*(i+1)),
                                                                               D3DXVECTOR3(cosf(delta_phi*j),
                                                                                           sinf(delta_phi*j),
                                                                                           0.0f),
                                                                               color,
                                                                               1.0f - static_cast<float>(i+1)/vertical_granularity);
                }
            }

            ib[ci++] = shift;
            ib[ci++] = shift+horizontal_granularity;
        }

    }
}

Plane::Plane(IDirect3DDevice9 *device, DWORD color, const TCHAR *shader_file,
             D3DXVECTOR3 position, D3DXVECTOR3 normal,
             unsigned granularity, float size) :
        Model(sizeof(Vertex), VERTEX_ELEMENT, shader_file,
            (granularity + 1) * (granularity + 1),
            6 * granularity * granularity,
            position, 0.0f),
        normal(normal), size(size)
{
    Tesselate(granularity, color);

    InitVIB(device);
    InitVDeclAndShader(device);
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
            vertices[i*(granularity+1)+j] = Vertex(start + j*u + i*v, normal, color);
            if (i == granularity-1)
                vertices[(i+1)*(granularity+1)+j] = Vertex(start + j*u + (i+1)*v, normal, color);

            if (j > 0)
            {
                ib[ci++] = (i+0)*(granularity+1)+(j-1);
                ib[ci++] = (i+1)*(granularity+1)+(j-1);
                ib[ci++] = (i+0)*(granularity+1)+(j-0);

                ib[ci++] = (i+1)*(granularity+1)+(j-1);
                ib[ci++] = (i+1)*(granularity+1)+(j-0);
                ib[ci++] = (i+0)*(granularity+1)+(j-0);
            }
        }
}

void Plane::Draw(IDirect3DDevice9 *device)
{
    OK( device->DrawIndexedPrimitive(D3DPT_TRIANGLELIST, 0, 0, vcount, 0, icount/3) );
}
