


#include "pipe.h"
#include "meshBuilder.h"





void initPipes() {
	
	
	
}



void Pipe_init(PipeSegment* ps) {
	MeshData*  md;
	StaticMesh* sm;
	int i;
	
	
	ps->length = 200;
	
	//md = mdcreate();
	//createCylinder(40, ps->length, 20, md);
	//createTetrahedron(10, md);
	
	
	md = meshBuilder_test();
	
	
	// initialize a static mesh object and fill it with the mesh data
	sm = calloc(1, sizeof(*sm));
	CHECK_OOM(sm);

	sm->vertexCnt = VEC_LEN(&md->verts);
	sm->indexCnt = VEC_LEN(&md->indices);
	sm->indexWidth = 2;
	
	printf("verts: %d, indices: %d \n", sm->vertexCnt, sm->indexCnt);
	
	// alloc space in static mesh
	sm->vertices = malloc(sizeof(*sm->vertices) * sm->vertexCnt);
	CHECK_OOM(sm->vertices);
	sm->indices.w16 = malloc(sizeof(*sm->indices.w16) * sm->indexCnt);
	CHECK_OOM(sm->indices.w16);
	
	// copy data
	for(i = 0; i < sm->vertexCnt; i++) sm->vertices[i] = *((StaticMeshVertex*)&VEC_ITEM(&md->verts, i));
	for(i = 0; i < sm->indexCnt; i++) sm->indices.w16[i] = VEC_ITEM(&md->indices, i);
	
	/*
	for(i = 0; i < sm->vertexCnt; i++) {
		Vector* v = &sm->vertices[i].v;
		printf("%d - [%.2f, %.2f, %.2f]\n", i, v->x, v->y, v->z);
		uint16_t* t = &sm->vertices[i].t;
		printf("%d - [%d, %d]\n", i, t[0], t[1]);
	}

	for(i = 0; i < sm->indexCnt; i+=3) {
		short* v = &sm->indices.w16[i];
		printf("face %d - [%d, %d, %d]\n", i / 3, v[0], v[1], v[2]);
	}
	*/
	
	StaticMesh_updateBuffers(sm);
	
	ps->sm = sm;
	
	
	
}









