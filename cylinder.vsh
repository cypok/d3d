vs_1_1

; v0 : vertex
; v1 : color
; v2.x : weight 1
; v2.y : weight 2
;
; c0-c3 : view and proj matrix
; c4-c7 : first bone
; c8-c10 : second bone


dcl_position v0             ; vertex
dcl_normal v1               ; normal
dcl_color v2                ; color
dcl_texcoord v3             ; weights

mov     oD0, v2

m4x4    r1, v0, c35
m4x4    r2, v0, c39
mul     r0, r1, v3.x
mad     r0, r2, v3.y, r0

m4x4    oPos, r0, c4