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
		float distance = intervalDistance(projectionA.min, projectionA.max, projectionB.min, projectionB.max);
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

bool lineSegmentsIntersect(vec2 p1, vec2 p2, vec2 p3, vec2 p4) {
	// Must use double precision for correct results!

	double x1 = p1.x, x2 = p2.x, x3 = p3.x, x4 = p4.x;
	double y1 = p1.y, y2 = p2.y, y3 = p3.y, y4 = p4.y;

	double d = (x1 - x2) * (y3 - y4) - (y1 - y2) * (x3 - x4);
	if (d == 0) {
		return false;
	}

	double pre = (x1*y2 - y1*x2), post = (x3*y4 - y3*x4);
	double x = (pre * (x3 - x4) - (x1 - x2) * post) / d;
	double y = (pre * (y3 - y4) - (y1 - y2) * post) / d;

	// Check if the x and y coordinates are within both lines
	if (x < fmin(x1, x2) || x > fmax(x1, x2) ||
		x < fmin(x3, x4) || x > fmax(x3, x4))
	{
		return false;
	}
	if (y < fmin(y1, y2) || y > fmax(y1, y2) ||
		y < fmin(y3, y4) || y > fmax(y3, y4))
	{
		return false;
	}

	return true;
}

bool lineIntersectsPolygon(vec2 p1, vec2 p2, vec2 *polygon, int vertexCount) {
	for (int i = 0; i < vertexCount; i++) {
		vec2 p3 = polygon[i];
		vec2 p4 = polygon[(i + 1) % vertexCount];
		if (lineSegmentsIntersect(p1, p2, p3, p4)) {
			return true;
		}
	}
	return false;
}

bool overlap(MyRectangle a, MyRectangle b) {
	bool noOverlap =
		a.min.x > b.max.x ||
		b.min.x > a.max.x ||
		a.min.y > b.max.y ||
		b.min.y > a.max.y;
	return !noOverlap;
}

bool polygonTrianglesIntersect(vec2 *trianglesA, int triangleCountA, vec2 *trianglesB, int triangleCountB) {
	MyRectangle rectA = getPolygonBounds(trianglesA, triangleCountA * 3);
	MyRectangle rectB = getPolygonBounds(trianglesB, triangleCountB * 3);
	if (!overlap(rectA, rectB)) {
		return false;
	}
	for (int iA = 0; iA < triangleCountA; ++iA) {
		vec2 triangleA[3];
		triangleA[0] = trianglesA[iA * 3 + 0];
		triangleA[1] = trianglesA[iA * 3 + 1];
		triangleA[2] = trianglesA[iA * 3 + 2];	
		for (int iB = 0; iB < triangleCountB; ++iB) {
			vec2 triangleB[3];
			triangleB[0] = trianglesB[iB * 3 + 0];
			triangleB[1] = trianglesB[iB * 3 + 1];
			triangleB[2] = trianglesB[iB * 3 + 2];
			if (polygonsIntersect(triangleB, 3, triangleA, 3)) {
				return true;
			}
		}
	}
	return false;
}
