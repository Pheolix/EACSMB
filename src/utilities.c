
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <math.h>
#include <time.h>

#include <unistd.h>
#include <dirent.h>

#include <GL/glew.h>
#include <GL/glx.h>
#include <GL/glu.h>



#include "utilities.h"



// time code

double getCurrentTime() { // in seconds
	double now;
	struct timespec ts;
	static double offset = 0;
	
	// CLOCK_MONOTONIC_RAW is linux-specific.
	clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
	
	now = (double)ts.tv_sec + ((double)ts.tv_nsec / 1000000000.0);
	if(offset == 0) offset = now;
	
	return now - offset;
}

double timeSince(double past) {
	double now = getCurrentTime();
	return now - past;
}



// GPU timers

void query_queue_init(QueryQueue* q) {
	glGenQueries(6, q->qids);
	q->head = 0;
	q->used = 0;
}

void query_queue_start(QueryQueue* q) {
	if(q->used < 6) {
		glexit("");
		glBeginQuery(GL_TIME_ELAPSED, q->qids[q->head]);
		glexit("");
		q->head = (q->head + 1) % 6;
		q->used++;
	}
	else {
		fprintf(stderr, "query queue exhausted \n");
	}
}

void query_queue_stop(QueryQueue* q) {
	glEndQuery(GL_TIME_ELAPSED);
}

int query_queue_try_result(QueryQueue* q, uint64_t* time) {
	uint64_t p;
	int tail;
	
	if(q->used == 0) {
		return 2;
	}
	
	tail = (q->head - q->used + 6) % 6; 
	
	glGetQueryObjectui64v(q->qids[tail], GL_QUERY_RESULT_AVAILABLE, &p);
	if(GL_FALSE == p) {
		return 1; // the query isn't ready yet
	}
	
	glGetQueryObjectui64v(q->qids[tail], GL_QUERY_RESULT, time); 
	q->used--;
	
	return 0;
}

int tryQueryTimer(GLuint id, uint64_t* time) {
	uint64_t p;
	
	glGetQueryObjectui64v(id, GL_QUERY_RESULT_AVAILABLE, &p);
	if(GL_TRUE == p) { 
		glGetQueryObjectui64v(id, GL_QUERY_RESULT, time); 
		return 0;
	}
	
	return 1;
}



char* readFile(char* path, int* srcLen) {
	
	int fsize;
	char* contents;
	FILE* f;
	
	
	f = fopen(path, "rb");
	if(!f) {
		fprintf(stderr, "Could not open file \"%s\"\n", path);
		return NULL;
	}
	
	fseek(f, 0, SEEK_END);
	fsize = ftell(f);
	rewind(f);
	
	contents = (char*)malloc(fsize + 2);
	
	fread(contents+1, sizeof(char), fsize, f);
	contents[0] = '\n';
	contents[fsize] = 0;
	
	fclose(f);
	
	if(srcLen) *srcLen = fsize + 1;
	
	return contents;
}


static int attrib_type_size(GLenum t) {
	switch(t) {
		case GL_DOUBLE: 
			return 8; 
		case GL_FLOAT: 
			return 4; 
		case GL_SHORT: 
		case GL_UNSIGNED_SHORT: 
			return 2; 
		case GL_BYTE: 
		case GL_UNSIGNED_BYTE: 
			return 1; 
		case GL_MATRIX_EXT: // abused for this function. does not conflict with anything
			return 4*16; 
		default:
			fprintf(stderr, "Unsupported VAO type\n");
			int a = *((int*)0);
			exit(2);
	}	
}



GLuint makeVAO(VAOConfig* details) {
	int i; // packed data is expected
	uintptr_t offset = 0;
	int stride = 0;
	int attrSlot = 0;
	GLuint vao;
	
	
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	for(i = 0; details[i].sz != 0; i++) {
		stride += details[i].sz * attrib_type_size(details[i].type);
	}
	
	for(i = 0; details[i].sz != 0; i++) {
		GLenum t;
		int ds;
		
		glEnableVertexAttribArray(i);
		t = details[i].type;
		if(t == GL_FLOAT) { // works only for my usage
			glVertexAttribFormat(attrSlot, details[i].sz, t, GL_FALSE, (void*)offset);
		}
		else if(t == GL_MATRIX_EXT) {
			glVertexAttribFormat(attrSlot++, 4, GL_FLOAT, GL_FALSE, (void*)offset);
			glVertexAttribFormat(attrSlot++, 4, GL_FLOAT, GL_FALSE, (void*)offset+4*4);
			glVertexAttribFormat(attrSlot++, 4, GL_FLOAT, GL_FALSE, (void*)offset+4*8);
			glVertexAttribFormat(attrSlot  , 4, GL_FLOAT, GL_FALSE, (void*)offset+4*12);
		}
		else {
			glVertexAttribIFormat(i, details[i].sz, t, (void*)offset);
		}
		glerr("vao init");
		
		if(t == GL_UNSIGNED_BYTE || t == GL_BYTE) ds = 1;
		else if(t == GL_UNSIGNED_SHORT || t == GL_SHORT) ds = 2;
		else ds = 4;
		
		offset += ds * details[i].sz;
		attrSlot++;
	}
	glexit("vao init");
	
	return vao;
}


size_t calcVAOStride(int bufferIndex, VAOConfig* details) {
	int i;
	int startIndex = -1, endIndex = -1;
	int stride = 0;
	
	// determine the buffer's range
	for(i = 0; details[i].sz != 0 && endIndex == -1; i++) {
		if(details[i].bufferIndex == bufferIndex) {
			stride += details[i].sz * attrib_type_size(details[i].type);
		}		
	}
	
	return stride;
}


// returns stride
size_t updateVAO(int bufferIndex, VAOConfig* details) {
	
	int i;
	int startIndex = -1, endIndex = -1;
	int stride = 0;
	
	// determine the buffer's range
	for(i = 0; details[i].sz != 0 && endIndex == -1; i++) {
		if(startIndex == -1) {
			if(details[i].bufferIndex == bufferIndex) startIndex = i;
		}
		else {
			
			if(details[i].bufferIndex != bufferIndex) {
				endIndex = i - 1;
				
			}
		}
		
		if(startIndex != -1 && endIndex == -1) {
			stride += details[i].sz * attrib_type_size(details[i].type);
		}		
	}
	if(endIndex == -1) endIndex = i - 1;
	
	
	int offset = 0;
	int attrSlot = startIndex;
	for(i = startIndex; i <= endIndex; i++) {
		glEnableVertexAttribArray(attrSlot);
		
		GLenum t = details[i].type;
		
		if(t == GL_FLOAT || details[i].normalized == GL_TRUE) { // works only for my usage
			glVertexAttribPointer(attrSlot, details[i].sz, t, details[i].normalized, stride, (void*)offset);
			glVertexAttribDivisor(attrSlot, details[i].divisor);
			glexit("");
		}
		else if(t == GL_MATRIX_EXT) {
			
			glEnableVertexAttribArray(attrSlot+1);
			glEnableVertexAttribArray(attrSlot+2);
			glEnableVertexAttribArray(attrSlot+3);
			
			glVertexAttribPointer(attrSlot,   4, GL_FLOAT, details[i].normalized, stride, (void*)offset);
			glVertexAttribPointer(attrSlot+1, 4, GL_FLOAT, details[i].normalized, stride, (void*)offset+4*4);
			glVertexAttribPointer(attrSlot+2, 4, GL_FLOAT, details[i].normalized, stride, (void*)offset+4*8);
			glVertexAttribPointer(attrSlot+3, 4, GL_FLOAT, details[i].normalized, stride, (void*)offset+4*12);

			glVertexAttribDivisor(attrSlot,   details[i].divisor);
			glVertexAttribDivisor(attrSlot+1, details[i].divisor);
			glVertexAttribDivisor(attrSlot+2, details[i].divisor);
			glVertexAttribDivisor(attrSlot+3, details[i].divisor);
			glexit("");
			
			attrSlot += 3;
		}
		else {
			glVertexAttribIPointer(attrSlot, details[i].sz, t, stride, (void*)offset);
			glVertexAttribDivisor(attrSlot, details[i].divisor);
			glexit("");
		}
		
		
		int ds = 0;
		if(t == GL_UNSIGNED_BYTE || t == GL_BYTE) ds = 1;
		else if(t == GL_UNSIGNED_SHORT || t == GL_SHORT) ds = 2;
		else if(t == GL_MATRIX_EXT) ds = 4*16;
		else ds = 4;
		
		offset += ds * details[i].sz;
		attrSlot++;
	} 
	

	glexit("vao update");
	return stride;
}



// length of the line, or length of the string if no \n found
size_t strlnlen(const char* s) {
	char* n;
	
	n = strchr(s, '\n');
	if(!n) return strlen(s);
	
	return n - s;
}

// strdup a line
char* strlndup(const char* s) {
	return strndup(s, strlnlen(s));
}

// line count;
int strlinecnt(const char* s) {
	int n;

	if(!*s) return 0;
	
	n = 1;
	while(*s) // just to make you cringe
		if(*s++ == '\n') 
			n++;
	
	return n;
}

char* pathJoin(const char* a, const char* b) {
	int alen, blen;
	char* o;
	
	
	alen = a ? strlen(a) : 0;
	blen = b ? strlen(b) : 0;
	
	o = malloc(alen + blen + 2);
	
	strcpy(o, a ? a : "");
	o[alen] = '/'; // TODO: fix the concat here
	strcpy(o + alen + 1, b ? b : "");
	o[alen + blen + 1] = 0; 
	
	return o;
}

