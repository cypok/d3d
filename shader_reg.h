#pragma once
#include "stdafx.h"

enum {
    // matrices
    EYE_REG = 3,
    VIEW_MATRIX_REG = 4,
    ROTATION_MATRIX_REG = 8,
    POSITION_MATRIX_REG = 12,

    // model
    TIME_REG = 32,
    SPECULAR_DEGRADATION_REG = 33,
    PYRAMID_RADIUS_REG = 34,
    BONE1_MATRIX_REG = 35,
    BONE2_MATRIX_REG = 39,

    // stuff
    SHADOW_MATRIX_REG = 48,
    SHADOW_COLOR_REG = 52,

    // light sources
    SCENE_COLOR_AMBIENT_REG = 64,

    DIRECTIONAL_VECTOR_REG = 65,
    DIRECTIONAL_COLOR_DIFFUSE_REG = 66,
    DIRECTIONAL_COLOR_SPECULAR_REG = 67,

    POINT_POSITION_REG = 68,
    POINT_COLOR_DIFFUSE_REG = 69,
    POINT_COLOR_SPECULAR_REG = 70,
    POINT_ATTENUATION_FACTOR_REG = 71,

    SPOT_POSITION_REG = 72,
    SPOT_VECTOR_REG = 73,
    SPOT_COLOR_DIFFUSE_REG = 74,
    SPOT_COLOR_SPECULAR_REG = 75,
    SPOT_ATTENUATION_FACTOR_REG = 76,
    SPOT_RANGE_FACTOR_REG = 77,

    ANISOTROPIC_DIFFUSE_COLORS_REG = 96,
    ANISOTROPIC_SPECULAR_COLORS_REG = 112,
};