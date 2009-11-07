vs_1_1

; v0 : vertex
; v1 : color
; c0.x : 0 <= time <= 1 (t)
; c1.x : sphere radius (R)
; c2-c5 : matrix


dcl_position v0             ; vertex
dcl_normal v1               ; normal
def c6, 1, 0, 0, 0          ; c6 = 1

dcl_color v2                ; color

mov oD0, v2


; we need to do this:
; oPos = v0 * (1 + t*(R/|v0| - 1))

dp3     r0, v0, v0          ; now r0.x = v0^2
rsq     r0, r0              ; r0.x = 1/|v0|
mul     r0, r0, c1          ; r0.x = R/|v0|
sub     r0, r0, c6          ; r0.x = R/|v0| - 1
mul     r0, r0, c0          ; r0.x = t*(R/|v0| - 1)
add     r0, r0, c6          ; r0.x = 1 + t*(R/|v0| - 1)

mul     r1.xyz, v0.xyz, r0.x
mov     r1.w,   v0.w        ; r1 = v0 * (1 + t*(R/|v0| - 1))

m4x4    oPos, r1, c2