#ifndef MYGAME_TRIANGULATOR_H
#define MYGAME_TRIANGULATOR_H

#include "mygame_math.h"
#include "mygame_polygon.h"

void triangulatePolygon(vec2 *vertices, int vertexCount, vec2 *outTriangles, int *outVertexCount);

#endif
