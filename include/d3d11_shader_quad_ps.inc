#if 0
//
// Generated by Microsoft (R) HLSL Shader Compiler 10.1
//
//
// Resource Bindings:
//
// Name                                 Type  Format         Dim      HLSL Bind  Count
// ------------------------------ ---------- ------- ----------- -------------- ------
// samplers                          sampler      NA          NA             s0      4 
// textures                          texture  float4          2d             t0      4 
//
//
//
// Input signature:
//
// Name                 Index   Mask Register SysValue  Format   Used
// -------------------- ----- ------ -------- -------- ------- ------
// TEXCOORD                 0   xyzw        0     NONE   float   xyzw
// COLOR                    0   xyzw        1     NONE   float   xyzw
// SV_POSITION              0   xyzw        2      POS   float       
// TEXCOORD                 1   xy          3     NONE   float   xy  
// TEXCOORD                 2     zw        3     NONE   float     zw
//
//
// Output signature:
//
// Name                 Index   Mask Register SysValue  Format   Used
// -------------------- ----- ------ -------- -------- ------- ------
// SV_TARGET                0   xyzw        0   TARGET   float   xyzw
//
//
// Sampler/Resource to DX9 shader sampler mappings:
//
// Target Sampler Source Sampler  Source Resource
// -------------- --------------- ----------------
// s0             s0              t0               
// s2             s1              t1               
// s5             s2              t2               
// s9             s3              t3               
//
//
// Level9 shader bytecode:
//
    ps_2_x
    def c0, -0.5, -3, -4, 0.5
    def c1, 0, 1, 2, 3
    def c2, 0.5, 1, 0, 0
    dcl t0
    dcl t1
    dcl t2
    dcl_2d s0
    dcl_2d s2
    dcl_2d s5
    dcl_2d s9
    add r0.xy, t2, c0.x
    add r0.xy, r0, r0
    mul r0.zw, r0.xyxy, t2.xywz
    dp2add r0.x, r0, r0, -c1.y
    abs r0.yz, r0.xzww
    add r0.yz, r0, -t2.xwzw
    min r0.w, t2.z, t2.w
    mad r0.yz, r0.w, c0.w, r0
    max r1.xy, r0.yzzw, c1.x
    max r1.z, r0.y, r0.z
    min r0.y, r1.z, c1.x
    dp2add r0.z, r1, r1, c1.x
    rsq r0.z, r0.z
    rcp r0.z, r0.z
    add r0.y, r0.z, r0.y
    mad r0.y, r0.w, -c0.w, r0.y
    add r0.y, r0.y, c1.y
    mad r0.z, r0.y, -c2.x, c2.y
    texld r1, t0, s0
    texld r2, t0, s9
    frc r3, t0.zzzw
    cmp r4, -r3, c1.x, c1.y
    add r3, -r3.zzzw, t0.zzzw
    cmp r4, t0.zzzw, c1.x, r4
    add r3, r3, r4
    abs r3, r3
    add r4, r3, -c1.yzwz
    add r3.xy, r3.w, c0.yzzw
    mul r3.xy, r3, r3
    mul r4, r4, r4
    cmp r1, -r4.z, r2, r1
    texld r2, t0, s2
    texld r5, t0, s5
    cmp r1, -r4.y, r5, r1
    cmp r1, -r4.x, r2, r1
    mul r2.x, r0.z, r1.w
    cmp r0.z, r0.z, r2.x, c1.x
    cmp r2.w, r0.y, r0.z, r1.w
    rcp r0.y, r0.w
    mul r0.z, r0.w, c0.w
    add r0.y, r0.y, r1.x
    add r0.y, r0.y, c0.x
    mul r5.w, r0.z, r0.y
    mov r5.xyz, c1.y
    cmp r5, -r3.y, r5, r1
    mov r2.xyz, r1
    cmp r0.w, r0.x, c1.x, r1.w
    cmp r1, -r3.x, r2, r5
    mov r0.xyz, r2
    cmp r0, -r4.w, r0, r1
    mul r0, r0, t1
    mov oC0, r0

// approximately 54 instruction slots used (4 texture, 50 arithmetic)
ps_4_0
dcl_sampler s0, mode_default
dcl_sampler s1, mode_default
dcl_sampler s2, mode_default
dcl_sampler s3, mode_default
dcl_resource_texture2d (float,float,float,float) t0
dcl_resource_texture2d (float,float,float,float) t1
dcl_resource_texture2d (float,float,float,float) t2
dcl_resource_texture2d (float,float,float,float) t3
dcl_input_ps linear v0.xyzw
dcl_input_ps linear v1.xyzw
dcl_input_ps linear v3.xy
dcl_input_ps linear v3.zw
dcl_output o0.xyzw
dcl_temps 2
ftoi r0.xy, v0.zwzz
imax r0.xy, -r0.xyxx, r0.xyxx
switch r0.x
  case l(1)
  sample r1.xyzw, v0.xyxx, t1.xyzw, s1
  break 
  case l(2)
  sample r1.xyzw, v0.xyxx, t2.xyzw, s2
  break 
  case l(3)
  sample r1.xyzw, v0.xyxx, t3.xyzw, s3
  break 
  default 
  sample r1.xyzw, v0.xyxx, t0.xyzw, s0
  break 
endswitch 
switch r0.y
  case l(2)
  add r0.xy, v3.xyxx, l(-0.500000, -0.500000, 0.000000, 0.000000)
  add r0.xy, r0.xyxx, r0.xyxx
  dp2 r0.x, r0.xyxx, r0.xyxx
  ge r0.x, r0.x, l(1.000000)
  movc r1.w, r0.x, l(0), r1.w
  break 
  case l(3)
  min r0.x, v3.w, v3.z
  add r0.yz, v3.xxyx, l(0.000000, -0.500000, -0.500000, 0.000000)
  mul r0.yz, r0.yyzy, v3.zzwz
  add r0.yz, r0.yyzy, r0.yyzy
  add r0.yz, |r0.yyzy|, -v3.zzwz
  mad r0.yz, r0.xxxx, l(0.000000, 0.500000, 0.500000, 0.000000), r0.yyzy
  max r0.w, r0.z, r0.y
  min r0.w, r0.w, l(0.000000)
  max r0.yz, r0.yyzy, l(0.000000, 0.000000, 0.000000, 0.000000)
  dp2 r0.y, r0.yzyy, r0.yzyy
  sqrt r0.y, r0.y
  add r0.y, r0.y, r0.w
  mad r0.x, -r0.x, l(0.500000), r0.y
  ge r0.y, r0.x, l(-1.000000)
  add r0.x, r0.x, l(1.000000)
  mad r0.x, -r0.x, l(0.500000), l(1.000000)
  max r0.x, r0.x, l(0.000000)
  mul r0.x, r0.x, r1.w
  movc r1.w, r0.y, r0.x, r1.w
  break 
  case l(4)
  min r0.x, v3.w, v3.z
  div r0.y, l(1.000000, 1.000000, 1.000000, 1.000000), r0.x
  add r0.y, r0.y, r1.x
  add r0.y, r0.y, l(-0.500000)
  mul r0.x, r0.y, r0.x
  mul r1.w, r0.x, l(0.500000)
  mov r1.xyz, l(1.000000,1.000000,1.000000,0)
  break 
  default 
  break 
endswitch 
mul o0.xyzw, r1.xyzw, v1.xyzw
ret 
// Approximately 59 instruction slots used
#endif

const BYTE g_D3d11Shader_QuadPixel[] =
{
     68,  88,  66,  67, 148, 106, 
     44,  63, 243,  50, 218, 209, 
      1, 150, 232, 132,  36,  23, 
    194, 254,   1,   0,   0,   0, 
    112,  12,   0,   0,   6,   0, 
      0,   0,  56,   0,   0,   0, 
    112,   4,   0,   0, 124,  10, 
      0,   0, 248,  10,   0,   0, 
    152,  11,   0,   0,  60,  12, 
      0,   0,  65, 111, 110,  57, 
     48,   4,   0,   0,  48,   4, 
      0,   0,   0,   2, 255, 255, 
    252,   3,   0,   0,  52,   0, 
      0,   0,   0,   0,  52,   0, 
      0,   0,  52,   0,   0,   0, 
     52,   0,   4,   0,  36,   0, 
      0,   0,  52,   0,   0,   0, 
      0,   0,   1,   1,   2,   0, 
      2,   2,   5,   0,   3,   3, 
      9,   0,   1,   2, 255, 255, 
     81,   0,   0,   5,   0,   0, 
     15, 160,   0,   0,   0, 191, 
      0,   0,  64, 192,   0,   0, 
    128, 192,   0,   0,   0,  63, 
     81,   0,   0,   5,   1,   0, 
     15, 160,   0,   0,   0,   0, 
      0,   0, 128,  63,   0,   0, 
      0,  64,   0,   0,  64,  64, 
     81,   0,   0,   5,   2,   0, 
     15, 160,   0,   0,   0,  63, 
      0,   0, 128,  63,   0,   0, 
      0,   0,   0,   0,   0,   0, 
     31,   0,   0,   2,   0,   0, 
      0, 128,   0,   0,  15, 176, 
     31,   0,   0,   2,   0,   0, 
      0, 128,   1,   0,  15, 176, 
     31,   0,   0,   2,   0,   0, 
      0, 128,   2,   0,  15, 176, 
     31,   0,   0,   2,   0,   0, 
      0, 144,   0,   8,  15, 160, 
     31,   0,   0,   2,   0,   0, 
      0, 144,   2,   8,  15, 160, 
     31,   0,   0,   2,   0,   0, 
      0, 144,   5,   8,  15, 160, 
     31,   0,   0,   2,   0,   0, 
      0, 144,   9,   8,  15, 160, 
      2,   0,   0,   3,   0,   0, 
      3, 128,   2,   0, 228, 176, 
      0,   0,   0, 160,   2,   0, 
      0,   3,   0,   0,   3, 128, 
      0,   0, 228, 128,   0,   0, 
    228, 128,   5,   0,   0,   3, 
      0,   0,  12, 128,   0,   0, 
     68, 128,   2,   0, 180, 176, 
     90,   0,   0,   4,   0,   0, 
      1, 128,   0,   0, 228, 128, 
      0,   0, 228, 128,   1,   0, 
     85, 161,  35,   0,   0,   2, 
      0,   0,   6, 128,   0,   0, 
    248, 128,   2,   0,   0,   3, 
      0,   0,   6, 128,   0,   0, 
    228, 128,   2,   0, 236, 177, 
     10,   0,   0,   3,   0,   0, 
      8, 128,   2,   0, 170, 176, 
      2,   0, 255, 176,   4,   0, 
      0,   4,   0,   0,   6, 128, 
      0,   0, 255, 128,   0,   0, 
    255, 160,   0,   0, 228, 128, 
     11,   0,   0,   3,   1,   0, 
      3, 128,   0,   0, 233, 128, 
      1,   0,   0, 160,  11,   0, 
      0,   3,   1,   0,   4, 128, 
      0,   0,  85, 128,   0,   0, 
    170, 128,  10,   0,   0,   3, 
      0,   0,   2, 128,   1,   0, 
    170, 128,   1,   0,   0, 160, 
     90,   0,   0,   4,   0,   0, 
      4, 128,   1,   0, 228, 128, 
      1,   0, 228, 128,   1,   0, 
      0, 160,   7,   0,   0,   2, 
      0,   0,   4, 128,   0,   0, 
    170, 128,   6,   0,   0,   2, 
      0,   0,   4, 128,   0,   0, 
    170, 128,   2,   0,   0,   3, 
      0,   0,   2, 128,   0,   0, 
    170, 128,   0,   0,  85, 128, 
      4,   0,   0,   4,   0,   0, 
      2, 128,   0,   0, 255, 128, 
      0,   0, 255, 161,   0,   0, 
     85, 128,   2,   0,   0,   3, 
      0,   0,   2, 128,   0,   0, 
     85, 128,   1,   0,  85, 160, 
      4,   0,   0,   4,   0,   0, 
      4, 128,   0,   0,  85, 128, 
      2,   0,   0, 161,   2,   0, 
     85, 160,  66,   0,   0,   3, 
      1,   0,  15, 128,   0,   0, 
    228, 176,   0,   8, 228, 160, 
     66,   0,   0,   3,   2,   0, 
     15, 128,   0,   0, 228, 176, 
      9,   8, 228, 160,  19,   0, 
      0,   2,   3,   0,  15, 128, 
      0,   0, 234, 176,  88,   0, 
      0,   4,   4,   0,  15, 128, 
      3,   0, 228, 129,   1,   0, 
      0, 160,   1,   0,  85, 160, 
      2,   0,   0,   3,   3,   0, 
     15, 128,   3,   0, 234, 129, 
      0,   0, 234, 176,  88,   0, 
      0,   4,   4,   0,  15, 128, 
      0,   0, 234, 176,   1,   0, 
      0, 160,   4,   0, 228, 128, 
      2,   0,   0,   3,   3,   0, 
     15, 128,   3,   0, 228, 128, 
      4,   0, 228, 128,  35,   0, 
      0,   2,   3,   0,  15, 128, 
      3,   0, 228, 128,   2,   0, 
      0,   3,   4,   0,  15, 128, 
      3,   0, 228, 128,   1,   0, 
    185, 161,   2,   0,   0,   3, 
      3,   0,   3, 128,   3,   0, 
    255, 128,   0,   0, 233, 160, 
      5,   0,   0,   3,   3,   0, 
      3, 128,   3,   0, 228, 128, 
      3,   0, 228, 128,   5,   0, 
      0,   3,   4,   0,  15, 128, 
      4,   0, 228, 128,   4,   0, 
    228, 128,  88,   0,   0,   4, 
      1,   0,  15, 128,   4,   0, 
    170, 129,   2,   0, 228, 128, 
      1,   0, 228, 128,  66,   0, 
      0,   3,   2,   0,  15, 128, 
      0,   0, 228, 176,   2,   8, 
    228, 160,  66,   0,   0,   3, 
      5,   0,  15, 128,   0,   0, 
    228, 176,   5,   8, 228, 160, 
     88,   0,   0,   4,   1,   0, 
     15, 128,   4,   0,  85, 129, 
      5,   0, 228, 128,   1,   0, 
    228, 128,  88,   0,   0,   4, 
      1,   0,  15, 128,   4,   0, 
      0, 129,   2,   0, 228, 128, 
      1,   0, 228, 128,   5,   0, 
      0,   3,   2,   0,   1, 128, 
      0,   0, 170, 128,   1,   0, 
    255, 128,  88,   0,   0,   4, 
      0,   0,   4, 128,   0,   0, 
    170, 128,   2,   0,   0, 128, 
      1,   0,   0, 160,  88,   0, 
      0,   4,   2,   0,   8, 128, 
      0,   0,  85, 128,   0,   0, 
    170, 128,   1,   0, 255, 128, 
      6,   0,   0,   2,   0,   0, 
      2, 128,   0,   0, 255, 128, 
      5,   0,   0,   3,   0,   0, 
      4, 128,   0,   0, 255, 128, 
      0,   0, 255, 160,   2,   0, 
      0,   3,   0,   0,   2, 128, 
      0,   0,  85, 128,   1,   0, 
      0, 128,   2,   0,   0,   3, 
      0,   0,   2, 128,   0,   0, 
     85, 128,   0,   0,   0, 160, 
      5,   0,   0,   3,   5,   0, 
      8, 128,   0,   0, 170, 128, 
      0,   0,  85, 128,   1,   0, 
      0,   2,   5,   0,   7, 128, 
      1,   0,  85, 160,  88,   0, 
      0,   4,   5,   0,  15, 128, 
      3,   0,  85, 129,   5,   0, 
    228, 128,   1,   0, 228, 128, 
      1,   0,   0,   2,   2,   0, 
      7, 128,   1,   0, 228, 128, 
     88,   0,   0,   4,   0,   0, 
      8, 128,   0,   0,   0, 128, 
      1,   0,   0, 160,   1,   0, 
    255, 128,  88,   0,   0,   4, 
      1,   0,  15, 128,   3,   0, 
      0, 129,   2,   0, 228, 128, 
      5,   0, 228, 128,   1,   0, 
      0,   2,   0,   0,   7, 128, 
      2,   0, 228, 128,  88,   0, 
      0,   4,   0,   0,  15, 128, 
      4,   0, 255, 129,   0,   0, 
    228, 128,   1,   0, 228, 128, 
      5,   0,   0,   3,   0,   0, 
     15, 128,   0,   0, 228, 128, 
      1,   0, 228, 176,   1,   0, 
      0,   2,   0,   8,  15, 128, 
      0,   0, 228, 128, 255, 255, 
      0,   0,  83,  72,  68,  82, 
      4,   6,   0,   0,  64,   0, 
      0,   0, 129,   1,   0,   0, 
     90,   0,   0,   3,   0,  96, 
     16,   0,   0,   0,   0,   0, 
     90,   0,   0,   3,   0,  96, 
     16,   0,   1,   0,   0,   0, 
     90,   0,   0,   3,   0,  96, 
     16,   0,   2,   0,   0,   0, 
     90,   0,   0,   3,   0,  96, 
     16,   0,   3,   0,   0,   0, 
     88,  24,   0,   4,   0, 112, 
     16,   0,   0,   0,   0,   0, 
     85,  85,   0,   0,  88,  24, 
      0,   4,   0, 112,  16,   0, 
      1,   0,   0,   0,  85,  85, 
      0,   0,  88,  24,   0,   4, 
      0, 112,  16,   0,   2,   0, 
      0,   0,  85,  85,   0,   0, 
     88,  24,   0,   4,   0, 112, 
     16,   0,   3,   0,   0,   0, 
     85,  85,   0,   0,  98,  16, 
      0,   3, 242,  16,  16,   0, 
      0,   0,   0,   0,  98,  16, 
      0,   3, 242,  16,  16,   0, 
      1,   0,   0,   0,  98,  16, 
      0,   3,  50,  16,  16,   0, 
      3,   0,   0,   0,  98,  16, 
      0,   3, 194,  16,  16,   0, 
      3,   0,   0,   0, 101,   0, 
      0,   3, 242,  32,  16,   0, 
      0,   0,   0,   0, 104,   0, 
      0,   2,   2,   0,   0,   0, 
     27,   0,   0,   5,  50,   0, 
     16,   0,   0,   0,   0,   0, 
    230,  26,  16,   0,   0,   0, 
      0,   0,  36,   0,   0,   8, 
     50,   0,  16,   0,   0,   0, 
      0,   0,  70,   0,  16, 128, 
     65,   0,   0,   0,   0,   0, 
      0,   0,  70,   0,  16,   0, 
      0,   0,   0,   0,  76,   0, 
      0,   3,  10,   0,  16,   0, 
      0,   0,   0,   0,   6,   0, 
      0,   3,   1,  64,   0,   0, 
      1,   0,   0,   0,  69,   0, 
      0,   9, 242,   0,  16,   0, 
      1,   0,   0,   0,  70,  16, 
     16,   0,   0,   0,   0,   0, 
     70, 126,  16,   0,   1,   0, 
      0,   0,   0,  96,  16,   0, 
      1,   0,   0,   0,   2,   0, 
      0,   1,   6,   0,   0,   3, 
      1,  64,   0,   0,   2,   0, 
      0,   0,  69,   0,   0,   9, 
    242,   0,  16,   0,   1,   0, 
      0,   0,  70,  16,  16,   0, 
      0,   0,   0,   0,  70, 126, 
     16,   0,   2,   0,   0,   0, 
      0,  96,  16,   0,   2,   0, 
      0,   0,   2,   0,   0,   1, 
      6,   0,   0,   3,   1,  64, 
      0,   0,   3,   0,   0,   0, 
     69,   0,   0,   9, 242,   0, 
     16,   0,   1,   0,   0,   0, 
     70,  16,  16,   0,   0,   0, 
      0,   0,  70, 126,  16,   0, 
      3,   0,   0,   0,   0,  96, 
     16,   0,   3,   0,   0,   0, 
      2,   0,   0,   1,  10,   0, 
      0,   1,  69,   0,   0,   9, 
    242,   0,  16,   0,   1,   0, 
      0,   0,  70,  16,  16,   0, 
      0,   0,   0,   0,  70, 126, 
     16,   0,   0,   0,   0,   0, 
      0,  96,  16,   0,   0,   0, 
      0,   0,   2,   0,   0,   1, 
     23,   0,   0,   1,  76,   0, 
      0,   3,  26,   0,  16,   0, 
      0,   0,   0,   0,   6,   0, 
      0,   3,   1,  64,   0,   0, 
      2,   0,   0,   0,   0,   0, 
      0,  10,  50,   0,  16,   0, 
      0,   0,   0,   0,  70,  16, 
     16,   0,   3,   0,   0,   0, 
      2,  64,   0,   0,   0,   0, 
      0, 191,   0,   0,   0, 191, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   7, 
     50,   0,  16,   0,   0,   0, 
      0,   0,  70,   0,  16,   0, 
      0,   0,   0,   0,  70,   0, 
     16,   0,   0,   0,   0,   0, 
     15,   0,   0,   7,  18,   0, 
     16,   0,   0,   0,   0,   0, 
     70,   0,  16,   0,   0,   0, 
      0,   0,  70,   0,  16,   0, 
      0,   0,   0,   0,  29,   0, 
      0,   7,  18,   0,  16,   0, 
      0,   0,   0,   0,  10,   0, 
     16,   0,   0,   0,   0,   0, 
      1,  64,   0,   0,   0,   0, 
    128,  63,  55,   0,   0,   9, 
    130,   0,  16,   0,   1,   0, 
      0,   0,  10,   0,  16,   0, 
      0,   0,   0,   0,   1,  64, 
      0,   0,   0,   0,   0,   0, 
     58,   0,  16,   0,   1,   0, 
      0,   0,   2,   0,   0,   1, 
      6,   0,   0,   3,   1,  64, 
      0,   0,   3,   0,   0,   0, 
     51,   0,   0,   7,  18,   0, 
     16,   0,   0,   0,   0,   0, 
     58,  16,  16,   0,   3,   0, 
      0,   0,  42,  16,  16,   0, 
      3,   0,   0,   0,   0,   0, 
      0,  10,  98,   0,  16,   0, 
      0,   0,   0,   0,   6,  17, 
     16,   0,   3,   0,   0,   0, 
      2,  64,   0,   0,   0,   0, 
      0,   0,   0,   0,   0, 191, 
      0,   0,   0, 191,   0,   0, 
      0,   0,  56,   0,   0,   7, 
     98,   0,  16,   0,   0,   0, 
      0,   0,  86,   6,  16,   0, 
      0,   0,   0,   0, 166,  27, 
     16,   0,   3,   0,   0,   0, 
      0,   0,   0,   7,  98,   0, 
     16,   0,   0,   0,   0,   0, 
     86,   6,  16,   0,   0,   0, 
      0,   0,  86,   6,  16,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   9,  98,   0,  16,   0, 
      0,   0,   0,   0,  86,   6, 
     16, 128, 129,   0,   0,   0, 
      0,   0,   0,   0, 166,  27, 
     16, 128,  65,   0,   0,   0, 
      3,   0,   0,   0,  50,   0, 
      0,  12,  98,   0,  16,   0, 
      0,   0,   0,   0,   6,   0, 
     16,   0,   0,   0,   0,   0, 
      2,  64,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,  63, 
      0,   0,   0,  63,   0,   0, 
      0,   0,  86,   6,  16,   0, 
      0,   0,   0,   0,  52,   0, 
      0,   7, 130,   0,  16,   0, 
      0,   0,   0,   0,  42,   0, 
     16,   0,   0,   0,   0,   0, 
     26,   0,  16,   0,   0,   0, 
      0,   0,  51,   0,   0,   7, 
    130,   0,  16,   0,   0,   0, 
      0,   0,  58,   0,  16,   0, 
      0,   0,   0,   0,   1,  64, 
      0,   0,   0,   0,   0,   0, 
     52,   0,   0,  10,  98,   0, 
     16,   0,   0,   0,   0,   0, 
     86,   6,  16,   0,   0,   0, 
      0,   0,   2,  64,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,  15,   0, 
      0,   7,  34,   0,  16,   0, 
      0,   0,   0,   0, 150,   5, 
     16,   0,   0,   0,   0,   0, 
    150,   5,  16,   0,   0,   0, 
      0,   0,  75,   0,   0,   5, 
     34,   0,  16,   0,   0,   0, 
      0,   0,  26,   0,  16,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   7,  34,   0,  16,   0, 
      0,   0,   0,   0,  26,   0, 
     16,   0,   0,   0,   0,   0, 
     58,   0,  16,   0,   0,   0, 
      0,   0,  50,   0,   0,  10, 
     18,   0,  16,   0,   0,   0, 
      0,   0,  10,   0,  16, 128, 
     65,   0,   0,   0,   0,   0, 
      0,   0,   1,  64,   0,   0, 
      0,   0,   0,  63,  26,   0, 
     16,   0,   0,   0,   0,   0, 
     29,   0,   0,   7,  34,   0, 
     16,   0,   0,   0,   0,   0, 
     10,   0,  16,   0,   0,   0, 
      0,   0,   1,  64,   0,   0, 
      0,   0, 128, 191,   0,   0, 
      0,   7,  18,   0,  16,   0, 
      0,   0,   0,   0,  10,   0, 
     16,   0,   0,   0,   0,   0, 
      1,  64,   0,   0,   0,   0, 
    128,  63,  50,   0,   0,  10, 
     18,   0,  16,   0,   0,   0, 
      0,   0,  10,   0,  16, 128, 
     65,   0,   0,   0,   0,   0, 
      0,   0,   1,  64,   0,   0, 
      0,   0,   0,  63,   1,  64, 
      0,   0,   0,   0, 128,  63, 
     52,   0,   0,   7,  18,   0, 
     16,   0,   0,   0,   0,   0, 
     10,   0,  16,   0,   0,   0, 
      0,   0,   1,  64,   0,   0, 
      0,   0,   0,   0,  56,   0, 
      0,   7,  18,   0,  16,   0, 
      0,   0,   0,   0,  10,   0, 
     16,   0,   0,   0,   0,   0, 
     58,   0,  16,   0,   1,   0, 
      0,   0,  55,   0,   0,   9, 
    130,   0,  16,   0,   1,   0, 
      0,   0,  26,   0,  16,   0, 
      0,   0,   0,   0,  10,   0, 
     16,   0,   0,   0,   0,   0, 
     58,   0,  16,   0,   1,   0, 
      0,   0,   2,   0,   0,   1, 
      6,   0,   0,   3,   1,  64, 
      0,   0,   4,   0,   0,   0, 
     51,   0,   0,   7,  18,   0, 
     16,   0,   0,   0,   0,   0, 
     58,  16,  16,   0,   3,   0, 
      0,   0,  42,  16,  16,   0, 
      3,   0,   0,   0,  14,   0, 
      0,  10,  34,   0,  16,   0, 
      0,   0,   0,   0,   2,  64, 
      0,   0,   0,   0, 128,  63, 
      0,   0, 128,  63,   0,   0, 
    128,  63,   0,   0, 128,  63, 
     10,   0,  16,   0,   0,   0, 
      0,   0,   0,   0,   0,   7, 
     34,   0,  16,   0,   0,   0, 
      0,   0,  26,   0,  16,   0, 
      0,   0,   0,   0,  10,   0, 
     16,   0,   1,   0,   0,   0, 
      0,   0,   0,   7,  34,   0, 
     16,   0,   0,   0,   0,   0, 
     26,   0,  16,   0,   0,   0, 
      0,   0,   1,  64,   0,   0, 
      0,   0,   0, 191,  56,   0, 
      0,   7,  18,   0,  16,   0, 
      0,   0,   0,   0,  26,   0, 
     16,   0,   0,   0,   0,   0, 
     10,   0,  16,   0,   0,   0, 
      0,   0,  56,   0,   0,   7, 
    130,   0,  16,   0,   1,   0, 
      0,   0,  10,   0,  16,   0, 
      0,   0,   0,   0,   1,  64, 
      0,   0,   0,   0,   0,  63, 
     54,   0,   0,   8, 114,   0, 
     16,   0,   1,   0,   0,   0, 
      2,  64,   0,   0,   0,   0, 
    128,  63,   0,   0, 128,  63, 
      0,   0, 128,  63,   0,   0, 
      0,   0,   2,   0,   0,   1, 
     10,   0,   0,   1,   2,   0, 
      0,   1,  23,   0,   0,   1, 
     56,   0,   0,   7, 242,  32, 
     16,   0,   0,   0,   0,   0, 
     70,  14,  16,   0,   1,   0, 
      0,   0,  70,  30,  16,   0, 
      1,   0,   0,   0,  62,   0, 
      0,   1,  83,  84,  65,  84, 
    116,   0,   0,   0,  59,   0, 
      0,   0,   2,   0,   0,   0, 
      0,   0,   0,   0,   5,   0, 
      0,   0,  29,   0,   0,   0, 
      1,   0,   0,   0,   0,   0, 
      0,   0,   9,   0,   0,   0, 
      2,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      4,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   1,   0,   0,   0, 
      2,   0,   0,   0,   1,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
     82,  68,  69,  70, 152,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   2,   0, 
      0,   0,  28,   0,   0,   0, 
      0,   4, 255, 255,   0, 129, 
      0,   0, 110,   0,   0,   0, 
     92,   0,   0,   0,   3,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      4,   0,   0,   0,   1,   0, 
      0,   0, 101,   0,   0,   0, 
      2,   0,   0,   0,   5,   0, 
      0,   0,   4,   0,   0,   0, 
    255, 255, 255, 255,   0,   0, 
      0,   0,   4,   0,   0,   0, 
     13,   0,   0,   0, 115,  97, 
    109, 112, 108, 101, 114, 115, 
      0, 116, 101, 120, 116, 117, 
    114, 101, 115,   0,  77, 105, 
     99, 114, 111, 115, 111, 102, 
    116,  32,  40,  82,  41,  32, 
     72,  76,  83,  76,  32,  83, 
    104,  97, 100, 101, 114,  32, 
     67, 111, 109, 112, 105, 108, 
    101, 114,  32,  49,  48,  46, 
     49,   0, 171, 171,  73,  83, 
     71,  78, 156,   0,   0,   0, 
      5,   0,   0,   0,   8,   0, 
      0,   0, 128,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   3,   0,   0,   0, 
      0,   0,   0,   0,  15,  15, 
      0,   0, 137,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   3,   0,   0,   0, 
      1,   0,   0,   0,  15,  15, 
      0,   0, 143,   0,   0,   0, 
      0,   0,   0,   0,   1,   0, 
      0,   0,   3,   0,   0,   0, 
      2,   0,   0,   0,  15,   0, 
      0,   0, 128,   0,   0,   0, 
      1,   0,   0,   0,   0,   0, 
      0,   0,   3,   0,   0,   0, 
      3,   0,   0,   0,   3,   3, 
      0,   0, 128,   0,   0,   0, 
      2,   0,   0,   0,   0,   0, 
      0,   0,   3,   0,   0,   0, 
      3,   0,   0,   0,  12,  12, 
      0,   0,  84,  69,  88,  67, 
     79,  79,  82,  68,   0,  67, 
     79,  76,  79,  82,   0,  83, 
     86,  95,  80,  79,  83,  73, 
     84,  73,  79,  78,   0, 171, 
     79,  83,  71,  78,  44,   0, 
      0,   0,   1,   0,   0,   0, 
      8,   0,   0,   0,  32,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   3,   0, 
      0,   0,   0,   0,   0,   0, 
     15,   0,   0,   0,  83,  86, 
     95,  84,  65,  82,  71,  69, 
     84,   0, 171, 171
};