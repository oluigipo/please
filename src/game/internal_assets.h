#ifndef INTERNAL_ASSETS_H
#define INTERNAL_ASSETS_H

#include "ext/stb_truetype.h"

struct Asset_Font
{
	stbtt_fontinfo info;
	void* data;
	uintsize data_size;
	int32 ascent, descent, line_gap;
}
typedef Asset_Font;

struct Asset_SoundBuffer
{
	int32 channels;
	int32 sample_rate;
	int32 sample_count;
	int16* samples;
}
typedef Asset_SoundBuffer;

struct Asset_Material
{
	vec4 base_color;
	vec3 ambient;
	uint32 diffuse;
	uint32 specular;
	uint32 normal;
	float32 shininess;
}
typedef Asset_Material;

struct Asset_3DModel
{
	mat4 transform;
	Asset_Material material;
	
	uint32 vbo, vao, ebo;
	uint32 index_type;
	uint32 prim_mode;
	
	int32 index_count;
} typedef Asset_3DModel;

struct Asset_Texture
{
	uint32 id;
	int32 width, height;
	int32 depth; // >0 for texture arrays
}
typedef Asset_Texture;

#endif //INTERNAL_ASSETS_H
