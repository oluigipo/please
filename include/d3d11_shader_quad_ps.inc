#if 0
//
// Generated by Microsoft (R) HLSL Shader Compiler 10.1
//
//
// Note: shader requires additional functionality:
//       Minimum-precision data types
//
//
// Buffer Definitions: 
//
// cbuffer VS_CONSTANT_BUFFER
// {
//
//   float4x4 uView;                    // Offset:    0 Size:    64 [unused]
//   float4 uTexsize01;                 // Offset:   64 Size:    16
//   float4 uTexsize23;                 // Offset:   80 Size:    16
//
// }
//
//
// Resource Bindings:
//
// Name                                 Type  Format         Dim      HLSL Bind  Count
// ------------------------------ ---------- ------- ----------- -------------- ------
// samplers                          sampler      NA          NA             s0      4 
// textures                          texture  float4          2d             t0      4 
// VS_CONSTANT_BUFFER                cbuffer      NA          NA            cb0      1 
//
//
//
// Input signature:
//
// Name                 Index   Mask Register SysValue  Format   Used
// -------------------- ----- ------ -------- -------- ------- ------
// TEXCOORD                 0   xy          0     NONE   float   xy  
// TEXCOORD                 3     zw        0     NONE   float     zw
// SV_POSITION              0   xyzw        1      POS   float       
// TEXCOORD                 2   xy          2     NONE   float   xy  
// TEXCOORD                 1     zw        2     NONE   float     zw
// COLOR                    0   xyzw        3     NONE  min16f   xyzw
//
//
// Output signature:
//
// Name                 Index   Mask Register SysValue  Format   Used
// -------------------- ----- ------ -------- -------- ------- ------
// SV_TARGET                0   xyzw        0   TARGET  min16f   xyzw
//
ps_4_0
dcl_globalFlags refactoringAllowed | enableMinimumPrecision
dcl_constantbuffer CB0[6], immediateIndexed
dcl_sampler s0, mode_default
dcl_sampler s1, mode_default
dcl_sampler s2, mode_default
dcl_sampler s3, mode_default
dcl_resource_texture2d (float,float,float,float) t0
dcl_resource_texture2d (float,float,float,float) t1
dcl_resource_texture2d (float,float,float,float) t2
dcl_resource_texture2d (float,float,float,float) t3
dcl_input_ps linear v0.xy
dcl_input_ps linear v0.zw
dcl_input_ps linear v2.xy
dcl_input_ps linear v2.zw
dcl_input_ps linear v3.xyzw {min16f}
dcl_output o0.xyzw {min16f}
dcl_temps 4
ftoi r0.xy, v0.zwzz
imax r0.xy, -r0.xyxx, r0.xyxx
switch r0.x
  case l(1)
  sample r1.xyzw {min16f}, v0.xyxx, t1.xyzw, s1
  mov r0.xz, cb0[4].zzwz
  break 
  case l(2)
  sample r1.xyzw {min16f}, v0.xyxx, t2.xyzw, s2
  mov r0.xz, cb0[5].xxyx
  break 
  case l(3)
  sample r1.xyzw {min16f}, v0.xyxx, t3.xyzw, s3
  mov r0.xz, cb0[5].zzwz
  break 
  default 
  sample r1.xyzw {min16f}, v0.xyxx, t0.xyzw, s0
  mov r0.xz, cb0[4].xxyx
  break 
endswitch 
switch r0.y
  case l(1)
  add r0.yw, v2.zzzw, l(0.000000, -0.500000, 0.000000, -0.500000)
  mul r0.yw, r0.yyyw, v2.xxxy
  min r2.x, v2.y, v2.x
  add r0.yw, r0.yyyw, r0.yyyw
  add r0.yw, |r0.yyyw|, -v2.xxxy
  mad r0.yw, r2.xxxx, l(0.000000, 0.500000, 0.000000, 0.500000), r0.yyyw
  max r2.y, r0.w, r0.y
  min r2.y, r2.y, l(0.000000)
  max r0.yw, r0.yyyw, l(0.000000, 0.000000, 0.000000, 0.000000)
  dp2 r0.y, r0.ywyy, r0.ywyy
  sqrt r0.y, r0.y
  add r0.y, r0.y, r2.y
  mad r0.y, -r2.x, l(0.500000), r0.y
  ge r0.w, r0.y, l(-1.000000)
  add r0.y, r0.y, l(1.000000)
  mad r0.y, -r0.y, l(0.500000), l(1.000000)
  max r0.y, r0.y, l(0.000000)
  mul r3.x {min16f}, r0.y {def32 as min16f}, r1.w {min16f}
  movc r1.w {min16f}, r0.w, r3.x {min16f}, r1.w {min16f}
  break 
  case l(2)
  deriv_rtx r0.yw, v0.xxxy
  deriv_rty r2.xy, v0.xyxx
  add r0.yw, |r0.yyyw|, |r2.xxxy|
  mul r0.xy, r0.xzxx, r0.ywyy
  min r0.x, r0.y, r0.x
  div r0.y, l(1.000000, 1.000000, 1.000000, 1.000000), r0.x
  add r3.x {min16f}, r1.x {min16f}, l(-0.501961) {def32 as min16f}
  mad r0.x, r0.x, l(0.047059), r3.x {min16f as def32}
  mul r0.x, r0.y, r0.x
  mul r0.w, r0.x, l(10.625000)
  mov r0.xyz, l(1.000000,1.000000,1.000000,0)
  mov r1.xyzw {min16f}, r0.xyzw {def32 as min16f}
  break 
  default 
  break 
endswitch 
mul o0.xyzw {min16f}, r1.xyzw {min16f}, v3.xyzw {min16f}
ret 
// Approximately 61 instruction slots used
#endif

const BYTE g_render_d3d11_shader_quad_ps[] =
{
     68,  88,  66,  67, 194, 121, 
    182, 214, 163,  89,  90, 243, 
    193,  74, 158, 149,  45,  73, 
    183,  24,   1,   0,   0,   0, 
      8,  10,   0,   0,   6,   0, 
      0,   0,  56,   0,   0,   0, 
    172,   1,   0,   0, 152,   2, 
      0,   0, 212,   2,   0,   0, 
    124,   9,   0,   0, 140,   9, 
      0,   0,  82,  68,  69,  70, 
    108,   1,   0,   0,   1,   0, 
      0,   0, 164,   0,   0,   0, 
      3,   0,   0,   0,  28,   0, 
      0,   0,   0,   4, 255, 255, 
      0, 129,   0,   0,  67,   1, 
      0,   0, 124,   0,   0,   0, 
      3,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   4,   0,   0,   0, 
      1,   0,   0,   0, 133,   0, 
      0,   0,   2,   0,   0,   0, 
      5,   0,   0,   0,   4,   0, 
      0,   0, 255, 255, 255, 255, 
      0,   0,   0,   0,   4,   0, 
      0,   0,  13,   0,   0,   0, 
    142,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      1,   0,   0,   0,   1,   0, 
      0,   0, 115,  97, 109, 112, 
    108, 101, 114, 115,   0, 116, 
    101, 120, 116, 117, 114, 101, 
    115,   0,  86,  83,  95,  67, 
     79,  78,  83,  84,  65,  78, 
     84,  95,  66,  85,  70,  70, 
     69,  82,   0, 171, 171, 171, 
    142,   0,   0,   0,   3,   0, 
      0,   0, 188,   0,   0,   0, 
     96,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      4,   1,   0,   0,   0,   0, 
      0,   0,  64,   0,   0,   0, 
      0,   0,   0,   0,  12,   1, 
      0,   0,   0,   0,   0,   0, 
     28,   1,   0,   0,  64,   0, 
      0,   0,  16,   0,   0,   0, 
      2,   0,   0,   0,  40,   1, 
      0,   0,   0,   0,   0,   0, 
     56,   1,   0,   0,  80,   0, 
      0,   0,  16,   0,   0,   0, 
      2,   0,   0,   0,  40,   1, 
      0,   0,   0,   0,   0,   0, 
    117,  86, 105, 101, 119,   0, 
    171, 171,   3,   0,   3,   0, 
      4,   0,   4,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
    117,  84, 101, 120, 115, 105, 
    122, 101,  48,  49,   0, 171, 
      1,   0,   3,   0,   1,   0, 
      4,   0,   0,   0,   0,   0, 
      0,   0,   0,   0, 117,  84, 
    101, 120, 115, 105, 122, 101, 
     50,  51,   0,  77, 105,  99, 
    114, 111, 115, 111, 102, 116, 
     32,  40,  82,  41,  32,  72, 
     76,  83,  76,  32,  83, 104, 
     97, 100, 101, 114,  32,  67, 
    111, 109, 112, 105, 108, 101, 
    114,  32,  49,  48,  46,  49, 
      0, 171,  73,  83,  71,  49, 
    228,   0,   0,   0,   6,   0, 
      0,   0,   8,   0,   0,   0, 
      0,   0,   0,   0, 200,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   3,   0, 
      0,   0,   0,   0,   0,   0, 
      3,   3,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
    200,   0,   0,   0,   3,   0, 
      0,   0,   0,   0,   0,   0, 
      3,   0,   0,   0,   0,   0, 
      0,   0,  12,  12,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0, 209,   0,   0,   0, 
      0,   0,   0,   0,   1,   0, 
      0,   0,   3,   0,   0,   0, 
      1,   0,   0,   0,  15,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0, 200,   0, 
      0,   0,   2,   0,   0,   0, 
      0,   0,   0,   0,   3,   0, 
      0,   0,   2,   0,   0,   0, 
      3,   3,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
    200,   0,   0,   0,   1,   0, 
      0,   0,   0,   0,   0,   0, 
      3,   0,   0,   0,   2,   0, 
      0,   0,  12,  12,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0, 221,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   3,   0,   0,   0, 
      3,   0,   0,   0,  15,  15, 
      0,   0,   1,   0,   0,   0, 
     84,  69,  88,  67,  79,  79, 
     82,  68,   0,  83,  86,  95, 
     80,  79,  83,  73,  84,  73, 
     79,  78,   0,  67,  79,  76, 
     79,  82,   0, 171,  79,  83, 
     71,  49,  52,   0,   0,   0, 
      1,   0,   0,   0,   8,   0, 
      0,   0,   0,   0,   0,   0, 
     40,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      3,   0,   0,   0,   0,   0, 
      0,   0,  15,   0,   0,   0, 
      1,   0,   0,   0,  83,  86, 
     95,  84,  65,  82,  71,  69, 
     84,   0, 171, 171,  83,  72, 
     69,  88, 160,   6,   0,   0, 
     64,   0,   0,   0, 168,   1, 
      0,   0, 106,   8,   1,   1, 
     89,   0,   0,   4,  70, 142, 
     32,   0,   0,   0,   0,   0, 
      6,   0,   0,   0,  90,   0, 
      0,   3,   0,  96,  16,   0, 
      0,   0,   0,   0,  90,   0, 
      0,   3,   0,  96,  16,   0, 
      1,   0,   0,   0,  90,   0, 
      0,   3,   0,  96,  16,   0, 
      2,   0,   0,   0,  90,   0, 
      0,   3,   0,  96,  16,   0, 
      3,   0,   0,   0,  88,  24, 
      0,   4,   0, 112,  16,   0, 
      0,   0,   0,   0,  85,  85, 
      0,   0,  88,  24,   0,   4, 
      0, 112,  16,   0,   1,   0, 
      0,   0,  85,  85,   0,   0, 
     88,  24,   0,   4,   0, 112, 
     16,   0,   2,   0,   0,   0, 
     85,  85,   0,   0,  88,  24, 
      0,   4,   0, 112,  16,   0, 
      3,   0,   0,   0,  85,  85, 
      0,   0,  98,  16,   0,   3, 
     50,  16,  16,   0,   0,   0, 
      0,   0,  98,  16,   0,   3, 
    194,  16,  16,   0,   0,   0, 
      0,   0,  98,  16,   0,   3, 
     50,  16,  16,   0,   2,   0, 
      0,   0,  98,  16,   0,   3, 
    194,  16,  16,   0,   2,   0, 
      0,   0,  98,  16,   0,   4, 
    242,  16,  16, 128,   1,  64, 
      0,   0,   3,   0,   0,   0, 
    101,   0,   0,   4, 242,  32, 
     16, 128,   1,  64,   0,   0, 
      0,   0,   0,   0, 104,   0, 
      0,   2,   4,   0,   0,   0, 
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
      0,  10, 242,   0,  16, 128, 
      1,  64,   0,   0,   1,   0, 
      0,   0,  70,  16,  16,   0, 
      0,   0,   0,   0,  70, 126, 
     16,   0,   1,   0,   0,   0, 
      0,  96,  16,   0,   1,   0, 
      0,   0,  54,   0,   0,   6, 
     82,   0,  16,   0,   0,   0, 
      0,   0, 166, 139,  32,   0, 
      0,   0,   0,   0,   4,   0, 
      0,   0,   2,   0,   0,   1, 
      6,   0,   0,   3,   1,  64, 
      0,   0,   2,   0,   0,   0, 
     69,   0,   0,  10, 242,   0, 
     16, 128,   1,  64,   0,   0, 
      1,   0,   0,   0,  70,  16, 
     16,   0,   0,   0,   0,   0, 
     70, 126,  16,   0,   2,   0, 
      0,   0,   0,  96,  16,   0, 
      2,   0,   0,   0,  54,   0, 
      0,   6,  82,   0,  16,   0, 
      0,   0,   0,   0,   6, 129, 
     32,   0,   0,   0,   0,   0, 
      5,   0,   0,   0,   2,   0, 
      0,   1,   6,   0,   0,   3, 
      1,  64,   0,   0,   3,   0, 
      0,   0,  69,   0,   0,  10, 
    242,   0,  16, 128,   1,  64, 
      0,   0,   1,   0,   0,   0, 
     70,  16,  16,   0,   0,   0, 
      0,   0,  70, 126,  16,   0, 
      3,   0,   0,   0,   0,  96, 
     16,   0,   3,   0,   0,   0, 
     54,   0,   0,   6,  82,   0, 
     16,   0,   0,   0,   0,   0, 
    166, 139,  32,   0,   0,   0, 
      0,   0,   5,   0,   0,   0, 
      2,   0,   0,   1,  10,   0, 
      0,   1,  69,   0,   0,  10, 
    242,   0,  16, 128,   1,  64, 
      0,   0,   1,   0,   0,   0, 
     70,  16,  16,   0,   0,   0, 
      0,   0,  70, 126,  16,   0, 
      0,   0,   0,   0,   0,  96, 
     16,   0,   0,   0,   0,   0, 
     54,   0,   0,   6,  82,   0, 
     16,   0,   0,   0,   0,   0, 
      6, 129,  32,   0,   0,   0, 
      0,   0,   4,   0,   0,   0, 
      2,   0,   0,   1,  23,   0, 
      0,   1,  76,   0,   0,   3, 
     26,   0,  16,   0,   0,   0, 
      0,   0,   6,   0,   0,   3, 
      1,  64,   0,   0,   1,   0, 
      0,   0,   0,   0,   0,  10, 
    162,   0,  16,   0,   0,   0, 
      0,   0, 166,  30,  16,   0, 
      2,   0,   0,   0,   2,  64, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0, 191,   0,   0, 
      0,   0,   0,   0,   0, 191, 
     56,   0,   0,   7, 162,   0, 
     16,   0,   0,   0,   0,   0, 
     86,  13,  16,   0,   0,   0, 
      0,   0,   6,  20,  16,   0, 
      2,   0,   0,   0,  51,   0, 
      0,   7,  18,   0,  16,   0, 
      2,   0,   0,   0,  26,  16, 
     16,   0,   2,   0,   0,   0, 
     10,  16,  16,   0,   2,   0, 
      0,   0,   0,   0,   0,   7, 
    162,   0,  16,   0,   0,   0, 
      0,   0,  86,  13,  16,   0, 
      0,   0,   0,   0,  86,  13, 
     16,   0,   0,   0,   0,   0, 
      0,   0,   0,   9, 162,   0, 
     16,   0,   0,   0,   0,   0, 
     86,  13,  16, 128, 129,   0, 
      0,   0,   0,   0,   0,   0, 
      6,  20,  16, 128,  65,   0, 
      0,   0,   2,   0,   0,   0, 
     50,   0,   0,  12, 162,   0, 
     16,   0,   0,   0,   0,   0, 
      6,   0,  16,   0,   2,   0, 
      0,   0,   2,  64,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,  63,   0,   0,   0,   0, 
      0,   0,   0,  63,  86,  13, 
     16,   0,   0,   0,   0,   0, 
     52,   0,   0,   7,  34,   0, 
     16,   0,   2,   0,   0,   0, 
     58,   0,  16,   0,   0,   0, 
      0,   0,  26,   0,  16,   0, 
      0,   0,   0,   0,  51,   0, 
      0,   7,  34,   0,  16,   0, 
      2,   0,   0,   0,  26,   0, 
     16,   0,   2,   0,   0,   0, 
      1,  64,   0,   0,   0,   0, 
      0,   0,  52,   0,   0,  10, 
    162,   0,  16,   0,   0,   0, 
      0,   0,  86,  13,  16,   0, 
      0,   0,   0,   0,   2,  64, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
     15,   0,   0,   7,  34,   0, 
     16,   0,   0,   0,   0,   0, 
    214,   5,  16,   0,   0,   0, 
      0,   0, 214,   5,  16,   0, 
      0,   0,   0,   0,  75,   0, 
      0,   5,  34,   0,  16,   0, 
      0,   0,   0,   0,  26,   0, 
     16,   0,   0,   0,   0,   0, 
      0,   0,   0,   7,  34,   0, 
     16,   0,   0,   0,   0,   0, 
     26,   0,  16,   0,   0,   0, 
      0,   0,  26,   0,  16,   0, 
      2,   0,   0,   0,  50,   0, 
      0,  10,  34,   0,  16,   0, 
      0,   0,   0,   0,  10,   0, 
     16, 128,  65,   0,   0,   0, 
      2,   0,   0,   0,   1,  64, 
      0,   0,   0,   0,   0,  63, 
     26,   0,  16,   0,   0,   0, 
      0,   0,  29,   0,   0,   7, 
    130,   0,  16,   0,   0,   0, 
      0,   0,  26,   0,  16,   0, 
      0,   0,   0,   0,   1,  64, 
      0,   0,   0,   0, 128, 191, 
      0,   0,   0,   7,  34,   0, 
     16,   0,   0,   0,   0,   0, 
     26,   0,  16,   0,   0,   0, 
      0,   0,   1,  64,   0,   0, 
      0,   0, 128,  63,  50,   0, 
      0,  10,  34,   0,  16,   0, 
      0,   0,   0,   0,  26,   0, 
     16, 128,  65,   0,   0,   0, 
      0,   0,   0,   0,   1,  64, 
      0,   0,   0,   0,   0,  63, 
      1,  64,   0,   0,   0,   0, 
    128,  63,  52,   0,   0,   7, 
     34,   0,  16,   0,   0,   0, 
      0,   0,  26,   0,  16,   0, 
      0,   0,   0,   0,   1,  64, 
      0,   0,   0,   0,   0,   0, 
     56,   0,   0,   9,  18,   0, 
     16, 128,   1,  64,   0,   0, 
      3,   0,   0,   0,  26,   0, 
     16,   0,   0,   0,   0,   0, 
     58,   0,  16, 128,   1,  64, 
      0,   0,   1,   0,   0,   0, 
     55,   0,   0,  12, 130,   0, 
     16, 128,   1,  64,   0,   0, 
      1,   0,   0,   0,  58,   0, 
     16,   0,   0,   0,   0,   0, 
     10,   0,  16, 128,   1,  64, 
      0,   0,   3,   0,   0,   0, 
     58,   0,  16, 128,   1,  64, 
      0,   0,   1,   0,   0,   0, 
      2,   0,   0,   1,   6,   0, 
      0,   3,   1,  64,   0,   0, 
      2,   0,   0,   0,  11,   0, 
      0,   5, 162,   0,  16,   0, 
      0,   0,   0,   0,   6,  20, 
     16,   0,   0,   0,   0,   0, 
     12,   0,   0,   5,  50,   0, 
     16,   0,   2,   0,   0,   0, 
     70,  16,  16,   0,   0,   0, 
      0,   0,   0,   0,   0,   9, 
    162,   0,  16,   0,   0,   0, 
      0,   0,  86,  13,  16, 128, 
    129,   0,   0,   0,   0,   0, 
      0,   0,   6,   4,  16, 128, 
    129,   0,   0,   0,   2,   0, 
      0,   0,  56,   0,   0,   7, 
     50,   0,  16,   0,   0,   0, 
      0,   0, 134,   0,  16,   0, 
      0,   0,   0,   0, 214,   5, 
     16,   0,   0,   0,   0,   0, 
     51,   0,   0,   7,  18,   0, 
     16,   0,   0,   0,   0,   0, 
     26,   0,  16,   0,   0,   0, 
      0,   0,  10,   0,  16,   0, 
      0,   0,   0,   0,  14,   0, 
      0,  10,  34,   0,  16,   0, 
      0,   0,   0,   0,   2,  64, 
      0,   0,   0,   0, 128,  63, 
      0,   0, 128,  63,   0,   0, 
    128,  63,   0,   0, 128,  63, 
     10,   0,  16,   0,   0,   0, 
      0,   0,   0,   0,   0,   9, 
     18,   0,  16, 128,   1,  64, 
      0,   0,   3,   0,   0,   0, 
     10,   0,  16, 128,   1,  64, 
      0,   0,   1,   0,   0,   0, 
      1,  64,   0,   0, 129, 128, 
      0, 191,  50,   0,   0,  10, 
     18,   0,  16,   0,   0,   0, 
      0,   0,  10,   0,  16,   0, 
      0,   0,   0,   0,   1,  64, 
      0,   0, 193, 192,  64,  61, 
     10,   0,  16, 128,   1,  64, 
      0,   0,   3,   0,   0,   0, 
     56,   0,   0,   7,  18,   0, 
     16,   0,   0,   0,   0,   0, 
     26,   0,  16,   0,   0,   0, 
      0,   0,  10,   0,  16,   0, 
      0,   0,   0,   0,  56,   0, 
      0,   7, 130,   0,  16,   0, 
      0,   0,   0,   0,  10,   0, 
     16,   0,   0,   0,   0,   0, 
      1,  64,   0,   0,   0,   0, 
     42,  65,  54,   0,   0,   8, 
    114,   0,  16,   0,   0,   0, 
      0,   0,   2,  64,   0,   0, 
      0,   0, 128,  63,   0,   0, 
    128,  63,   0,   0, 128,  63, 
      0,   0,   0,   0,  54,   0, 
      0,   6, 242,   0,  16, 128, 
      1,  64,   0,   0,   1,   0, 
      0,   0,  70,  14,  16,   0, 
      0,   0,   0,   0,   2,   0, 
      0,   1,  10,   0,   0,   1, 
      2,   0,   0,   1,  23,   0, 
      0,   1,  56,   0,   0,  10, 
    242,  32,  16, 128,   1,  64, 
      0,   0,   0,   0,   0,   0, 
     70,  14,  16, 128,   1,  64, 
      0,   0,   1,   0,   0,   0, 
     70,  30,  16, 128,   1,  64, 
      0,   0,   3,   0,   0,   0, 
     62,   0,   0,   1,  83,  70, 
     73,  48,   8,   0,   0,   0, 
     16,   0,   0,   0,   0,   0, 
      0,   0,  83,  84,  65,  84, 
    116,   0,   0,   0,  61,   0, 
      0,   0,   4,   0,   0,   0, 
      0,   0,   0,   0,   6,   0, 
      0,   0,  29,   0,   0,   0, 
      1,   0,   0,   0,   0,   0, 
      0,   0,   8,   0,   0,   0, 
      2,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      4,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   6,   0,   0,   0, 
      1,   0,   0,   0,   1,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0
};
