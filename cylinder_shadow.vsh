vs_1_1

; _________________________________
; VIEW
; c3 : eye
; c4-c7  : view matrix
; c8-c11 : rotation matrix
; c12-c15 : position matrix
; _________________________________
; OBJECT PROPERTIES
; c35-c38 : first bone (B1)
; c39-c42 : second bone (B2)
; _________________________________
; STUFF
; c48 : shadow matrix
; c52 : shadow color

dcl_position v0             ; vertex
dcl_texcoord v3             ; v3.x and v3.y are weights

; some useful stuff
def c0, 0, 0, 0, 0
def c1, 1, 1, 1, 1

; _________________________________
; CALCULATING POSITION
; recieves: -
; returns: r8 (norm), r9 (vertex)

; we need to do this:
; 2. w1*[B1 x v0] + w2*[B2 x v0]
; 1. rotate
; 3. move

m4x4    r1, v0, c35         ; r1 = [B1 x v0]
m4x4    r2, v0, c39         ; r2 = [B2 x v0]
mul     r0, r1, v3.x        ; r0 = w1*[B1 x v0]
mad     r0, r2, v3.y, r0    ; r0 = w1*[B1 x v0] + w2*[B2 x v0]
m4x4    r9, r0, c8          ; Rotate( r0 ) ...
m4x4    r0, r9, c12         ; cylinder_moving( r0 ) ...
m4x4    r9, r0, c48         ; making shadow ...

rcp     r1, r9.w
mul     r9.xyz, r9.xyz, r1 
mov     r9.w, c1

m4x4    oPos, r9, c4

; set color
mov     oD0, c52