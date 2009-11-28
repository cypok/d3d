#include "stdafx.h"
#include "models.h"

Cylinder::Cylinder(IDirect3DDevice9 *device, DWORD color, const TCHAR *shader_file, const TCHAR * shadow_shader_file,
                   D3DXVECTOR3 position, float time_speed,
                   unsigned vertical_granularity, unsigned horizontal_granularity,
                   float height, float radius,
                   float rotation_angle) :
        ModelWithShadow(sizeof(VertexWithWeights), VERTEX_WITH_WEIGHTS_ELEMENT,
            (vertical_granularity+1)*horizontal_granularity,
            2*(horizontal_granularity+1)*vertical_granularity,
            position, time_speed),
        height(height), radius(radius), rotation_angle(rotation_angle)
{
    Tesselate(vertical_granularity, horizontal_granularity, color);

    InitVIB(device);
    InitVDeclAndShader(device, shader_file, shadow_shader_file);
}

void Cylinder::SetShaderConstants(IDirect3DDevice9 *device)
{
    ModelWithShadow::SetShaderConstants(device);

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
