#ifndef INTERNAL_2DSCENE_H
#define INTERNAL_2DSCENE_H

struct Render_Tilemap2D
{
	mat4 transform;
	vec4 color;
	vec2 cell_uv_size;
	int32 width, height;
	uint16* indices;
}
typedef Render_Tilemap2D;

struct Render_Sprite2D
{
	mat4 transform;
	vec4 texcoords;
	vec4 color;
}
typedef Render_Sprite2D;

struct Render_Layer2D
{
	Asset_Texture* texture;
	int32 sprite_count, tilemap_count;
	
	Render_Sprite2D* sprites;
	Render_Tilemap2D* tilemaps;
}
typedef Render_Layer2D;

#endif //INTERNAL_2DSCENE_H
