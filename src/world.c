

#include "common_math.h"
#include "common_gl.h"

#include "utilities.h"


#include "game.h"
#include "world.h"
#include "scene.h"

// temp?
#include "building.h"




void World_init(World* w) {
	
	VEC_INIT(&w->itemInstances);
	VEC_INIT(&w->partInstances);
	VEC_INIT(&w->items);
	
	
	w->lm = calloc(1, sizeof(*w->lm));
	LightManager_Init(w->lm);
	w->lightingPass = LightManager_CreateRenderPass(w->lm);
	// not static //w->lm->dtex = w->gs->depthTexBuffer;
	
	HT_init(&w->itemLookup, 4);
	
	//initMap(&w->map);
	MapInfo_Init(&w->map);
	
	w->mapTexMan = TextureManager_alloc();
	w->meshTexMan = TextureManager_alloc();
	w->decalTexMan = TextureManager_alloc();
	
	w->dmm = dynamicMeshManager_alloc(1024*50);
	w->smm = meshManager_alloc();
	w->dm = DecalManager_alloc(1024*50);
	w->cdm = CustomDecalManager_alloc(1024*50);
	

	
	w->sunShadow = ShadowMap_alloc();
	w->sunShadow->size = (Vector2i){1024, 1024};
	ShadowMap_SetupFBOs(w->sunShadow);

	
	// hack becore CES is made
	w->dmm->ces = &w->gs->ces;
	
	w->map.tm = w->mapTexMan;
	w->dmm->tm = w->meshTexMan;
	w->dm->tm = w->decalTexMan;
	w->cdm->tm = w->decalTexMan;
	
	Map_readConfigFile(&w->map, "assets/config/terrain.json");
	meshManager_readConfigFile(w->smm, "assets/config/models.json");
	dynamicMeshManager_readConfigFile(w->dmm, "assets/config/models.json");
	DecalManager_readConfigFile(w->dm, "assets/config/decals.json");
	
	w->emitters = makeEmitter();
	
	loadItemConfig(w, "assets/config/items.json");
	
	
	
	// old bezier roads
	//w->roads = calloc(1, sizeof(*w->roads));
	//initRoadBlock(w->roads);
	
	//RoadControlPoint rcp = {
		//{5,5},
		//{50,50},
		//{40,7}
	//};
	//int id;
	//rbAddRoad(w->roads, &rcp, &id);
	
	//roadblock_update_vbo(w->roads);
	
	// water plane, temporary hack
	w->wp = calloc(1, sizeof(*w->wp));
	WaterPlane_create(w->wp, 200, &(Vector){0,0,0});
	
	Pipe_init(&w->testmesh);
	
	// hack to test lightmanager
	LightManager_AddPointLight(w->lm, (Vector){10,10, 10}, 200, 20);
	
	
	// -------- building test ------------
	
	
	Building b;
	VEC_INIT(&b.outlines);
	VEC_INIT(&b.vertices);
	VEC_INIT(&b.indices);
	
	
	BuildingOutline* bo;
	bo = BuildingOutline_rect(0, 30, (Vector2){0,0}, (Vector2){10,10});
	VEC_PUSH(&b.outlines, bo);
	bo = BuildingOutline_rect(0, 20, (Vector2){-6,0}, (Vector2){2,10});
	VEC_PUSH(&b.outlines, bo);
	bo = BuildingOutline_rect(0, 20, (Vector2){6,0}, (Vector2){2,10});
	VEC_PUSH(&b.outlines, bo);
	bo = BuildingOutline_rect(0, 10, (Vector2){-8,0}, (Vector2){2,10});
	VEC_PUSH(&b.outlines, bo);
	bo = BuildingOutline_rect(0, 10, (Vector2){8,0}, (Vector2){2,10});
	VEC_PUSH(&b.outlines, bo);
	
	Building_extrudeAll(&b);
	
// 	VEC_EACH(&b.vertices, i, v) {
// 		printf("~%d [%.2f,%.2f,%.2f]\n", i, v.p.x,v.p.y,v.p.z);
// 	}
	
	Building_capAll(&b); // causes memory corruption
	
	
	int building_ind = dynamicMeshManager_addMesh(w->dmm, "building", Building_CreateDynamicMesh(&b));
	

	
	
	
	// -----------------------------------
	
	meshManager_updateGeometry(w->smm);
	dynamicMeshManager_updateGeometry(w->dmm);
	

	DynamicMeshInstance inst = {
		pos: {100, 100, 30},
		scale: 30,
		dir: {0, 0, 1},
		rot: 0,
		alpha: 0.5,
		texIndex: 2,
	};
	dynamicMeshManager_addInstance(w->dmm, building_ind, &inst);
	//printf("^^^^ %d\n", building_ind);
	Vector v = {50,50,0};
	World_spawnAt_DynamicMesh(w, building_ind, &v);
	
	
	// very last thing: load textures
	TextureManager_loadAll(w->mapTexMan, (Vector2i){256, 256}); 
	TextureManager_loadAll(w->meshTexMan, (Vector2i){256, 256}); 
	TextureManager_loadAll(w->decalTexMan, (Vector2i){256, 256}); 


	// terrain pass
	w->terrainPass = Map_CreateRenderPass(&w->map);
	
	// solids pass
	w->solidsPass = DynamicMeshManager_CreateRenderPass(w->dmm);
	RenderPass_addDrawable(w->solidsPass, Emitter_CreateDrawable(w->emitters));


	// decals pass
	w->decalPass = DecalManager_CreateRenderPass(w->dm);
	
	
	RenderPass_addDrawable(w->decalPass, CustomDecalManager_CreateDrawable(w->cdm));
	
	
	w->roads = RoadNetwork_alloc();
	
	
	
	
	
	
	
	
	
	/// hacks
	
	for(int i = 0; i < 0; i++) {
		Vector v = {
			.x = 10, //frand(0, 500),
			.y = 10, //frand(0, 500),
			.z = 30,
		};
		
		//World_spawnAt_Item(w, "tree", &v);
		World_spawnAt_DynamicMesh(w,  4, &v);
	}
	
	CustomDecal* cd = pcalloc(cd); 
	cd->thickness = 9.0f;
	
	CustomDecalManager_AddDecal(w->cdm, "test", cd);
	
	Matrix rm;
	
	mIdent(&rm);
	mTrans3f(100, 100, 0, &rm);
	mRotZ(1, &rm);

	Vector pos1 = {40, 55, 20};
	Vector pos2 = {40, 95, 20};
	Vector pos3 = {100, 30, 20};
	Vector pos4 = {90, 110, 20};
	
	vMatrixMul(&pos1, &rm, &pos1);
	vMatrixMul(&pos2, &rm, &pos2);
	vMatrixMul(&pos3, &rm, &pos3);
	vMatrixMul(&pos4, &rm, &pos4);
	
	CustomDecalManager_AddInstance(w->cdm, 0, &(CustomDecalInstance){
		.pos1 = pos1,
		.pos2 = pos2,
		.pos3 = pos3,
		.pos4 = pos4,
		.thickness = 5,
		.tex12 = .5,
		.tex34 = 2,
	});
	
	
	
	World_spawnAt_CustomDecal(w, 0, 50, &(Vector2){100, 100}, &(Vector2){300, 300});

	
}



static Item* findItem(World* w, char* itemName) {
	int64_t index;
	
	if(HT_get(&w->itemLookup, itemName, &index)) {
		fprintf(stderr, "!!! item not found: '%s'\n", itemName);
		return -1;
	}
	
	//printf("index of %s:%d\n", itemName, index);
	return VEC_DATA(&w->items)[index];
}


static ItemInstance* allocItemInstance(Item* item) {
	ItemInstance* inst;
	
	inst = calloc(1, sizeof(*inst) + (sizeof(inst->parts[0]) * item->numParts));
	CHECK_OOM(inst);
	
	inst->item = item;
	
	return inst;
}

static int spawnPart(World* w, ItemPart* part, Vector* center) {
	Vector loc;
	
	vAdd(center, &part->offset, &loc);
	
	switch(part->type) {
		case ITEM_TYPE_DYNAMICMESH:
			return World_spawnAt_DynamicMesh(w, part->index, &loc);
		
		case ITEM_TYPE_STATICMESH:
			return World_spawnAt_StaticMesh(w, part->index, &loc);
		
		case ITEM_TYPE_EMITTER:
			return World_spawnAt_Emitter(w, part->index, &loc);

		case ITEM_TYPE_LIGHT:
			return World_spawnAt_Light(w, part->index, &loc);

		case ITEM_TYPE_DECAL:
			return World_spawnAt_Decal(w, part->index, &loc);
	
		default:
			printf("unknown part item type: %d\n", part->type);
	}
}



int World_spawnAt_Item(World* w, char* itemName, Vector* location) {
	int i;
	Item* item;
	ItemInstance* inst;
	
	item = findItem(w, itemName);
	if(item < 0) {
		printf("returning \n");
		return 1;
	}
	
	
	inst = allocItemInstance(item);
	
	for(i = 0; i < item->numParts; i++) {
		//printf("trying to spawn %d : %d\n", item->parts[i].index, i);
		spawnPart(w, &item->parts[i], location);
		
	}
	
}


int World_spawnAt_DynamicMesh(World* w, int dmIndex, Vector* location) {
	DynamicMeshInstance dmi;
	float h;
	
	Vector2i loci;
	//printf("dynamic mesh spawn");
	loci.x = location->x;
	loci.y = location->y;
	
	// look up the height there.
	//getTerrainHeight(&w->map, &loci, 1, &h);
	h = Map_getTerrainHeight(&w->map, loci);
	
	//printf("map h at [%.1f, %.1f]: %.4f\n", location->x, location->y, h);
	
	// spawn instance
	dmi.pos.x = location->x;
	dmi.pos.y = location->y;
	dmi.pos.z = h + .5; // HACK +.5z for gazebo origin offset
	
	dmi.scale = 1;
	
	dmi.dir = (Vector){1, 0, 0};
	dmi.rot = 0;// F_PI / 5.0;
	
	dmi.alpha = 1.0;
	
	dynamicMeshManager_addInstance(w->dmm, dmIndex, &dmi);
	// note: dynamic mesh instances are updated every frame automatically
	
	uint16_t dmindex16 = dmIndex;
	uint32_t eid = newEID();
	CES_addComponentName(&w->gs->ces, "position", eid, &dmi.pos);
	CES_addComponentName(&w->gs->ces, "meshIndex", eid, &dmindex16);
	
	C_Rotation r = {
		{0, 0, 1},
		frand(0, 3.28)
	};
	CES_addComponentName(&w->gs->ces, "rotation", eid, &r);
	
	float av = frand(-2, 2);
	CES_addComponentName(&w->gs->ces, "angularVelocity", eid, &av);
	
	C_PathFollow pf = {
		.path = Path_makeRandomLoop(&dmi.pos, 50, 10, .01),
		.distTravelled = frand(0, 10000),
		.speed = frand(5, 100)
	};
//	CES_addComponentName(&w->gs->ces, "pathFollow", eid, &pf);
}

int World_spawnAt_StaticMesh(World* w, int smIndex, Vector* location) {
	StaticMeshInstance smi;
	float h;
	
	Vector2i loci;
	
	//printf("static mesh spawn");
	loci.x = location->x;
	loci.y = location->y;
	
	// look up the height there.
	//getTerrainHeight(&w->map, &loci, 1, &h);
	h = Map_getTerrainHeight(&w->map, loci);
	
	//printf("map h at [%.1f, %.1f]: %.4f\n", location->x, location->y, h);
	
	// spawn instance
	smi.pos.x = location->x;
	smi.pos.y = location->y;
	smi.pos.z = h + .5; // HACK +.5z for gazebo origin offset
	
	smi.scale = 1;
	
	smi.dir = (Vector){1, 0, 0};
	smi.rot = F_PI / 2.0;
	
	smi.alpha = 1.0;
	
	meshManager_addInstance(w->smm, smIndex, &smi);
	meshManager_updateInstances(w->smm);
	
}

int World_spawnAt_Emitter(World* w, int emitterIndex, Vector* location) {
	float h;
	
	Vector2i loci;
	
	loci.x = location->x;
	loci.y = location->y;
	
	// look up the height there.
	//getTerrainHeight(&w->map, &loci, 1, &h);
	h = Map_getTerrainHeight(&w->map, loci);
	
	// printf("map h at [%.1f, %.1f]: %.4f\n", location->x, location->y, h);
	// emitter
	EmitterInstance inst = {
		.pos = (Vector){location->x, location->y, h + 2},
		.scale = 10,
		.start_time = 0,
		.lifespan = 1<<15
	};
	
	emitterAddInstance(w->emitters, &inst);
	emitter_update_vbo(w->emitters);
	
	
}

int World_spawnAt_Light(World* w, int lightIndex, Vector* location) {
	float h;
	
	Vector2i loci;
	Vector groundloc;
	
	loci.x = location->x;
	loci.y = location->y;
	// look up the height there.
	//getTerrainHeight(&w->map, &loci, 1, &h);
	h = Map_getTerrainHeight(&w->map, loci);
	
	groundloc = (Vector){location->x, location->y, h};

	//printf("spawning light %f,%f,%f\n", groundloc.x, groundloc.y, groundloc.z);

	
	LightManager_AddPointLight(w->lm, groundloc, 10.0, 0.02);
	
}

int World_spawnAt_Decal(World* w, int index, Vector* location) {
	float h;
	
	Vector2i loci;
	Vector groundloc;
	
	loci.x = location->x;
	loci.y = location->y;
	// look up the height there.
	//getTerrainHeight(&w->map, &loci, 1, &h);
	h = Map_getTerrainHeight(&w->map, loci);
	
	groundloc = (Vector){location->x, location->y, h};

	//printf("spawning decal %f,%f,%f\n", groundloc.x, groundloc.y, groundloc.z);

	DecalInstance di;
	
	di.pos = groundloc;
	
	DecalManager_AddInstance(w->dm, index, &di);
	
}


int World_spawnAt_CustomDecal(World* w, int texIndex, float width, const Vector2* p1, const Vector2* p2) {
	float h;
	
	Vector2i loci;
	Vector groundloc;
	CustomDecalInstance di;
	
	// look up the terrain heights
	//loci.x = p1->x;
	//loci.y = p1->y;	
	//getTerrainHeight(&w->map, &loci, 1, &h);
	//groundloc = (Vector){p1->x, p1->y, h};
	
	//loci.x = p2->x;
	//loci.y = p2->y;
	//getTerrainHeight(&w->map, &loci, 1, &h);
	//groundloc = (Vector){p2->x, p2->y, h};

	//printf("spawning decal %f,%f,%f\n", groundloc.x, groundloc.y, groundloc.z);

	Vector2 n;
	float hw = width / 2;
	
	Vector2 p12;	
	vSub2(p2, p1, &p12);
	
	n = (Vector2){p12.y, -p12.x};
	vNorm2(&n, &n);
	vScale2(&n, hw, &n);
	
	vAdd(&n, p1, &di.pos1);
	vAdd(&n, p2, &di.pos3);
	
	vScale(&n, -1, &n);
	vAdd(&n, p1, &di.pos2);
	vAdd(&n, p2, &di.pos4);
	
	di.pos1.z = Map_getTerrainHeight3f(&w->map, di.pos1);
	di.pos2.z = Map_getTerrainHeight3f(&w->map, di.pos2);
	di.pos3.z = Map_getTerrainHeight3f(&w->map, di.pos3);
	di.pos4.z = Map_getTerrainHeight3f(&w->map, di.pos4);
	
	di.thickness = 50;
	
	di.tex12 = .5;
	di.tex34 = 3.0;
	//di.pos = groundloc;
	
	CustomDecalManager_AddInstance(w->cdm, texIndex, &di);
	
}



void World_spawnAt_Road(World* w, Vector2* start,  Vector2* stop) {
	
	
	RoadNode* n, *n2;
	
	//printf("start/stop: %f,%f / %f,%f\n", start->x, start->y, stop->x, stop->y);
	
	int i1 = RoadNetwork_GetNodeRadius(w->roads, start, 10);
	int i2 = RoadNetwork_GetNodeRadius(w->roads, stop, 10);
	
	if(i1 == -1) {
		n = pcalloc(n);
		n->pos = *start;
		i1 = RoadNetwork_AddNode(w->roads, n);
	}
	
	if(i2 == -1 || i1 == i2) { // duplicate nodes can cause a math segfault deeper in the decal calculation
		n2 = pcalloc(n2);
		n2->pos = *stop;
		i2 = RoadNetwork_AddNode(w->roads, n2);
	}
		
	Road_AddEdge1Way(w->roads, i1, i2);
	
	RoadNetwork_FlushDirty(w->roads, w);
	
	
	/* old bezier roads
	
	RoadControlPoint rcp = {
		{start->x, start->y},
		{stop->x, stop->y},
		{(start->x + stop->x) / 2 , (stop->y + start->y) / 2}
	};
	int id;
	rbAddRoad(w->roads, &rcp, &id);
	
	roadblock_update_vbo(w->roads);
	
	*/
	
}



void World_drawTerrain(World* w, PassFrameParams* pfp) {
	
	RenderPass_preFrameAll(w->terrainPass, pfp);
	RenderPass_renderAll(w->terrainPass, pfp->dp);
	RenderPass_postFrameAll(w->terrainPass);
}



void World_drawSolids(World* w, PassFrameParams* pfp) {
	//meshManager_draw(w->smm, view, proj);
	
	RenderPass_preFrameAll(w->solidsPass, pfp);
	RenderPass_renderAll(w->solidsPass, pfp->dp);
	RenderPass_postFrameAll(w->solidsPass);
	
	//glBlendFuncSeparatei(1, GL_SRC_COLOR, GL_ZERO, GL_SRC_ALPHA, GL_ZERO);
	//glBlendFuncSeparatei(1, GL_ONE, GL_ZERO, GL_SRC_ALPHA,  GL_ONE_MINUS_DST_COLOR);
	//glBlendFuncSeparatei(1, GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
	glBlendFuncSeparatei(1, GL_ONE, GL_ZERO, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
	WaterPlane_draw(w->wp, pfp->dp->mWorldView, pfp->dp->mViewProj);
	
}



void World_drawDecals(World* w, PassFrameParams* pfp) {
	
	// old bezier roads
	//drawRoad(w->roads, w->gs->depthTexBuffer, pfp->dp->mWorldView, pfp->dp->mViewProj);
	
	RenderPass_preFrameAll(w->decalPass, pfp);
	RenderPass_renderAll(w->decalPass, pfp->dp);
	RenderPass_postFrameAll(w->decalPass);
	
}











/*


depth buf              w         -        w         r         -        -
      tex              -         r        -         -         r        r

light buf              w?        w?       w?        w?        w        -
      tex              -         -        -         -         -        r
       
         separate     full     depth    depth     depth      depth    GBuf
           FBOs       GBuf      tex     write     read       tex      tex
       |-----------|---------|--------|--------,----------|--------|---------|
         prepasses   terrain   decals   solids   emitters   lights   shading




shadows and reflections


*/














