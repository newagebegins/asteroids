#ifndef MYGAME_POLYGON_H
#define MYGAME_POLYGON_H

#include "mygame_math.h"

struct MyPolygon {
	vec2 untransformedVertices[64];
	vec2 vertices[64];
	int vertexCount;
};

MyRectangle getPolygonBounds(MyPolygon *polygon);
bool polygonsIntersect(MyPolygon *polygonA, MyPolygon *polygonB);
bool isPointInPolygon(vec2 p, MyPolygon *polygon);

#endif
