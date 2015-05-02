#ifndef MYGAME_POLYGON_H
#define MYGAME_POLYGON_H

#include "mygame_math.h"

struct MyPolygon {
	vec2 vertices[64];
	int vertexCount;
};

MyRectangle getPolygonBounds(vec2 *polygon, int vertexCount);
bool polygonsIntersect(vec2 *verticesA, int vertexCountA, vec2 *verticesB, int vertexCountB);
bool isPointInPolygon(vec2 p, vec2 *vertices, int vertexCount);

#endif
