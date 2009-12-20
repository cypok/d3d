vs_1_1

; _________________________________
; VIEW
; c3 : eye
; c4-c7  : view matrix
; c8-c11 : rotation matrix
; c12-c15 : position matrix
; _________________________________
; OBJECT PROPERTIES
; c32 : 0 <= time <= 1 (t)
; c33 : 1 / specular degradation (f)
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
;

dcl_position    v0          ; vertex
dcl_tangent     v1          ; tangent
dcl_binormal    v2          ; binormal
dcl_normal      v3          ; normal
dcl_texcoord    v4          ; u,v

; some useful stuff
def c0, 0, 0, 0, 0
def c1, 1, 1, 1, 1


; _________________________________
; CALCULATING POSITION
; recieves: -
; returns: r8 (norm), r9 (vertex)

mov     r8, v3              ; r8 = normal
m4x4    r9, v0, c12         ; r9 = moving( r9 )
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

mov     oT0, v4             ; t0 = (u,v)
m3x3    oT2, r10, v1        ; t2 = G(L)
m3x3    oT3, r11, v1        ; t3 = G(eye)