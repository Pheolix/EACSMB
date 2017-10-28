#ifndef __EACSMB_world_h__
#define __EACSMB_world_h__


#include "ds.h"
#include "map.h"
#include "road.h"
#include "pipe.h"
#include "emitter.h"
#include "staticMesh.h"
#include "dynamicMesh.h"


struct GameState;

enum ItemTypes {
	ITEM_TYPE_UNKNOWN = 0,
	ITEM_TYPE_STATICMESH,
	ITEM_TYPE_DYNAMICMESH,
	ITEM_TYPE_EMITTER
};


typedef struct {
	int type;
	int index;
	Vector offset;
} ItemPart;

typedef struct {
	char* name;
	int numParts;
	ItemPart* parts;
} Item;



typedef struct {
	ItemPart* part;
	int parentItemInst;
} PartInstance;

typedef struct {
	Item* item;
	Vector pos;
	PartInstance parts[];
} ItemInstance;



struct OrientData {
	Vector pos;
	float scale;
	
	Vector dir;
	float alpha;
	
	Vector rotAxis;
	float rot;
	
};

struct ItemFlags {
	unsigned char shouldDelete : 1; 
	unsigned char wasDeleted : 1; 
	
};


typedef struct SimpleKinematics {
	Vector velocity;
	float angularVelocity;
	
	Vector acceleration;
	float angularAcceleration;
	
	Vector axisOfRotation;
	
} SimpleKinematics;



static const uint32_t ITEM_BASE_IDS[] = {
	[ITEM_TYPE_UNKNOWN] =     4000000000,
	[ITEM_TYPE_STATICMESH] =  0000000000,
	[ITEM_TYPE_DYNAMICMESH] = 1000000000,
};

static inline uint32_t itemBaseID(enum ItemTypes e) {
	return ITEM_BASE_IDS[e];
}


// World is the entire world's contents. Scene is the part you can see.
// eventually, probably, the graphics data will be moved into Scene

// no drawing is handled by the World code. It is for managment and operations only.
// drawing is too complicated and crosses too many lines to fit here.
typedef struct World {
	struct GameState* gs; // pointer to parent
	
	MeshManager* smm;
	DynamicMeshManager* dmm;
	Emitter* emitters;
	
	RoadBlock* roads;
	
	MapInfo map;
	
	VEC(Item*) items;
	HashTable(int) itemLookup;
	
	VEC(ItemInstance*) itemInstances;
	VEC(PartInstance*) partInstances;
	
	VEC(uint32_t) itemOrientKeys;
	
	VEC(struct OrientData) staticOrients[2];
	
	int curOrient;
	VEC(struct OrientData) orients[2];
	
	VEC(uint32_t) simpleKinematicsKeys;
	VEC(struct SimpleKinematics) simpleKinematics;
	
	
	VEC(struct ItemFlags) flags;
	
	
	
	PipeSegment testmesh;
	
	
} World;







void World_drawTerrain(World* w);
void World_drawSolids(World* w, Matrix* view, Matrix* proj);
void World_drawDecals(World* w, Matrix* view, Matrix* proj);


int World_spawnAt_DynamicMesh(World* w, int dmIndex, Vector* location);
int World_spawnAt_StaticMesh(World* w, int smIndex, Vector* location);
void World_spawnAt_Road(World* w, Vector2* start,  Vector2* stop);


void loadItemConfig(World* w, char* path);



#endif // __EACSMB_world_h__
