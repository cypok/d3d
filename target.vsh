vs_1_1


; _________________________________
; OBJECT PROPERTIES
; c40 : center pixel
; c41 : up pixel
; c42 : right pixel
; c43 : down pixel
; c44 : left pixel

dcl_position v0             ; vertex
dcl_color v2                ; color
dcl_texcoord v3             ; u,v

mov     oPos.xyzw, v0.yxzw
mov     oD0,  v2

add     oT0,  v3, c40
add     oT1,  v3, c41
add     oT2,  v3, c42
add     oT3,  v3, c43
add     oT4,  v3, c44