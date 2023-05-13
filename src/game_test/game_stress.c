typedef uint32 G_Unit;

typedef uint8 G_BlockID;
enum G_BlockID
{
	G_BlockID_Null = 0,
	G_BlockID_Stone,
	G_BlockID_Grass,
};

struct G_Chunk
{
	G_Unit x, y;
	G_BlockID block[32][128][32];
}
typedef G_Chunk;

struct G_StressState
{
	G_Chunk chunks[4];
};

static void
G_StressInit(void)
{
	G_StressState* s = ArenaPushStruct(engine->persistent_arena, G_StressState);
	game->stress = s;
	
	
}

static void
G_StressUpdateAndRender(void)
{
	G_StressState* s = game->stress;
	
	
}
