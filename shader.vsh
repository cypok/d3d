vs_1_1

; v0 : vertex
; v1 : color (C)

; _________________________________
; VIEW
; c3 : eye
; c4-c7  : view matrix
; c8-c11 : pyramid rotation matrix
; c12-c15 : pyramid position matrix
; _________________________________
; OBJECT PROPERTIES
; c32 : 1 / specular degradation (f)
; c33 : 0 <= time <= 1 (t)
; c34 : sphere radius (R)
; _________________________________
; LIGHT SOURCES
; > SCENE
;   c64 : ambient color (Ia)
;
; > DIRECTIONAL
;   c65 : vector (-L)
;   c66 : diffuse color (Id)
;   c67 : specular color (Is)
; 
; > POINT
;   c68 : position (P)
;   c69 : diffuse color (Id)
;   c70 : specular color (Is)
; 
; > SPOT


dcl_position v0             ; vertex
dcl_normal v1               ; normal
dcl_color v2                ; color

; some useful stuff
def c0, 0, 0, 0, 0
def c1, 1, 1, 1, 1
def c2, 2, 2, 2, 2

; _________________________________
; CALCULATING POSITION
; recieves: -
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
mad     r0, r0, c34, -r2    ; r0.x = R/|v0| - 1
mad     r0, r0, c33, r2     ; r0.x = 1 + t*(R/|v0| - 1)
 sub    r2, r3, r8          ; r2 = fnorm - inorm
 mad    r8, r2, c33, r8     ; r8 = (norm) = inorm + t*(fnorm - inorm)
 dp3    r2, r8, r8          ; r2 = |norm|^2
 rsq    r2, r2              ; r2 = 1/|norm|
 mul    r8, r8, r2          ; norm = norm/|norm|

mul     r1.xyz, r1.xyz, r0  ; r1 = v0 * (1 + t*(R/|v0| - 1))
m4x4    r9, r1, c12         ; r9 = Pyramid_moving( r9 )
m4x4    oPos, r9, c4

; _________________________________
; C O L O R   C O L O R   C O L O R
; recieves: r7 (eye - v), r8 (norm), r9 (vertex)
; returns: r10 (color)

; calculating (eye - v)/|eye-v|
    sub     r7, c3, r9          ; r2 = eye - v
    dp3     r0, r7, r7          ; normalizing...
    rsq     r0, r0              ; ...
    mul     r7, r7, r0          ; r2 = (eye - v)/|eye-v|

; ambient
    mul     r10, v2, c64        ; r0 = C * Ia

; _________________________________
; CALCULATING DIRECTIONAL COLOR
; recieves: r7 (eye - v), r8 (norm), r9 (vertex), r10 (color)
; returns: r10 (color)

    ; diffuse
        dp3     r1, r8, -c65        ; r1 = (norm, L)
        sge     r3, r1, c0          ; r3 = r1 >= 0
        mul     r1, r1, r3          ; now r1 >= 0
        mul     r0, v2, c66         ; r0 = C * Id
        mul     r2, r0, r1          ; r2 = C * Id * (norm, L)

    add     r10, r10, r2        ; r10 = ambient + diffuse

    ; specular
        dp3     r1, r8, -c65        ; r1 = (norm, L)
        mul     r1, r1, c2          ; r1 = 2*(norm, L)
        mad     r1, r1, r8, c65     ; r1 = 2*(norm, L)*norm - L
        dp3     r1, r7, r1          ; r1 = ( eye-v, 2*(norm, L)*norm - L )

        mov     r1.w, c32           ; powering and checking that it's > 0
        lit     r1, r1              ; r1.z = r1^f

        mul     r0, v2, c67         ; r0 = C * Is
        mul     r2, r0, r1.z        ; r2 = C * Is * ( eye-v, 2*(norm, L)*norm - L )

    add     r10, r10, r2        ; color = ambient + diffuse + specular

; _________________________________
; CALCULATING POINT COLOR
; recieves: r7 (eye - v), r8 (norm), r9 (vertex), r10 (color)
; returns: r10 (color)

    sub     r6, c68, r9         ; r6 = L = PointSource - Vertex
    dp3     r0, r6, r6          ; normalizing...
    rsq     r0, r0              ; ...
    mul     r6, r6, r0          ; r6 = L/|L|
    

    ; diffuse
        dp3     r1, r8, r6          ; r1 = (norm, L)
        sge     r3, r1, c0          ; r3 = r1 >= 0
        mul     r1, r1, r3          ; now r1 >= 0
        mul     r0, v2, c69         ; r0 = C * Id
        mul     r2, r0, r1          ; r2 = C * Id * (norm, L)

    add     r10, r10, r2        ; r10 = ambient + diffuse

    ; specular
        dp3     r1, r8, r6          ; r1 = (norm, L)
        mul     r1, r1, c2          ; r1 = 2*(norm, L)
        mad     r1, r1, r8, -r6     ; r1 = 2*(norm, L)*norm - L
        dp3     r1, r7, r1          ; r1 = ( eye-v, 2*(norm, L)*norm - L )

        mov     r1.w, c32           ; powering and checking that it's > 0
        lit     r1, r1              ; r1.z = r1^f

        mul     r0, v2, c70         ; r0 = C * Is
        mul     r2, r0, r1.z        ; r2 = C * Is * ( eye-v, 2*(norm, L)*norm - L )

    add     r10, r10, r2        ; color = ambient + diffuse + specular

; set color
mov     oD0, r10