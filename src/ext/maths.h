#ifndef MATHS_H
#define MATHS_H

// NOTE(ljre):
//     This is nakst's public domain implementation of several math functions taken
//     from https://gist.github.com/nakst/f9c00ef6969fd6ad380bcbae2e10e64a .
//
//     I made some modifications so it compiles as C code and uses my coding conventions :P

#if defined(__clang__)
#   define MATHSDECL internal
#   pragma float_control(precise, on, push)
#elif defined(__GNUC__)
#   define MATHSDECL internal __attribute__((optimize("-fno-fast-math")))
#else
#   define MATHSDECL internal
#endif

///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////

// NOTE Compile without fast math flags.

/*
This is free and unencumbered software released into the public domain.
Anyone is free to copy, modify, publish, use, compile, sell, or
distribute this software, either in source code form or as a compiled
binary, for any purpose, commercial or non-commercial, and by any
means.
In jurisdictions that recognize copyright laws, the author or authors
of this software dedicate any and all copyright interest in the
software to the public domain. We make this dedication for the benefit
of the public at large and to the detriment of our heirs and
successors. We intend this dedication to be an overt act of
relinquishment in perpetuity of all present and future rights to this
software under copyright law.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.
For more information, please refer to http://unlicense.org/
*/

union Math_ConvertDoubleInteger_
{
	float64 d;
	uint64 i;
}
typedef Math_ConvertDoubleInteger_;

union Math_ConvertFloatInteger_
{
	float32 f;
	uint64 i;
}
typedef Math_ConvertFloatInteger_;

internal const float64 Math_double_to_integer = 1.0 / 2.22044604925031308085e-16;

MATHSDECL float64 Math_Sqrt64(float64 x);
MATHSDECL float32 Math_Sqrt(float32 x);

MATHSDECL float64
Math_Floor64(float64 x)
{
	if (x == 0)
		return x;
	
	Math_ConvertDoubleInteger_ convert = { x };
	uint64 sign = convert.i & 0x8000000000000000;
	int32 exponent = (int32) ((convert.i >> 52) & 0x7FF) - 0x3FF;
	
	if (exponent >= 52)
	{
		// There aren't any bits representing a fractional part.
		return x;
	}
	else if (exponent >= 0)
	{
		// Positive exponent.
		float64 y = sign ? (x - Math_double_to_integer + Math_double_to_integer - x) : (x + Math_double_to_integer - Math_double_to_integer - x);
		return y > 0 ? x + y - 1 : x + y;
	}
	else if (exponent < 0)
	{
		// Negative exponent.
		return sign ? -1.0 : 0.0;
	}
	
	return 0;
}

MATHSDECL float32
Math_Floor(float32 x)
{
	Math_ConvertFloatInteger_ convert = { x };
	uint32 sign = convert.i & 0x80000000;
	int32 exponent = (int32) ((convert.i >> 23) & 0xFF) - 0x7F;
	
	if (exponent >= 23)
	{
		// There aren't any bits representing a fractional part.
	}
	else if (exponent >= 0)
	{
		// Positive exponent.
		uint32 mask = 0x7FFFFF >> exponent;
		if (!(mask & convert.i))
			return x; // Already an integer.
		if (sign)
			convert.i += mask;
		convert.i &= ~mask; // Mask out the fractional bits.
	}
	else if (exponent < 0)
	{
		// Negative exponent.
		return sign ? -1.0 : 0.0;
	}
	
	return convert.f;
}

#define D(x) (((Math_ConvertDoubleInteger_) { .i = (x) }).d)
#define F(x) (((Math_ConvertFloatInteger_) { .i = (x) }).f)

MATHSDECL float64
Math_Sine64_(float64 x)
{
	// Calculates sin(x) for x in [0, pi/4].
	
	float64 x2 = x * x;
	
	return x * (D(0x3FF0000000000000) + x2 * (D(0xBFC5555555555540) + x2 * (D(0x3F8111111110ED80) + x2 * (D(0xBF2A01A019AE6000) 
																										  + x2 * (D(0x3EC71DE349280000) + x2 * (D(0xBE5AE5DC48000000) + x2 * D(0x3DE5D68200000000)))))));
}

MATHSDECL float32
Math_Sine_(float32 x)
{
	// Calculates sin(x) for x in [0, pi/4].
	
	float32 x2 = x * x;
	
	return x * (F(0x3F800000) + x2 * (F(0xBE2AAAA0) + x2 * (F(0x3C0882C0) + x2 * F(0xB94C6000))));
}

MATHSDECL float64
Math_ArcSine64_(float64 x)
{
	// Calculates arcsin(x) for x in [0, 0.5].
	
	float64 x2 = x * x;
	
	return x * (D(0x3FEFFFFFFFFFFFE6) + x2 * (D(0x3FC555555555FE00) + x2 * (D(0x3FB333333292DF90) + x2 * (D(0x3FA6DB6DFD3693A0) 
																										  + x2 * (D(0x3F9F1C608DE51900) + x2 * (D(0x3F96EA0659B9A080) + x2 * (D(0x3F91B4ABF1029100) 
																																											  + x2 * (D(0x3F8DA8DAF31ECD00) + x2 * (D(0x3F81C01FD5000C00) + x2 * (D(0x3F94BDA038CF6B00)
																																																												  + x2 * (D(0xBF8E849CA75B1E00) + x2 * D(0x3FA146C2D37F2C60))))))))))));
}

MATHSDECL float32
Math_ArcSine_(float32 x)
{
	// Calculates arcsin(x) for x in [0, 0.5].
	
	float32 x2 = x * x;
	
	return x * (F(0x3F800004) + x2 * (F(0x3E2AA130) + x2 * (F(0x3D9B2C28) + x2 * (F(0x3D1C1800) + x2 * F(0x3D5A97C0)))));
}

MATHSDECL float64
Math_ArcTangent64_(float64 x)
{
	// Calculates arctan(x) for x in [0, 0.5].
	
	float64 x2 = x * x;
	
	return x * (D(0x3FEFFFFFFFFFFFF8) + x2 * (D(0xBFD5555555553B44) + x2 * (D(0x3FC9999999803988) + x2 * (D(0xBFC249248C882E80) 
																										  + x2 * (D(0x3FBC71C5A4E4C220) + x2 * (D(0xBFB745B3B75243F0) + x2 * (D(0x3FB3AFAE9A2939E0) 
																																											  + x2 * (D(0xBFB1030C4A4A1B90) + x2 * (D(0x3FAD6F65C35579A0) + x2 * (D(0xBFA805BCFDAFEDC0)
																																																												  + x2 * (D(0x3F9FC6B5E115F2C0) + x2 * D(0xBF87DCA5AB25BF80))))))))))));
}

MATHSDECL float32
Math_ArcTangent_(float32 x)
{
	// Calculates arctan(x) for x in [0, 0.5].
	
	float32 x2 = x * x;
	
	return x * (F(0x3F7FFFF8) + x2 * (F(0xBEAAA53C) + x2 * (F(0x3E4BC990) + x2 * (F(0xBE084A60) + x2 * F(0x3D8864B0)))));
}

MATHSDECL float64
Math_Cosine64_(float64 x)
{
	// Calculates cos(x) for x in [0, pi/4].
	
	float64 x2 = x * x;
	
	return D(0x3FF0000000000000) + x2 * (D(0xBFDFFFFFFFFFFFA0) + x2 * (D(0x3FA555555554F7C0) + x2 * (D(0xBF56C16C16475C00) 
																									 + x2 * (D(0x3EFA019F87490000) + x2 * (D(0xBE927DF66B000000) + x2 * D(0x3E21B949E0000000))))));
}

MATHSDECL float32
Math_Cosine_(float32 x)
{
	// Calculates cos(x) for x in [0, pi/4].
	
	float32 x2 = x * x;
	
	return F(0x3F800000) + x2 * (F(0xBEFFFFDA) + x2 * (F(0x3D2A9F60) + x2 * F(0xBAB22C00)));
}

MATHSDECL float64
Math_Tangent64_(float64 x)
{
	// Calculates tan(x) for x in [0, pi/4].
	
	float64 x2 = x * x;
	
	return x * (D(0x3FEFFFFFFFFFFFE8) + x2 * (D(0x3FD5555555558000) + x2 * (D(0x3FC1111110FACF90) + x2 * (D(0x3FABA1BA266BFD20) 
																										  + x2 * (D(0x3F9664F30E56E580) + x2 * (D(0x3F822703B08BDC00) + x2 * (D(0x3F6D698D2E4A4C00) 
																																											  + x2 * (D(0x3F57FF4F23EA4400) + x2 * (D(0x3F424F3BEC845800) + x2 * (D(0x3F34C78CA9F61000)
																																																												  + x2 * (D(0xBF042089F8510000) + x2 * (D(0x3F29D7372D3A8000) + x2 * (D(0xBF19D1C5EF6F0000)
																																																																													  + x2 * (D(0x3F0980BDF11E8000)))))))))))))));
}

MATHSDECL float32
Math_Tangent_(float32 x)
{
	// Calculates tan(x) for x in [0, pi/4].
	
	float32 x2 = x * x;
	
	return x * (F(0x3F800001) + x2 * (F(0x3EAAA9AA) + x2 * (F(0x3E08ABA8) + x2 * (F(0x3D58EC90) 
																				  + x2 * (F(0x3CD24840) + x2 * (F(0x3AC3CA00) + x2 * F(0x3C272F00)))))));
}

MATHSDECL float64
Math_Sin64(float64 x)
{
	bool negate = false;
	
	// x in -infty, infty
	
	if (x < 0)
	{
		x = -x;
		negate = true;
	}
	
	// x in 0, infty
	
	x -= 2 * PI64 * Math_Floor64(x / (2 * PI64));
	
	// x in 0, 2*pi
	
	if (x < PI64 / 2)
	{}
	else if (x < PI64)
		x = PI64 - x;
	else if (x < 3 * PI64 / 2)
	{
		x = x - PI64;
		negate = !negate;
	}
	else
	{
		x = PI64 * 2 - x;
		negate = !negate;
	}
	
	// x in 0, pi/2
	
	float64 y = x < PI64 / 4 ? Math_Sine64_(x) : Math_Cosine64_(PI64 / 2 - x);
	return negate ? -y : y;
}

MATHSDECL float32
Math_Sin(float32 x)
{
	bool negate = false;
	
	// x in -infty, infty
	
	if (x < 0)
	{
		x = -x;
		negate = true;
	}
	
	// x in 0, infty
	
	x -= 2 * PI32 * Math_Floor(x / (2 * PI32));
	
	// x in 0, 2*pi
	
	if (x < PI32 / 2)
	{}
	else if (x < PI32)
		x = PI32 - x;
	else if (x < 3 * PI32 / 2)
	{
		x = x - PI32;
		negate = !negate;
	}
	else
	{
		x = PI32 * 2 - x;
		negate = !negate;
	}
	
	// x in 0, pi/2
	
	float32 y = x < PI32 / 4 ? Math_Sine_(x) : Math_Cosine_(PI32 / 2 - x);
	return negate ? -y : y;
}

MATHSDECL float64
Math_Cos64(float64 x)
{
	bool negate = false;
	
	// x in -infty, infty
	
	if (x < 0)
		x = -x;
	
	// x in 0, infty
	
	x -= 2 * PI64 * Math_Floor64(x / (2 * PI64));
	
	// x in 0, 2*pi
	
	if (x < PI64 / 2)
	{}
	else if (x < PI64)
	{
		x = PI64 - x;
		negate = !negate;
	}
	else if (x < 3 * PI64 / 2)
	{
		x = x - PI64;
		negate = !negate;
	}
	else
		x = PI64 * 2 - x;
	
	// x in 0, pi/2
	
	float64 y = x < PI64 / 4 ? Math_Cosine64_(x) : Math_Sine64_(PI64 / 2 - x);
	return negate ? -y : y;
}

MATHSDECL float32
Math_Cos(float32 x)
{
	bool negate = false;
	
	// x in -infty, infty
	
	if (x < 0)
		x = -x;
	
	// x in 0, infty
	
	x -= 2 * PI32 * Math_Floor(x / (2 * PI32));
	
	// x in 0, 2*pi
	
	if (x < PI32 / 2)
	{}
	else if (x < PI32)
	{
		x = PI32 - x;
		negate = !negate;
	}
	else if (x < 3 * PI32 / 2)
	{
		x = x - PI32;
		negate = !negate;
	}
	else
		x = PI32 * 2 - x;
	
	// x in 0, pi/2
	
	float32 y = x < PI32 / 4 ? Math_Cosine_(x) : Math_Sine_(PI32 / 2 - x);
	return negate ? -y : y;
}

MATHSDECL float64
Math_Tan64(float64 x)
{
	bool negate = false;
	
	// x in -infty, infty
	
	if (x < 0)
	{
		x = -x;
		negate = !negate;
	}
	
	// x in 0, infty
	
	x -= PI64 * Math_Floor64(x / PI64);
	
	// x in 0, pi
	
	if (x > PI64 / 2)
	{
		x = PI64 - x;
		negate = !negate;
	}
	
	// x in 0, pi/2
	
	float64 y = x < PI64 / 4 ? Math_Tangent64_(x) : (1.0 / Math_Tangent64_(PI64 / 2 - x));
	return negate ? -y : y;
}

MATHSDECL float32
Math_Tan(float32 x)
{
	bool negate = false;
	
	// x in -infty, infty
	
	if (x < 0)
	{
		x = -x;
		negate = !negate;
	}
	
	// x in 0, infty
	
	x -= PI32 * Math_Floor(x / PI32);
	
	// x in 0, pi
	
	if (x > PI32 / 2)
	{
		x = PI32 - x;
		negate = !negate;
	}
	
	// x in 0, pi/2
	
	float32 y = x < PI32 / 4 ? Math_Tangent_(x) : (1.0f / Math_Tangent_(PI32 / 2 - x));
	return negate ? -y : y;
}

MATHSDECL float64
Math_ArcSin64(float64 x)
{
	bool negate = false;
	
	if (x < 0)
	{
		x = -x; 
		negate = true; 
	}
	
	float64 y;
	
	if (x < 0.5)
		y = Math_ArcSine64_(x);
	else
		y = PI64 / 2 - 2 * Math_ArcSine64_(Math_Sqrt64(0.5 - 0.5 * x));
	
	return negate ? -y : y;
}

MATHSDECL float32
Math_ArcSin(float32 x)
{
	bool negate = false;
	
	if (x < 0)
	{
		x = -x; 
		negate = true; 
	}
	
	float32 y;
	
	if (x < 0.5)
		y = Math_ArcSine_(x);
	else
		y = PI32 / 2 - 2 * Math_ArcSine_(Math_Sqrt(0.5f - 0.5f * x));
	
	return negate ? -y : y;
}

MATHSDECL float64
Math_ArcCos64(float64 x)
{ return Math_ArcSin64(-x) + PI64 / 2; }

MATHSDECL float32
Math_ArcCos(float32 x)
{ return Math_ArcSin(-x) + PI32 / 2; }

MATHSDECL float64
Math_ArcTan64(float64 x)
{
	bool negate = false;
	
	if (x < 0)
	{
		x = -x; 
		negate = true; 
	}
	
	bool reciprocal_taken = false;
	
	if (x > 1)
	{
		x = 1 / x;
		reciprocal_taken = true;
	}
	
	float64 y;
	
	if (x < 0.5)
		y = Math_ArcTangent64_(x);
	else
		y = 0.463647609000806116 + Math_ArcTangent64_((2 * x - 1) / (2 + x));
	
	if (reciprocal_taken)
		y = PI64 / 2 - y;
	
	return negate ? -y : y;
}

MATHSDECL float32
Math_ArcTan(float32 x)
{
	bool negate = false;
	
	if (x < 0)
	{
		x = -x; 
		negate = true; 
	}
	
	bool reciprocal_taken = false;
	
	if (x > 1)
	{
		x = 1 / x;
		reciprocal_taken = true;
	}
	
	float32 y;
	
	if (x < 0.5f)
		y = Math_ArcTangent_(x);
	else
		y = 0.463647609000806116f + Math_ArcTangent_((2 * x - 1) / (2 + x));
	
	if (reciprocal_taken)
		y = PI32 / 2 - y;
	
	return negate ? -y : y;
}

MATHSDECL float64
Math_ArcTan264(float64 y, float64 x)
{
	if (x == 0)
		return y > 0 ? PI64 / 2 : -PI64 / 2;
	else if (x > 0)
		return Math_ArcTan64(y / x);
	else if (y >= 0)
		return PI64 + Math_ArcTan64(y / x);
	else
		return -PI64 + Math_ArcTan64(y / x);
}

MATHSDECL float32
Math_ArcTan2(float32 y, float32 x)
{
	if (x == 0)
		return y > 0 ? PI32 / 2 : -PI32 / 2;
	else if (x > 0)
		return Math_ArcTan(y / x);
	else if (y >= 0)
		return PI32 + Math_ArcTan(y / x);
	else
		return -PI32 + Math_ArcTan(y / x);
}

MATHSDECL float64
Math_Exp264(float64 x)
{
	float64 a = Math_Floor64(x * 8);
	int64 ai = (int64)a;
	
	if (ai < -1024)
		return 0;
	
	float64 b = x - a / 8;
	
	float64 y = D(0x3FF0000000000000) + b * (D(0x3FE62E42FEFA3A00) + b * (D(0x3FCEBFBDFF829140) 
																		  + b * (D(0x3FAC6B08D73C4A40) + b * (D(0x3F83B2AB53873280) + b * (D(0x3F55D88F363C6C00) 
																																		   + b * (D(0x3F242C003E4A2000) + b * D(0x3EF0B291F6C00000)))))));
	
	const float64 m[8] = {
		D(0x3FF0000000000000),
		D(0x3FF172B83C7D517B),
		D(0x3FF306FE0A31B715),
		D(0x3FF4BFDAD5362A27),
		D(0x3FF6A09E667F3BCD),
		D(0x3FF8ACE5422AA0DB),
		D(0x3FFAE89F995AD3AD),
		D(0x3FFD5818DCFBA487),
	};
	
	y *= m[ai & 7];
	
	Math_ConvertDoubleInteger_ c;
	c.d = y;
	c.i += (ai >> 3) << 52;
	return c.d;
}

MATHSDECL float32
Math_Exp2(float32 x)
{
	float32 a = Math_Floor(x);
	int32 ai = (int32)a;
	
	if (ai < -128)
		return 0;
	
	float32 b = x - a;
	
	float32 y = F(0x3F7FFFFE) + b * (F(0x3F31729A) + b * (F(0x3E75E700) 
														  + b * (F(0x3D64D520) + b * (F(0x3C128280) + b * F(0x3AF89400)))));
	
	Math_ConvertFloatInteger_ c;
	c.f = y;
	c.i += ai << 23;
	return c.f;
}

MATHSDECL float64
Math_Log264(float64 x)
{
	Math_ConvertDoubleInteger_ c;
	c.d = x;
	int64 e = ((c.i >> 52) & 2047) - 0x3FF;
	c.i = (c.i & ~(0x7FFLL << 52)) + (0x3FFLL << 52);
	x = c.d;
	
	float64 a;
	
	if (x < 1.125)
		a = 0;
	else if (x < 1.250)
	{
		x *= 1.125 / 1.250;
		a = D(0xBFC374D65D9E608E);
	}
	else if (x < 1.375)
	{
		x *= 1.125 / 1.375;
		a = D(0xBFD28746C334FECB);
	}
	else if (x < 1.500)
	{
		x *= 1.125 / 1.500;
		a = D(0xBFDA8FF971810A5E);
	}
	else if (x < 1.625)
	{
		x *= 1.125 / 1.625;
		a = D(0xBFE0F9F9FFC8932A);
	}
	else if (x < 1.750)
	{
		x *= 1.125 / 1.750;
		a = D(0xBFE465D36ED11B11);
	}
	else if (x < 1.875)
	{
		x *= 1.125 / 1.875;
		a = D(0xBFE79538DEA712F5);
	}
	else
	{
		x *= 1.125 / 2.000;
		a = D(0xBFEA8FF971810A5E);
	}
	
	float64 y = D(0xC00FF8445026AD97) + x * (D(0x40287A7A02D9353F) + x * (D(0xC03711C58D55CEE2) 
																		  + x * (D(0x4040E8263C321A26) + x * (D(0xC041EB22EA691BB3) + x * (D(0x403B00FB376D1F10) 
																																		   + x * (D(0xC02C416ABE857241) + x * (D(0x40138BA7FAA3523A) + x * (D(0xBFF019731AF80316) 
																																																			+ x * D(0x3FB7F1CD3852C200)))))))));
	
	return y - a + e;
}

MATHSDECL float32
Math_Log2(float32 x)
{
	Math_ConvertFloatInteger_ c;
	c.f = x;
	int32 e = ((c.i >> 23) & 255) - 0x7F;
	c.i = (c.i & ~(0xFF << 23)) + (0x7F << 23);
	x = c.f;
	
	float64 y = F(0xC05B5154) + x * (F(0x410297C6) + x * (F(0xC1205CEB) 
														  + x * (F(0x4114DF63) + x * (F(0xC0C0DBBB) + x * (F(0x402942C6) 
																										   + x * (F(0xBF3FF98A) + x * (F(0x3DFE1050) + x * F(0xBC151480))))))));
	
	return (float32)(y + e);
}

MATHSDECL float64
Math_Pow64(float64 x, float64 y)
{ return Math_Exp264(y * Math_Log264(x)); }

MATHSDECL float32
Math_Pow(float32 x, float32 y)
{ return Math_Exp2(y * Math_Log2(x)); }

MATHSDECL float64
Math_Mod64(float64 x, float64 y)
{ return x - y * Math_Floor64(x / y); }

MATHSDECL float32
Math_Mod(float32 x, float32 y)
{ return x - y * Math_Floor(x / y); }

MATHSDECL float64
Math_Sqrt64(float64 x)
{ return Math_Pow64(x, 0.5); }

MATHSDECL float32
Math_Sqrt(float32 x)
{ return Math_Pow(x, 0.5f); }

MATHSDECL float64
Math_Round64(float64 x)
{ return Math_Floor64(x + 0.5); }

MATHSDECL float32
Math_Round(float32 x)
{ return Math_Floor(x + 0.5f); }

MATHSDECL float64
Math_Ceil64(float64 x)
{ return -Math_Floor64(-x); }

MATHSDECL float32
Math_Ceil(float32 x)
{ return -Math_Floor(-x); }

MATHSDECL float64
Math_Abs64(float64 x)
{ return x < 0.0 ? -x : x; }

MATHSDECL float32
Math_Abs(float32 x)
{ return x < 0.0f ? -x : x; }

#undef MATHSDECL

#ifdef __clang__
#   pragma float_control(pop)
#endif

#endif //MATHS_H
