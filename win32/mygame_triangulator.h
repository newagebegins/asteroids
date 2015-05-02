#ifndef MYGAME_TRIANGULATOR_H
#define MYGAME_TRIANGULATOR_H

#include "mygame_math.h"
#include "mygame_polygon.h"

struct TriangulatePolygonResult {
	vec2 triangles[300];
	int vertexCount;
};
TriangulatePolygonResult triangulatePolygon(vec2 *vertices, int vertexCount);

#endif
