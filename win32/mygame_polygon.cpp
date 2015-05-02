#include "mygame_polygon.h"

static vec2 getEdge(int edgeIndex, MyPolygon *polygon) {
	vec2 result;
	vec2 p1 = polygon->vertices[edgeIndex];
	vec2 p2;
	if (edgeIndex + 1 >= polygon->vertexCount) {
		p2 = polygon->vertices[0];
	}
	else {
		p2 = polygon->vertices[edgeIndex + 1];
	}
	result = p2 - p1;
	return result;
}

static float intervalDistance(float minA, float maxA, float minB, float maxB) {
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
static ProjectPolygonResult projectPolygon(vec2 axis, MyPolygon *polygon) {
	ProjectPolygonResult result;
	float dotProduct = dot(axis, polygon->vertices[0]);
	result.min = dotProduct;
	result.max = dotProduct;
	for (int i = 1; i < polygon->vertexCount; ++i) {
		dotProduct = dot(axis, polygon->vertices[i]);
		if (dotProduct < result.min) {
			result.min = dotProduct;
		}
		else if (dotProduct > result.max) {
			result.max = dotProduct;
		}
	}
	return result;
}

bool polygonsIntersect(MyPolygon *polygonA, MyPolygon *polygonB) {
	for (int edgeIndex = 0; edgeIndex < polygonA->vertexCount + polygonB->vertexCount; ++edgeIndex) {
		vec2 edge;
		if (edgeIndex < polygonA->vertexCount) {
			edge = getEdge(edgeIndex, polygonA);
		}
		else {
			edge = getEdge(edgeIndex - polygonA->vertexCount, polygonB);
		}
		vec2 axis = normalize(Vec2(-edge.y, edge.x));
		ProjectPolygonResult projectionA = projectPolygon(axis, polygonA);
		ProjectPolygonResult projectionB = projectPolygon(axis, polygonB);
		float distance = intervalDistance(projectionA.min, projectionA.max,
			projectionB.min, projectionB.max);
		if (distance > 0) {
			return false;
		}
	}
	return true;
}

MyRectangle getPolygonBounds(MyPolygon *polygon) {
	MyRectangle result;
	result.min.x = polygon->vertices[0].x;
	result.max.x = polygon->vertices[0].x;
	result.min.y = polygon->vertices[0].y;
	result.max.y = polygon->vertices[0].y;
	for (int i = 1; i < polygon->vertexCount; i++) {
		vec2 q = polygon->vertices[i];
		result.min.x = fminf(q.x, result.min.x);
		result.max.x = fmaxf(q.x, result.max.x);
		result.min.y = fminf(q.y, result.min.y);
		result.max.y = fmaxf(q.y, result.max.y);
	}
	return result;
}

bool isPointInPolygon(vec2 p, MyPolygon *polygon) {
	MyRectangle bounds = getPolygonBounds(polygon);
	if (p.x < bounds.min.x || p.x > bounds.max.x || p.y < bounds.min.y || p.y > bounds.max.y) {
		return false;
	}
	// http://www.ecse.rpi.edu/Homepages/wrf/Research/Short_Notes/pnpoly.html
	bool inside = false;
	for (int i = 0, j = polygon->vertexCount - 1; i < polygon->vertexCount; j = i++) {
		if ((polygon->vertices[i].y > p.y) != (polygon->vertices[j].y > p.y) &&
			p.x < (polygon->vertices[j].x - polygon->vertices[i].x) * (p.y - polygon->vertices[i].y) / (polygon->vertices[j].y - polygon->vertices[i].y) + polygon->vertices[i].x) {
			inside = !inside;
		}
	}
	return inside;
}
