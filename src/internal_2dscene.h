#ifndef INTERNAL_2DSCENE_H
#define INTERNAL_2DSCENE_H

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
	
	int32 sprite_count;
	Render_Sprite2D* sprites;
}
typedef Render_Layer2D;

#endif //INTERNAL_2DSCENE_H
