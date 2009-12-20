vs_1_1

; _________________________________
; VIEW
; c3 : eye
; c4-c7  : view matrix
; c8-c11 : pyramid rotation matrix
; c12-c15 : pyramid position matrix
; _________________________________
; OBJECT PROPERTIES
; c32 : 0 <= time <= 1 (t)
; c33 : 1 / specular degradation (f)
; c34 : sphere radius (R)
; _________________________________
; LIGHT SOURCES
; > SCENE
;   c64 : ambient color (Ia)
;
; > POINT
;   c68 : position (P)
;   c69 : diffuse color (Id)
;   c70 : specular color (Is)
;   c71 : attenuation (a, b, c)

dcl_position v0             ; vertex
dcl_normal v1               ; normal
dcl_color v2                ; color
dcl_texcoord v3             ; u,v

; some useful stuff
def c0, 0, 0, 0, 0
def c1, 1, 1, 1, 1

; _________________________________
; CALCULATING POSITION
; recieves: -
; returns: r8 (norm), r9 (vertex)

; we need to do this:
; 1. rotate
; 2. v0 * (1 + t*(R/|v0| - 1))
; 3. move

; v1 - initial normal (inorm)
; v0/|v0| - final normal (fnorm)

m4x4    r1, v0, c8          ; r1 = Rotate( v0 )
 m4x4   r8, v1, c8          ; r8 = Rotate( norm ) = inorm
dp3     r0, r1, r1          ; r0 = Rotate(v0)^2
sge     r2, r0, r0          ; r2 = 1
rsq     r0, r0              ; r0.x = 1/|v0|
 mul    r3, r1, r0          ; r3 = v0/|v0| = fnorm
mad     r0, r0, c34, -r2    ; r0.x = R/|v0| - 1
mad     r0, r0, c32, r2     ; r0.x = 1 + t*(R/|v0| - 1)
 sub    r2, r3, r8          ; r2 = fnorm - inorm
 mad    r8, r2, c32, r8     ; r8 = (norm) = inorm + t*(fnorm - inorm)
 dp3    r2, r8, r8          ; r2 = |norm|^2
 rsq    r2, r2              ; r2 = 1/|norm|
 mul    r8, r8, r2          ; norm = norm/|norm|

mul     r1.xyz, r1.xyz, r0  ; r1 = v0 * (1 + t*(R/|v0| - 1))
m4x4    r9, r1, c12         ; r9 = Pyramid_moving( r9 )
m4x4    oPos, r9, c4


; _________________________________
; PREPARATION FOR PS
; receives: r8 (norm), r9 (vertex)

sub     r0, c68, r9         ; get and normalize vector to light source
dp3     r1, r0, r0
rsq     r1, r1
mul     r10, r0, r1         ; r10 BUSY

sub     r0, c3, r9          ; get and normalize vector to eye
dp3     r1, r0, r0
rsq     r1, r1
mul     r11, r0, r1         ; r11 BUSY

mov     r0.xyz, r8          ; r0 = (cosf*sint, sinf*sint, cost, ?)
mad     r0.w, r0.z, -r0.z, c1.x ; r0.w = 1 - cost^2
rsq     r1.w, r0.w          ; r1.w = 1/sint
rcp     r0.w, r1.w          ; r0.w = sint
mul     r0.xy, r0.xy, r1.w  ; r0.xy = (cosf, sinf)

; r0 = (cosf, sinf, cost, sint)

;       |   -sinf       cosf        0       |
;  G =  |   -cosf*cost  -sinf*cost  sint    |
;       |   cosf*sint   sinf*sint   cost    |

mov     r5.x, -r0.y
mov     r5.y, r0.x
mov     r5.z, c0
mul     r6.x, -r0.x, r0.z
mul     r6.y, -r0.y, r0.z
mov     r6.z, r0.w
mul     r7.x, r0.x, r0.w
mul     r7.y, r0.y, r0.w
mov     r7.z, r0.z

    ; in future it would be unnecessery
    mov r8.xy, c0
    mov r8.z, c1

mov     oT0, v3             ; t0 = (u,v)
m3x3    oT1, r10, r5        ; t1 = G(L)
mov     oT2, r8             ; t2 = (0,0,1)
m3x3    oT3, r11, r5        ; t3 = G(eye)