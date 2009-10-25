vs_1_1

; v0 : vertex
; v1 : color
; v2.x : weight 1
; v2.y : weight 2
;
; c0-c3 : view and proj matrix
; c4-c7 : first bone
; c8-c10 : second bone


dcl_position v0    ; vertex

dcl_color v1       ; color

dcl_texcoord v2   ; weights

mov     oD0, v1
m4x4    r1, v0, c4
m4x4    r2, v0, c8
mul     r0, r1, v2.x
mad     r0, r2, v2.y, r0

m4x4    oPos, r0,  c0