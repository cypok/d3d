vs_1_1

dcl_position v0             ; vertex
dcl_color v2                ; color
dcl_texcoord v3             ; u,v

mov     oPos.xyzw, v0.yxzw
mov     oD0,  v2
mov     oT0,  v3