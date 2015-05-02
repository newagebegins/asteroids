#include "mygame_polygon.h"

static inline vec2 getEdge(int edgeIndex, vec2 *vertices, int vertexCount) {
	vec2 result;
	vec2 p1 = vertices[edgeIndex];
	vec2 p2;
	if (edgeIndex + 1 >= vertexCount) {
		p2 = vertices[0];
	}
	else {
		p2 = vertices[edgeIndex + 1];
	}
	result = p2 - p1;
	return result;
}

static inline float intervalDistance(float minA, float maxA, float minB, float maxB) {
	float result;
	if (minA < minB) {
		result = minB - maxA;
	}
	else {
		result = minA - maxB;
	}
	return result;
}

struct ProjectPolygonResult {
	float min, max;
};
static ProjectPolygonResult projectPolygon(vec2 axis, vec2 *vertices, int vertexCount) {
	ProjectPolygonResult result;
	float dotProduct = dot(axis, vertices[0]);
	result.min = dotProduct;
	result.max = dotProduct;
	for (int i = 1; i < vertexCount; ++i) {
		dotProduct = dot(axis, vertices[i]);
		if (dotProduct < result.min) {
			result.min = dotProduct;
		}
		else if (dotProduct > result.max) {
			result.max = dotProduct;
		}
	}
	return result;
}

bool polygonsIntersect(vec2 *verticesA, int vertexCountA, vec2 *verticesB, int vertexCountB) {
	for (int edgeIndex = 0; edgeIndex < vertexCountA + vertexCountB; ++edgeIndex) {
		vec2 edge;
		if (edgeIndex < vertexCountA) {
			edge = getEdge(edgeIndex, verticesA, vertexCountA);
		}
		else {
			edge = getEdge(edgeIndex - vertexCountA, verticesB, vertexCountB);
		}
		vec2 axis = normalize(Vec2(-edge.y, edge.x));
		ProjectPolygonResult projectionA = projectPolygon(axis, verticesA, vertexCountA);
		ProjectPolygonResult projectionB = projectPolygon(axis, verticesB, vertexCountB);
		float distance = intervalDistance(projectionA.min, projectionA.max,
			projectionB.min, projectionB.max);
		if (distance > 0) {
			return false;
		}
	}
	return true;
}

MyRectangle getPolygonBounds(vec2 *vertices, int vertexCount) {
	MyRectangle result;
	result.min.x = vertices[0].x;
	result.max.x = vertices[0].x;
	result.min.y = vertices[0].y;
	result.max.y = vertices[0].y;
	for (int i = 1; i < vertexCount; i++) {
		vec2 q = vertices[i];
		result.min.x = fminf(q.x, result.min.x);
		result.max.x = fmaxf(q.x, result.max.x);
		result.min.y = fminf(q.y, result.min.y);
		result.max.y = fmaxf(q.y, result.max.y);
	}
	return result;
}

bool isPointInPolygon(vec2 p, vec2 *vertices, int vertexCount) {
	MyRectangle bounds = getPolygonBounds(vertices, vertexCount);
	if (p.x < bounds.min.x || p.x > bounds.max.x || p.y < bounds.min.y || p.y > bounds.max.y) {
		return false;
	}
	// http://www.ecse.rpi.edu/Homepages/wrf/Research/Short_Notes/pnpoly.html
	bool inside = false;
	for (int i = 0, j = vertexCount - 1; i < vertexCount; j = i++) {
		if ((vertices[i].y > p.y) != (vertices[j].y > p.y) &&
			p.x < (vertices[j].x - vertices[i].x) * (p.y - vertices[i].y) / (vertices[j].y - vertices[i].y) + vertices[i].x) {
			inside = !inside;
		}
	}
	return inside;
}
