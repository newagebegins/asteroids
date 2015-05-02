#ifndef MYGAME_TRIANGULATOR_H
#define MYGAME_TRIANGULATOR_H

#include "mygame_math.h"
#include "mygame_polygon.h"

struct TriangulatePolygonResult {
	MyPolygon triangles[100];
	int trianglesCount;
};
TriangulatePolygonResult triangulatePolygon(MyPolygon *polygon);

#endif
