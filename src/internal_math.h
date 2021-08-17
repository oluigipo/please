#ifndef INTERNAL_MATH_H
#define INTERNAL_MATH_H

struct QuadI32
{
	int32 x, y;
	int32 width, height;
} typedef QuadI32;

struct QuadF32
{
	float32 x, y;
	float32 width, height;
} typedef QuadF32;

// NOTE(ljre): Example API
#define Quad_Area(quad) ((quad).width * (quad).height)

#endif //INTERNAL_MATH_H
