vs_1_1

; v0 : vertex
; v1 : color (C)

; _________________________________
; VIEW
; c3 : eye
; c4-c7  : view matrix
; c8-c11 : pyramid rotation matrix
; _________________________________
; MORPHING  CONSTANTS
; c32.x : 0 <= time <= 1 (t)
; c33.x : sphere radius (R)
; _________________________________
; LIGHT SOURCES
; > DIRECTIONAL
;    c64 : vector (L)
;    c65 : diffuse color (Id)
;    c66 : specular color (Is)
;    c67 : ambient color (Ia)
;    c68 : 1 / specular degradation (f)
; 
; > POINT
; 
; > SPOT


dcl_position v0             ; vertex
dcl_normal v1               ; normal
dcl_color v2                ; color

def c0, 0, 0, 0, 0
def c1, 1, 1, 1, 1
def c2, 2, 2, 2, 2

; _________________________________
; CALCULATING POSITION
; gets: -
; using: r0, r1, r2, r3
; returns: r8 (norm), r9 (vertex)

; we need to do this:
; oPos = v0 * (1 + t*(R/|v0| - 1))
; v1 - initial normal (inorm)
; v0/|v0| - final normal (fnorm)

m4x4    r1, v0, c8          ; r1 = Rotate( v0 )
 m4x4   r8, v1, c8          ; r8 = Rotate( norm ) = inorm
dp3     r0, r1, r1          ; r0 = Rotate(v0)^2
sge     r2, r0, r0          ; r2 = 1
rsq     r0, r0              ; r0.x = 1/|v0|
 mul    r3, r1, r0          ; r3 = v0/|v0| = fnorm
mad     r0, r0, c33, -r2    ; r0.x = R/|v0| - 1
mad     r0, r0, c32, r2     ; r0.x = 1 + t*(R/|v0| - 1)
 sub    r2, r3, r8          ; r2 = fnorm - inorm
 mad    r8, r2, c32, r8     ; r8 = (norm) = inorm + t*(fnorm - inorm)
 dp3    r2, r8, r8          ; r2 = |norm|^2
 rsq    r2, r2              ; r2 = 1/|norm|
 mul    r8, r8, r2          ; norm = norm/|norm|

mul     r9.xyz, r1.xyz, r0.x
mov     r9.w, r1.w          ; r2 = v0 * (1 + t*(R/|v0| - 1))
m4x4    oPos, r9, c4

; _________________________________
; CALCULATING POINT COLOR
; gets: r8 (norm), r9 (vertex)
; using: r0, r1, r2, r3, r4
; returns: r10 (point color)

; diffuse
dp3     r1, r8, -c64        ; r1 = (norm, -L)
sge     r3, r1, c0          ; r3 = r1 >= 0
mul     r1, r1, r3          ; now r1 >= 0
mul     r0, v2, c65         ; r0 = C * Id
mul     r10, r0, r1         ; r10 = C * Id * (norm, -L)

; specular
sub     r2, c3, r9          ; r2 = eye - v
dp3     r1, r2, r2          ; normalizing...
rsq     r1, r1              ; ...
mul     r2, r2, r1          ; r2 = (eye - v)/|eye-v|

dp3     r1, r8, -c64        ; r1 = (norm, -L)
mul     r1, r1, c2          ; r1 = 2*(norm, -L)
mad     r1, r1, r8, c64     ; r1 = 2*(norm, -L)*norm - (-L)
dp3     r1, r2, r1          ; r1 = ( eye-v, 2*(norm, -L)*norm - (-L) )
sge     r4, r1, c0          ; r4 = r1 >= 0
mul     r1, r1, r4          ; now r1 >= 0

mov     r1.x, c1.x          ; powering...
mov     r1.w, c68           ; ...
lit     r1, r1              ; r1.z = r1^f

mul     r0, v2, c66         ; r0 = C * Is
mul     r2, r0, r1.z        ; r2 = C * Is * ( eye-v, 2*(norm, -L)*norm - (-L) )


; resulting color
add     r10, r10, r2        ; point color = diffuse + specular
mov     oD0, r10