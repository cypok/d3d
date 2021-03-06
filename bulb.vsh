vs_1_1

; _________________________________
; VIEW
; c3 : eye
; c4-c7  : view matrix
; c8-c11 : pyramid rotation matrix
; c12-c15 : pyramid position matrix
; _________________________________
; OBJECT PROPERTIES
; c34 : sphere radius (R)
; _________________________________
; LIGHT SOURCES
; > POINT
;   c69 : diffuse color (Id)

dcl_position v0             ; vertex

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
dp3     r0, r1, r1          ; r0 = Rotate(v0)^2
rsq     r0, r0              ; r0.x = 1/|v0|
mul     r0, r0, c34         ; r0.x = R/|v0|

mul     r1.xyz, r1.xyz, r0.x; r1 = v0 * (1 + t*(R/|v0| - 1))
m4x4    r9, r1, c12         ; r9 = Pyramid_moving( r9 )
m4x4    oPos, r9, c4

; set color
mov     oD0, c69            ; color = WHITE