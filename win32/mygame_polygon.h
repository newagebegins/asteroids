#ifndef MYGAME_POLYGON_H
#define MYGAME_POLYGON_H

#include "mygame.h"
#include "mygame_math.h"

MyRectangle getPolygonBounds(vec2 *polygon, int vertexCount);
bool polygonsIntersect(vec2 *verticesA, int vertexCountA, vec2 *verticesB, int vertexCountB);
bool isPointInPolygon(vec2 p, vec2 *vertices, int vertexCount);
bool lineIntersectsPolygon(vec2 p1, vec2 p2, vec2 *polygon, int vertexCount);
bool lineSegmentsIntersect(vec2 p1, vec2 p2, vec2 p3, vec2 p4);
bool polygonTrianglesIntersect(vec2 *trianglesA, int triangleCountA, vec2 *trianglesB, int triangleCountB);

#endif
