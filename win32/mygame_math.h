#ifndef MYGAME_MATH_H
#define MYGAME_MATH_H

struct vec2 {
	float x, y;
};

struct MyPolygon {
	vec2 untransformedVertices[64];
	vec2 vertices[64];
	int vertexCount;
};

#endif
