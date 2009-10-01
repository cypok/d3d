vs_1_1

; v0 : vertex
; v1 : color
; c0.x : 0 <= time <= 1 (t)
; c1.x : sphere radius (R)
; c2 : matrix

dcl_position    v0      ; vertex

dcl_color       v1      ; color
mov             oD0, v1 ; color w/o changes

; we need to do this:
; oPos = v0 * (1 + t*(R/|v0| - 1))

; 1/|v0|
mul     r0.x, v0.x, v0.x
mul     r0.y, v0.y, v0.y
mul     r0.z, v0.z, v0.z    ; now r0.i = v0.i^2
add     r0.x, r0.x, r0.y
add     r0.x, r0.x, r0.z    ; now r0.x = v0^2
rsq     r0.x, r0.x          ; r0.x = 1/|v0|

mul     r0.x, r0.x, c1.x    ; r0.x = R/|v0|

mov     r1, c0
sge     r1.x, r1.x, r1.x    ; r1.x = 1

sub     r0.x, r0.x, r1.x    ; r0.x = R/|v0| - 1

mul     r0.x, r0.x, c0.x    ; r0.x = t*(R/|v0| - 1)

add     r0.x, r0.x, r1.x    ; r0.x = 1 + t*(R/|v0| - 1)

mul     r1.x, v0.x, r0.x
mul     r1.y, v0.y, r0.x
mul     r1.z, v0.z, r0.x
mov     r1.w, v0.w          ; r1 = v0 * (1 + t*(R/|v0| - 1))

m4x4    oPos, r1 , c2        