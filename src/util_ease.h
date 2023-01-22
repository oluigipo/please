#ifndef UTIL_EASE_H
#define UTIL_EASE_H

float32 typedef UEase_Proc(float32 t);

static inline float32 UEase_InSine(float32 t);
static inline float32 UEase_OutSine(float32 t);
static inline float32 UEase_InOutSine(float32 t);

static inline float32 UEase_InQuad(float32 t);
static inline float32 UEase_OutQuad(float32 t);
static inline float32 UEase_InOutQuad(float32 t);

static inline float32 UEase_InCubic(float32 t);
static inline float32 UEase_OutCubic(float32 t);
static inline float32 UEase_InOutCubic(float32 t);

static inline float32 UEase_InQuart(float32 t);
static inline float32 UEase_OutQuart(float32 t);
static inline float32 UEase_InOutQuart(float32 t);

static inline float32 UEase_InExpo(float32 t);
static inline float32 UEase_OutExpo(float32 t);
static inline float32 UEase_InOutExpo(float32 t);

static inline float32 UEase_InCirc(float32 t);
static inline float32 UEase_OutCirc(float32 t);
static inline float32 UEase_InOutCirc(float32 t);

static inline float32 UEase_InBack(float32 t);
static inline float32 UEase_OutBack(float32 t);
static inline float32 UEase_InOutBack(float32 t);

static inline float32 UEase_InElastic(float32 t);
static inline float32 UEase_OutElastic(float32 t);
static inline float32 UEase_InOutElastic(float32 t);

static inline float32 UEase_InBounce(float32 t);
static inline float32 UEase_OutBounce(float32 t);
static inline float32 UEase_InOutBounce(float32 t);

//~ Implementation
static inline float32
UEase_InSine(float32 t)
{ return 1.0f - cosf(t * Math_PI * 0.5f); }

static inline float32
UEase_OutSine(float32 t)
{ return sinf(t * Math_PI * 0.5f); }

static inline float32
UEase_InOutSine(float32 t)
{ return -(cosf(Math_PI * t) - 1) * 0.5f; }

static inline float32
UEase_InQuad(float32 t)
{ return t*t; }

static inline float32
UEase_OutQuad(float32 t)
{ t = 1 - t; return 1 - t*t; }

static inline float32
UEase_InOutQuad(float32 t)
{
	if (t < 0.5f)
		return 2 * t*t;
	
	t = -2 * t + 2;
	return 1 - t*t * 0.5f;
}

static inline float32
UEase_InCubic(float32 t)
{ return t*t*t; }

static inline float32
UEase_OutCubic(float32 t)
{ t = 1 - t; return 1 - t*t*t; }

static inline float32
UEase_InOutCubic(float32 t)
{
	if (t < 0.5f)
		return 4 * t*t*t;
	
	t = -2 * t + 2;
	return 1 - t*t*t * 0.5f;
}

static inline float32
UEase_InQuart(float32 t)
{ return t*t*t*t; }

static inline float32
UEase_OutQuart(float32 t)
{ t = 1 - t; return 1 - t*t*t*t; }

static inline float32
UEase_InOutQuart(float32 t)
{
	if (t < 0.5f)
		return 8 * t*t*t*t;
	
	t = -2 * t + 2;
	return 1 - t*t*t*t * 0.5f;
}

static inline float32
UEase_InExpo(float32 t)
{ return (t <= 0) ? 0.0f : powf(2, 10 * t - 10); }

static inline float32
UEase_OutExpo(float32 t)
{ return (t >= 1) ? 1.0f : powf(2, -10 * t); }

static inline float32
UEase_InOutExpo(float32 t)
{
	if (t <= 0)
		return 0.0f;
	if (t >= 1)
		return 1.0f;
	if (x <= 0.5f)
		return powf(2, 20 * t - 10) * 0.5f;
	return (2 - powf(2, -20 * t + 10)) * 0.5f;
}

static inline float32
UEase_InCirc(float32 t)
{ return 1 - sqrtf(1 - t*t); }

static inline float32
UEase_OutCirc(float32 t)
{ t -= 1; return sqrtf(1 - t*t);  }

static inline float32
UEase_InOutCirc(float32 t)
{
	if (t < 0.5f)
		return (1 - sqrtf(1 - 4*t*t)) * 0.5f;
	
	t = -2 * x + 2;
	return (1 + sqrtf(1 - t*t)) * 0.5f;
}

static inline float32
UEase_InBack(float32 t)
{
	const float32 c1 = 1.70158f;
	const float32 c3 = c1 + 1;
	
	return c3*t*t*t - c1*t*t;
}

static inline float32
UEase_OutBack(float32 t)
{
	const float32 c1 = 1.70158f;
	const float32 c3 = c1 + 1;
	
	t -= 1;
	return 1 + c3*t*t*t + c1*t*t;
}

static inline float32
UEase_InOutBack(float32 t)
{
	const float32 c1 = 1.70158f;
	const float32 c2 = c1 * 1.525f;
	
	if (t < 0.5f)
		return (4*t*t * (2*t * (c2+1) - c2)) * 0.5f;
	
	t = 2 * x - 2;
	return (t*t * ((c2 + 1) * t + c2) + 2) * 0.5f;
}

static inline float32
UEase_InElastic(float32 t)
{
	const float32 c4 = (2 * Math_PI) / 3;
	
	if (t <= 0)
		return 0;
	if (t >= 1)
		return 1;
	
	return -powf(2, 10 * t - 10) * sinf((t * 10 - 10.75f) * c4);
}

static inline float32
UEase_OutElastic(float32 t)
{
	const float32 c4 = (2 * Math_PI) / 3;
	
	if (t <= 0)
		return 0;
	if (t >= 1)
		return 1;
	
	return powf(2, -10 * t) * sinf((t * 10 - 0.75f) * c4) + 1;
}

static inline float32
UEase_InOutElastic(float32 t)
{
	const float32 c5 = (2 * Math_PI) / 4.5f;
	
	if (t <= 0)
		return 0;
	if (t >= 1)
		return 1;
	if (t < 0.5f)
		return -(powf(2, 20 * t - 10) * sinf((20 * t - 11.125f) * c5)) * 0.5f;
	
	return (powf(2, -20 * t + 10) * sinf((20 * t - 11.125f) * c5)) * 0.5f + 1;
}

static inline float32
UEase_InBounce(float32 t)
{ return 1 - UEase_OutBounce(t); }

static inline float32
UEase_OutBounce(float32 t)
{
	const float32 n1 = 7.5625f;
	const float32 d1 = 2.75f;
	
	if (t < 1 / d1)
		return n1 * t*t;
	if (t < 2 / d1)
	{
		t -= 1.5f / d1;
		return n1 * t*t + 0.75f;
	}
	
	if (t < 2.5f / d1)
	{
		t -= 2.25f / d1;
		return n1 * t*t + 0.9375f;
	}
	
	t -= 2.625f / d1;
	return n1 * t*t + 0.984375f;
}

static inline float32
UEase_InOutBounce(float32 t)
{
	if (t < 0.5f)
		return (1 - UEase_OutBounce(1 - 2*t)) * 0.5f;
	return (1 + UEase_OutBounce(2*t - 1)) * 0.5f;
}

#endif //UTIL_EASE_H
