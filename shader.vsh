vs_1_1

; v0 : vertex
; v1 : color (C)

; _________________________________
; MATRICES
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
;    c65 : color (I)
; 
; > POINT
; 
; > SPOT


dcl_position v0             ; vertex
dcl_normal v1               ; normal
dcl_color v2                ; color

; _________________________________
; CALCULATING POSITION
; gets: -
; using: r0, r1, r2, r3
; returns: r8 (norm)

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
 sub    r2, r3, r8			; r2 = fnorm - inorm
 mad    r8, r2, c32, r8     ; r8 = (norm) = inorm + t*(fnorm - inorm)
 dp3	r2, r8, r8			; r2 = |norm|^2
 rsq    r2, r2				; r2 = 1/|norm|
 mul    r8, r8, r2			; norm = norm/|norm|

mul     r2.xyz, r1.xyz, r0.x
mov     r2.w,   r1.w        ; r2 = v0 * (1 + t*(R/|v0| - 1))
m4x4    oPos, r2, c4

; _________________________________
; CALCULATING COLOR
; gets: r8 (norm)
; using: r1
; returns: r9 (color)

dp3     r1, r8, -c64		; r1 = (norm, -L)
mul		r9, v2, c65			; r9 = C * I
mul     r9, r9, r1			; r9 = C * I * (norm, -L)

mov oD0, r9
