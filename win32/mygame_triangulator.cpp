#include <assert.h>
#include "mygame_triangulator.h"

#define CONCAVE -1
#define TANGENTIAL 0
#define CONVEX 1

inline int getPreviousIndex(int index, int vertexCount) {
	int result = (index == 0 ? vertexCount : index) - 1;
	return result;
}

inline int getNextIndex(int index, int vertexCount) {
	int result = (index + 1) % vertexCount;
	return result;
}

inline int computeSpannedAreaSign(float p1x, float p1y, float p2x, float p2y, float p3x, float p3y) {
	float area = p1x * (p3y - p2y);
	area += p2x * (p1y - p3y);
	area += p3x * (p2y - p1y);
	int result = signum(area);
	return result;
}

inline int classifyVertex(int index, int *indices, float *vertices, int vertexCount) {
	int prevIndex = getPreviousIndex(index, vertexCount);
	int previous = indices[prevIndex] * 2;
	int current = indices[index] * 2;
	int nextIndex = getNextIndex(index, vertexCount);
	int next = indices[nextIndex] * 2;
	int result = computeSpannedAreaSign(vertices[previous], vertices[previous + 1], vertices[current], vertices[current + 1],
		vertices[next], vertices[next + 1]);
	return result;
}

inline bool isEarTip(int earTipIndex, int *vertexTypes, int vertexCount, int *indices, float *vertices) {
	if (vertexTypes[earTipIndex] == CONCAVE) {
		return false;
	}

	int previousIndex = getPreviousIndex(earTipIndex, vertexCount);
	int nextIndex = getNextIndex(earTipIndex, vertexCount);
	int p1 = indices[previousIndex] * 2;
	int p2 = indices[earTipIndex] * 2;
	int p3 = indices[nextIndex] * 2;
	float p1x = vertices[p1], p1y = vertices[p1 + 1];
	float p2x = vertices[p2], p2y = vertices[p2 + 1];
	float p3x = vertices[p3], p3y = vertices[p3 + 1];

	// Check if any point is inside the triangle formed by previous, current and next vertices.
	// Only consider vertices that are not part of this triangle, or else we'll always find one inside.
	for (int i = getNextIndex(nextIndex, vertexCount); i != previousIndex; i = getNextIndex(i, vertexCount)) {
		// Concave vertices can obviously be inside the candidate ear, but so can tangential vertices
		// if they coincide with one of the triangle's vertices.
		if (vertexTypes[i] != CONVEX) {
			int v = indices[i] * 2;
			float vx = vertices[v];
			float vy = vertices[v + 1];
			// Because the polygon has clockwise winding order, the area sign will be positive if the point is strictly inside.
			// It will be 0 on the edge, which we want to include as well.
			// note: check the edge defined by p1->p3 first since this fails _far_ more then the other 2 checks.
			if (computeSpannedAreaSign(p3x, p3y, p1x, p1y, vx, vy) >= 0) {
				if (computeSpannedAreaSign(p1x, p1y, p2x, p2y, vx, vy) >= 0) {
					if (computeSpannedAreaSign(p2x, p2y, p3x, p3y, vx, vy) >= 0) {
						return false;
					}
				}
			}
		}
	}
	return true;
}

inline int findEarTip(int vertexCount, int *vertexTypes, int *indices, float *vertices) {
	for (int i = 0; i < vertexCount; i++) {
		if (isEarTip(i, vertexTypes, vertexCount, indices, vertices)) {
			return i;
		}
	}

	// Desperate mode: if no vertex is an ear tip, we are dealing with a degenerate polygon (e.g. nearly collinear).
	// Note that the input was not necessarily degenerate, but we could have made it so by clipping some valid ears.

	// Idea taken from Martin Held, "FIST: Fast industrial-strength triangulation of polygons", Algorithmica (1998),
	// http://citeseerx.ist.psu.edu/viewdoc/summary?doi=10.1.1.115.291

	// Return a convex or tangential vertex if one exists.
	for (int i = 0; i < vertexCount; i++) {
		if (vertexTypes[i] != CONCAVE) {
			return i;
		}
	}
	return 0; // If all vertices are concave, just return the first one.
}

struct IntArray {
	int e[100];
	int size;
};

inline void arrayRemoveIndex(IntArray *arr, int index) {
	assert(index >= 0 && index < arr->size);
	for (int i = index; i < (arr->size - 1); ++i) {
		arr->e[i] = arr->e[i + 1];
	}
	arr->size--;
}

inline void arrayAdd(IntArray *arr, int x) {
	arr->e[arr->size] = x;
	arr->size++;
}

inline void triangulate(int vertexCount, float *vertices, IntArray *triangles, IntArray *indices, IntArray *vertexTypes) {
	while (vertexCount > 3) {
		int earTipIndex = findEarTip(vertexCount, vertexTypes->e, indices->e, vertices);

		// Cut ear tip
		arrayAdd(triangles, indices->e[getPreviousIndex(earTipIndex, vertexCount)]);
		arrayAdd(triangles, indices->e[earTipIndex]);
		arrayAdd(triangles, indices->e[getNextIndex(earTipIndex, vertexCount)]);
		arrayRemoveIndex(indices, earTipIndex);
		arrayRemoveIndex(vertexTypes, earTipIndex);
		vertexCount--;

		// The type of the two vertices adjacent to the clipped vertex may have changed.
		int previousIndex = getPreviousIndex(earTipIndex, vertexCount);
		int nextIndex = earTipIndex == vertexCount ? 0 : earTipIndex;
		vertexTypes->e[previousIndex] = classifyVertex(previousIndex, indices->e, vertices, vertexCount);
		vertexTypes->e[nextIndex] = classifyVertex(nextIndex, indices->e, vertices, vertexCount);
	}

	if (vertexCount == 3) {
		arrayAdd(triangles, indices->e[0]);
		arrayAdd(triangles, indices->e[1]);
		arrayAdd(triangles, indices->e[2]);
	}
}

TriangulatePolygonResult triangulatePolygon(MyPolygon *polygon) {
	// Polygon vertices should be in counterclockwise order.

	TriangulatePolygonResult result = {};

	float *vertices = (float *)polygon->untransformedVertices;
	int vertexCount = polygon->vertexCount;
	IntArray indicesArray = {};
	indicesArray.size = vertexCount;
	int *indices = indicesArray.e;
	for (int i = 0; i < vertexCount; i++) {
		// Convert to clockwise order.
		indices[i] = vertexCount - 1 - i;
	}

	IntArray vertexTypesArray = {};
	int *vertexTypes = vertexTypesArray.e;
	vertexTypesArray.size = vertexCount;
	for (int i = 0; i < vertexCount; ++i) {
		vertexTypes[i] = classifyVertex(i, indices, vertices, vertexCount);
	}

	IntArray triangles = {};
	triangulate(vertexCount, vertices, &triangles, &indicesArray, &vertexTypesArray);

	result.trianglesCount = triangles.size / 3;
	for (int i = 0, j = 0; i < triangles.size - 2; i += 3, ++j) {
		result.triangles[j].vertexCount = 3;
		result.triangles[j].untransformedVertices[2] = polygon->untransformedVertices[triangles.e[i]];
		result.triangles[j].untransformedVertices[1] = polygon->untransformedVertices[triangles.e[i + 1]];
		result.triangles[j].untransformedVertices[0] = polygon->untransformedVertices[triangles.e[i + 2]];
	}

	return result;
}
