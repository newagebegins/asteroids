#include <math.h>
#include <stdio.h>
#include <time.h>
#include <assert.h>
#ifdef _WIN32
#include "gl3w.h"
#else
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#endif
#include "mygame.h"

#define PI 3.14159265f
#define DEG_TO_RAD 0.0174532925f
#define RAD_TO_DEG 57.2957795f;
#define SCREEN_WIDTH 960.0f
#define SCREEN_HEIGHT 540.0f
#define MAX_PLAYER_SPEED 500.0f
#define PLAYER_ACCELERATION 1000.0f
#define PLAYER_DECELERATION 200.0f
#define PLAYER_REVIVE_DURATION 1.0f
#define BULLET_SPEED 600.0f
#define MAX_BULLET_DISTANCE 400.0f
#define ASTEROID_SCALE_SMALL 20
#define ASTEROID_SCALE_MEDIUM 60
#define ASTEROID_SCALE_BIG 90
#define LEVEL_END_DURATION 2.0f

#define arrayCount(array) (sizeof(array) / sizeof((array)[0]))

void(*platformLog)(const char *);

inline vec2 Vec2(float x, float y) {
    vec2 result;
    result.x = x;
    result.y = y;
    return result;
}
inline vec2 operator*(float a, vec2 b) {
    vec2 result;
    result.x = a * b.x;
    result.y = a * b.y;
    return result;
}
inline vec2 operator*(vec2 b, float a) {
    vec2 result = a * b;
    return result;
}
inline vec2& operator*=(vec2 &b, float a) {
    b = a * b;
    return b;
}
inline vec2 operator+(vec2 a, vec2 b) {
    vec2 result;
    result.x = a.x + b.x;
    result.y = a.y + b.y;
    return result;
}
inline vec2& operator+=(vec2 &a, vec2 b) {
    a = a + b;
    return a;
}
inline vec2 operator-(vec2 a, vec2 b) {
    vec2 result;
    result.x = a.x - b.x;
    result.y = a.y - b.y;
    return result;
}
inline vec2& operator-=(vec2 &a, vec2 b) {
    a = a - b;
    return a;
}
inline float dot(vec2 a, vec2 b) {
    float result = a.x*b.x + a.y*b.y;
    return result;
}
inline float lenSq(vec2 a) {
    float result = dot(a, a);
    return result;
}
inline float len(vec2 a) {
    float result = sqrt(lenSq(a));
    return result;
}
inline vec2 normalize(vec2 a) {
    vec2 result = a;
    float length = len(a);
    if (length > 0.0f) {
        result = a*(1.0f/len(a));
    }
    return result;
}

struct MyRectangle {
    vec2 min;
    vec2 max;
};

struct mat4 {
    float e[16];
    inline float& operator[](int i) {
        return e[i];
    }
};
inline mat4 operator*(mat4 a, mat4 b) {
    mat4 r = {};
    r[0]  = a[0]*b[0]  + a[4]*b[1]  + a[8] *b[2]  + a[12]*b[3];
    r[1]  = a[1]*b[0]  + a[5]*b[1]  + a[9] *b[2]  + a[13]*b[3];
    r[2]  = a[2]*b[0]  + a[6]*b[1]  + a[10]*b[2]  + a[14]*b[3];
    r[3]  = a[3]*b[0]  + a[7]*b[1]  + a[11]*b[2]  + a[15]*b[3];
    r[4]  = a[0]*b[4]  + a[4]*b[5]  + a[8] *b[6]  + a[12]*b[7];
    r[5]  = a[1]*b[4]  + a[5]*b[5]  + a[9] *b[6]  + a[13]*b[7];
    r[6]  = a[2]*b[4]  + a[6]*b[5]  + a[10]*b[6]  + a[14]*b[7];
    r[7]  = a[3]*b[4]  + a[7]*b[5]  + a[11]*b[6]  + a[15]*b[7];
    r[8]  = a[0]*b[8]  + a[4]*b[9]  + a[8] *b[10] + a[12]*b[11];
    r[9]  = a[1]*b[8]  + a[5]*b[9]  + a[9] *b[10] + a[13]*b[11];
    r[10] = a[2]*b[8]  + a[6]*b[9]  + a[10]*b[10] + a[14]*b[11];
    r[11] = a[3]*b[8]  + a[7]*b[9]  + a[11]*b[10] + a[15]*b[11];
    r[12] = a[0]*b[12] + a[4]*b[13] + a[8] *b[14] + a[12]*b[15];
    r[13] = a[1]*b[12] + a[5]*b[13] + a[9] *b[14] + a[13]*b[15];
    r[14] = a[2]*b[12] + a[6]*b[13] + a[10]*b[14] + a[14]*b[15];
    r[15] = a[3]*b[12] + a[7]*b[13] + a[11]*b[14] + a[15]*b[15];
    return r;
}
inline vec2 operator*(mat4 a, vec2 b) {
    vec2 r = {};
    r.x = a[0]*b.x + a[4]*b.y + a[12];
    r.y = a[1]*b.x + a[5]*b.y + a[13];
    return r;
}
inline mat4 createIdentityMatrix() {
    mat4 m = {};
    m[0] = 1.0f;
    m[5] = 1.0f;
    m[10] = 1.0f;
    m[15] = 1.0f;
    return m;
}
inline mat4 createTranslationMatrix(float x, float y) {
    mat4 m = {};
    m[0] = 1.0f;
    m[5] = 1.0f;
    m[10] = 1.0f;
    m[12] = x;
    m[13] = y;
    m[15] = 1.0f;
    return m;
}
inline mat4 createScaleMatrix(float scaleX, float scaleY) {
    mat4 m = {};
    m[0] = scaleX;
    m[5] = scaleY;
    m[10] = 1.0f;
    m[15] = 1.0f;
    return m;
}
inline mat4 createScaleMatrix(float scale) { return createScaleMatrix(scale, scale); }
inline mat4 createRotationMatrix(float angle) {
    mat4 m = {};
    m[0] = cosf(angle);
    m[1] = -sinf(angle);
    m[4] = sinf(angle);
    m[5] = cosf(angle);
    m[10] = 1.0f;
    m[15] = 1.0f;
    return m;
}
inline mat4 createOrthoMatrix(float left, float right,
                              float bottom, float top,
                              float zNear, float zFar) {
    mat4 m = {};
    m[0] = 2.0f / (right - left);
    m[5] = 2.0f / (top - bottom);
    m[10] = -2.0f / (zFar - zNear);
    m[12] = -(right + left) / (right - left);
    m[13] = -(top + bottom) / (top - bottom);
    m[14] = -(zFar + zNear) / (zFar - zNear);
    m[15] = 1.0f;
    return m;
}

#define RANDOM_UINT_MAX 65535

static unsigned long sRandomSeed = 0;

static void seedRandom(unsigned long seed) {
    sRandomSeed = seed;
}

static unsigned long randomUInt() {
    sRandomSeed = sRandomSeed * 0x343fd + 0x269ec3;
    return sRandomSeed >> 16;
}

// max is included
inline int randomInt(int min, int max) {
    int result = min + (randomUInt() % (int)(max - min + 1));
    return result;
}
inline int randomInt(int max) {
    int result = randomInt(0, max);
    return result;
}

inline float randomFloat(float min, float max) {
    float result = (float)randomInt((int)min, (int)max);
    return result;
}
inline float randomFloat(float max) {
    float result = randomFloat(0, max);
    return result;
}

inline float randomAngle() {
    float result = randomFloat(0, 360)*DEG_TO_RAD;
    return result;
}

static vec2 randomDirection() {
    float angle = randomAngle();
    vec2 result = Vec2(cosf(angle), sinf(angle));
    return result;
}

static GLuint compileShader(const char *source, GLenum shaderType) {
    GLuint shader = glCreateShader(shaderType);
    if (!shader) {
        return 0;
    }
    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);
    GLint compiled = 0;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
    if (!compiled) {
        GLchar log[4096];
        glGetShaderInfoLog(shader, sizeof(log), NULL, log);
        platformLog("Shader error!\n");
        platformLog(log);
        shader = 0;
    }
    return shader;
}

#define PLAYER_SHIP_VERTICES_COUNT 6
static vec2 playerShipVertices[PLAYER_SHIP_VERTICES_COUNT] = {
    Vec2(0.0f, 0.9f), Vec2(-0.7f, -0.9f), // ship left side
    Vec2(0.0f, 0.9f), Vec2(0.7f, -0.9f), // ship right side
    Vec2(-0.54f, -0.49f), Vec2(0.54f, -0.49f), // ship bottom side
};
static vec2 transformedPlayerShipVertices[PLAYER_SHIP_VERTICES_COUNT] = {};

#define PLAYER_FLAME_VERTICES_COUNT 4
static vec2 playerFlameVertices[PLAYER_FLAME_VERTICES_COUNT] = {
    Vec2(-0.3f, -0.49f), Vec2(0.0f, -1.0f), // flame left side
    Vec2(0.3f, -0.49f), Vec2(0.0f, -1.0f), // flame right side
};
static vec2 transformedPlayerFlameVertices[PLAYER_FLAME_VERTICES_COUNT] = {};

#define SHIP_FRAGMENT_VERTICES_COUNT 2
static vec2 shipFragmentVertices[SHIP_FRAGMENT_VERTICES_COUNT] = {
    Vec2(-0.5f, 0.0f),
    Vec2(0.5f, 0.0f),
};
static vec2 transformedShipFragmentVertices[SHIP_FRAGMENT_VERTICES_COUNT] = {0};

struct MyPolygon {
    vec2 untransformedVertices[64];
    vec2 vertices[64];
    int vertexCount;
};

struct Player {
    bool alive;
    vec2 pos;
    vec2 vel;
    vec2 accel;
    float angle;
    float reviveTimer;
    float flameFlickTimer;
    bool flameVisible;
    MyPolygon collisionPolygon;
};
inline void initPlayer(Player *player) {
    player->alive = true;
    player->pos = Vec2(SCREEN_WIDTH/2, SCREEN_HEIGHT/2);
    player->vel = Vec2(0.0f, 0.0f);
    player->accel = Vec2(0.0f, 0.0f);
    player->angle = PI/2;
    player->flameFlickTimer = 0.0f;
    player->flameVisible = false;

    player->collisionPolygon.vertexCount = 3;   
    player->collisionPolygon.untransformedVertices[0] = Vec2(0.0f, 0.9f);
    player->collisionPolygon.untransformedVertices[1] = Vec2(-0.54f, -0.49f);
    player->collisionPolygon.untransformedVertices[2] = Vec2(0.54f, -0.49f);
}

struct Asteroid {
    bool active;
    vec2 position;
    vec2 velocity;
    float scale;
    MyPolygon polygon;
    MyPolygon collisionPolygons[60];
    int collisionPolygonsCount;
    MyRectangle bounds;
};

struct ShipFragment {
    vec2 position;
    vec2 velocity;
    float scale;
    float angle;
};

struct Bullet {
    bool active;
    vec2 position;
    vec2 velocity;
    float distance;
};

struct ExplosionParticle {
    bool active;
    vec2 position;
    vec2 velocity;
    float maxDistance;
    float distance;
};

inline vec2 getEdge(int edgeIndex, MyPolygon *polygon) {
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

inline float intervalDistance(float minA, float maxA, float minB, float maxB) {
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

static bool polygonsIntersect(MyPolygon *polygonA, MyPolygon *polygonB) {
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

static MyRectangle getPolygonBounds(MyPolygon *polygon) {
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

static bool isPointInPolygon(vec2 p, MyPolygon *polygon) {
    MyRectangle bounds = getPolygonBounds(polygon);
    if (p.x < bounds.min.x || p.x > bounds.max.x || p.y < bounds.min.y || p.y > bounds.max.y) {
        return false;
    }
    // http://www.ecse.rpi.edu/Homepages/wrf/Research/Short_Notes/pnpoly.html
    bool inside = false;
    for (int i = 0, j = polygon->vertexCount - 1; i < polygon->vertexCount; j = i++) {
        if ((polygon->vertices[i].y > p.y) != (polygon->vertices[j].y > p.y) &&
            p.x < (polygon->vertices[j].x - polygon->vertices[i].x) * (p.y - polygon->vertices[i].y ) / (polygon->vertices[j].y - polygon->vertices[i].y) + polygon->vertices[i].x) {
            inside = !inside;
        }
    }
    return inside;
}

inline int getPreviousIndex(int index, int vertexCount) {
    int result = (index == 0 ? vertexCount : index) - 1;
    return result;
}

inline int getNextIndex(int index, int vertexCount) {
    int result = (index + 1) % vertexCount;
    return result;
}

int signum(float x) {
    if (x > 0) {
        return 1;
    }
    if (x < 0) {
        return -1;
    }
    return 0;
}

static int computeSpannedAreaSign(float p1x, float p1y, float p2x, float p2y, float p3x, float p3y) {
    float area = p1x * (p3y - p2y);
    area += p2x * (p1y - p3y);
    area += p3x * (p2y - p1y);
    int result = signum(area);
    return result;
}

#define CONCAVE -1
#define TANGENTIAL 0
#define CONVEX 1

static int classifyVertex(int index, int *indices, float *vertices, int vertexCount) {
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
    int e[500];
    int size;
};

static void arrayRemoveIndex(IntArray *arr, int index) {
    assert(index >= 0 && index < arr->size);
    for (int i = index; i < (arr->size - 1); ++i) {
        arr->e[i] = arr->e[i+1];
    }
    arr->size--;
}

static void arrayAdd(IntArray *arr, int x) {
    arr->e[arr->size] = x;
    arr->size++;
}

static void cutEarTip(int earTipIndex, int *indices, IntArray *triangles, int *vertexCount, IntArray *indicesArray, IntArray *vertexTypesArray) {
    arrayAdd(triangles, indices[getPreviousIndex(earTipIndex, *vertexCount)]);
    arrayAdd(triangles, indices[earTipIndex]);
    arrayAdd(triangles, indices[getNextIndex(earTipIndex, *vertexCount)]);

    arrayRemoveIndex(indicesArray, earTipIndex);
    arrayRemoveIndex(vertexTypesArray, earTipIndex);
    (*vertexCount)--;
}

inline void triangulate(int vertexCount, int *vertexTypes, int *indices, float *vertices, IntArray *triangles, IntArray *indicesArray, IntArray *vertexTypesArray) {
    while (vertexCount > 3) {
        int earTipIndex = findEarTip(vertexCount, vertexTypes, indices, vertices);
        cutEarTip(earTipIndex, indices, triangles, &vertexCount, indicesArray, vertexTypesArray);

        // The type of the two vertices adjacent to the clipped vertex may have changed.
        int previousIndex = getPreviousIndex(earTipIndex, vertexCount);
        int nextIndex = earTipIndex == vertexCount ? 0 : earTipIndex;
        vertexTypes[previousIndex] = classifyVertex(previousIndex, indices, vertices, vertexCount);
        vertexTypes[nextIndex] = classifyVertex(nextIndex, indices, vertices, vertexCount);
    }

    if (vertexCount == 3) {
        arrayAdd(triangles, indices[0]);
        arrayAdd(triangles, indices[1]);
        arrayAdd(triangles, indices[2]);
    }
}

struct TriangulatePolygonResult {
    MyPolygon triangles[100];
    int trianglesCount;
};
static TriangulatePolygonResult triangulatePolygon(MyPolygon *polygon) {
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
    triangulate(vertexCount, vertexTypesArray.e, indices, vertices, &triangles, &indicesArray, &vertexTypesArray);

    result.trianglesCount = triangles.size / 3;
    for (int i = 0, j = 0; i < triangles.size-2; i += 3, ++j) {
        result.triangles[j].vertexCount = 3;
        result.triangles[j].untransformedVertices[2] = polygon->untransformedVertices[triangles.e[i]];
        result.triangles[j].untransformedVertices[1] = polygon->untransformedVertices[triangles.e[i+1]];
        result.triangles[j].untransformedVertices[0] = polygon->untransformedVertices[triangles.e[i+2]];
    }
    
    return result;
}

Input g_input;

static GLint g_positionAttrib;
static GLint g_colorUniform;
static Player g_player;
static Asteroid g_asteroids[20];
static ShipFragment g_shipFragments[5];
static Bullet g_bullets[10];
static float g_levelEndTimer;
static int g_currentLevel = 10;
static float g_windowWidth;
static float g_windowHeight;
static float g_viewportX;
static float g_viewportY;
static float g_viewportWidth;
static float g_viewportHeight;
static ExplosionParticle g_explosionParticles[30];

static void transformAsteroid(Asteroid *asteroid) {
    mat4 translationMatrix = createTranslationMatrix(asteroid->position.x, asteroid->position.y);
    mat4 scaleMatrix = createScaleMatrix(asteroid->scale);
    mat4 modelMatrix = translationMatrix * scaleMatrix;
    for (int i = 0; i < asteroid->polygon.vertexCount; ++i) {
        asteroid->polygon.vertices[i] = modelMatrix * asteroid->polygon.untransformedVertices[i];
    }
    for (int i = 0; i < asteroid->collisionPolygonsCount; ++i) {
        for (int j = 0; j < asteroid->collisionPolygons[i].vertexCount; ++j) {
            asteroid->collisionPolygons[i].vertices[j] = modelMatrix * asteroid->collisionPolygons[i].untransformedVertices[j];
        }
    }
    asteroid->bounds = getPolygonBounds(&asteroid->polygon);
}

static void createAsteroid(Asteroid *asteroid, vec2 position, vec2 velocity, float scale) {
    asteroid->active = true;
    asteroid->position = position;
    asteroid->velocity = velocity;
    asteroid->scale = scale;
    int type = randomInt(1, 4);

#if 1
    switch (type) {
        case 1: {
            asteroid->polygon.vertexCount = 12;
            asteroid->polygon.untransformedVertices[0] = Vec2(-0.5f, 0.8f);
            asteroid->polygon.untransformedVertices[1] = Vec2(-0.3f, 0.4f);
            asteroid->polygon.untransformedVertices[2] = Vec2(-0.9f, 0.4f);
            asteroid->polygon.untransformedVertices[3] = Vec2(-0.9f, -0.2f);
            asteroid->polygon.untransformedVertices[4] = Vec2(-0.3f, -0.8f);
            asteroid->polygon.untransformedVertices[5] = Vec2(0.2f, -0.7f);
            asteroid->polygon.untransformedVertices[6] = Vec2(0.5f, -0.8f);
            asteroid->polygon.untransformedVertices[7] = Vec2(0.9f, -0.3f);
            asteroid->polygon.untransformedVertices[8] = Vec2(0.1f, 0.0f);
            asteroid->polygon.untransformedVertices[9] = Vec2(0.9f, 0.2f);
            asteroid->polygon.untransformedVertices[10] = Vec2(0.9f, 0.4f);
            asteroid->polygon.untransformedVertices[11] = Vec2(0.2f, 0.8f);

            // asteroid->collisionPolygonsCount = 3;

            // asteroid->collisionPolygons[0].vertexCount = 6;
            // asteroid->collisionPolygons[0].untransformedVertices[0] = Vec2(-0.5f, 0.8f);
            // asteroid->collisionPolygons[0].untransformedVertices[1] = Vec2(-0.3f, 0.4f);
            // asteroid->collisionPolygons[0].untransformedVertices[2] = Vec2(0.1f, 0.0f);
            // asteroid->collisionPolygons[0].untransformedVertices[3] = Vec2(0.9f, 0.2f);
            // asteroid->collisionPolygons[0].untransformedVertices[4] = Vec2(0.9f, 0.4f);
            // asteroid->collisionPolygons[0].untransformedVertices[5] = Vec2(0.2f, 0.8f);

            // asteroid->collisionPolygons[1].vertexCount = 4;
            // asteroid->collisionPolygons[1].untransformedVertices[0] = Vec2(0.1f, 0.0f);
            // asteroid->collisionPolygons[1].untransformedVertices[1] = Vec2(0.2f, -0.7f);
            // asteroid->collisionPolygons[1].untransformedVertices[2] = Vec2(0.5f, -0.8f);
            // asteroid->collisionPolygons[1].untransformedVertices[3] = Vec2(0.9f, -0.3f);

            // asteroid->collisionPolygons[2].vertexCount = 6;
            // asteroid->collisionPolygons[2].untransformedVertices[0] = Vec2(-0.9f, 0.4f);
            // asteroid->collisionPolygons[2].untransformedVertices[1] = Vec2(-0.9f, -0.2f);
            // asteroid->collisionPolygons[2].untransformedVertices[2] = Vec2(-0.3f, -0.8f);
            // asteroid->collisionPolygons[2].untransformedVertices[3] = Vec2(0.2f, -0.7f);
            // asteroid->collisionPolygons[2].untransformedVertices[4] = Vec2(0.1f, 0.0f);
            // asteroid->collisionPolygons[2].untransformedVertices[5] = Vec2(-0.3f, 0.4f);

            break;
        }
        case 2: {
            asteroid->polygon.vertexCount = 12;
            asteroid->polygon.untransformedVertices[0] = Vec2(-0.4f, 0.7f);
            asteroid->polygon.untransformedVertices[1] = Vec2(-0.7f, 0.4f);
            asteroid->polygon.untransformedVertices[2] = Vec2(-0.6f, 0.0f);
            asteroid->polygon.untransformedVertices[3] = Vec2(-0.7f, -0.3f);
            asteroid->polygon.untransformedVertices[4] = Vec2(-0.5f, -0.6f);
            asteroid->polygon.untransformedVertices[5] = Vec2(-0.2f, -0.5f);
            asteroid->polygon.untransformedVertices[6] = Vec2(0.4f, -0.6f);
            asteroid->polygon.untransformedVertices[7] = Vec2(0.8f, -0.1f);
            asteroid->polygon.untransformedVertices[8] = Vec2(0.5f, 0.3f);
            asteroid->polygon.untransformedVertices[9] = Vec2(0.7f, 0.5f);
            asteroid->polygon.untransformedVertices[10] = Vec2(0.4f, 0.7f);
            asteroid->polygon.untransformedVertices[11] = Vec2(0.0f, 0.6f);

            // asteroid->collisionPolygonsCount = 3;

            // asteroid->collisionPolygons[0].vertexCount = 8;
            // asteroid->collisionPolygons[0].untransformedVertices[0] = Vec2(-0.5f, 0.7f);
            // asteroid->collisionPolygons[0].untransformedVertices[1] = Vec2(-0.7f, 0.4f);
            // asteroid->collisionPolygons[0].untransformedVertices[2] = Vec2(-0.6f, 0.0f);
            // asteroid->collisionPolygons[0].untransformedVertices[3] = Vec2(-0.2f, -0.5f);
            // asteroid->collisionPolygons[0].untransformedVertices[4] = Vec2(0.4f, -0.6f);
            // asteroid->collisionPolygons[0].untransformedVertices[5] = Vec2(0.8f, -0.1f);
            // asteroid->collisionPolygons[0].untransformedVertices[6] = Vec2(0.5f, 0.3f);
            // asteroid->collisionPolygons[0].untransformedVertices[7] = Vec2(0.0f, 0.6f);

            // asteroid->collisionPolygons[1].vertexCount = 4;
            // asteroid->collisionPolygons[1].untransformedVertices[0] = Vec2(0.4f, 0.7f);
            // asteroid->collisionPolygons[1].untransformedVertices[1] = Vec2(0.0f, 0.6f);
            // asteroid->collisionPolygons[1].untransformedVertices[2] = Vec2(0.5f, 0.3f);
            // asteroid->collisionPolygons[1].untransformedVertices[3] = Vec2(0.7f, 0.5f);

            // asteroid->collisionPolygons[2].vertexCount = 4;
            // asteroid->collisionPolygons[2].untransformedVertices[0] = Vec2(-0.6f, 0.0f);
            // asteroid->collisionPolygons[2].untransformedVertices[1] = Vec2(-0.7f, -0.3f);
            // asteroid->collisionPolygons[2].untransformedVertices[2] = Vec2(-0.5f, -0.6f);
            // asteroid->collisionPolygons[2].untransformedVertices[3] = Vec2(-0.2f, -0.5f);

            break;
        }
        case 3: {
            asteroid->polygon.vertexCount = 10;
            asteroid->polygon.untransformedVertices[0] = Vec2(-0.3f, 0.7f);
            asteroid->polygon.untransformedVertices[1] = Vec2(-0.7f, 0.5f);
            asteroid->polygon.untransformedVertices[2] = Vec2(-0.7f, -0.3f);
            asteroid->polygon.untransformedVertices[3] = Vec2(-0.3f, -0.6f);
            asteroid->polygon.untransformedVertices[4] = Vec2(0.3f, -0.6f);
            asteroid->polygon.untransformedVertices[5] = Vec2(0.9f, -0.3f);
            asteroid->polygon.untransformedVertices[6] = Vec2(0.6f, 0.1f);
            asteroid->polygon.untransformedVertices[7] = Vec2(0.7f, 0.4f);
            asteroid->polygon.untransformedVertices[8] = Vec2(0.4f, 0.6f);
            asteroid->polygon.untransformedVertices[9] = Vec2(0.1f, 0.4f);

            // asteroid->collisionPolygonsCount = 2;

            // asteroid->collisionPolygons[0].vertexCount = 8;
            // asteroid->collisionPolygons[0].untransformedVertices[0] = Vec2(-0.3f, 0.7f);
            // asteroid->collisionPolygons[0].untransformedVertices[1] = Vec2(-0.7f, 0.5f);
            // asteroid->collisionPolygons[0].untransformedVertices[2] = Vec2(-0.7f, -0.3f);
            // asteroid->collisionPolygons[0].untransformedVertices[3] = Vec2(-0.3f, -0.6f);
            // asteroid->collisionPolygons[0].untransformedVertices[4] = Vec2(0.3f, -0.6f);
            // asteroid->collisionPolygons[0].untransformedVertices[5] = Vec2(0.9f, -0.3f);
            // asteroid->collisionPolygons[0].untransformedVertices[6] = Vec2(0.6f, 0.1f);
            // asteroid->collisionPolygons[0].untransformedVertices[7] = Vec2(0.1f, 0.4f);

            // asteroid->collisionPolygons[1].vertexCount = 4;
            // asteroid->collisionPolygons[1].untransformedVertices[0] = Vec2(0.4f, 0.6f);
            // asteroid->collisionPolygons[1].untransformedVertices[1] = Vec2(0.1f, 0.4f);
            // asteroid->collisionPolygons[1].untransformedVertices[2] = Vec2(0.6f, 0.1f);
            // asteroid->collisionPolygons[1].untransformedVertices[3] = Vec2(0.7f, 0.4f);

            break;
        }
        case 4: {
            asteroid->polygon.vertexCount = 11;
            asteroid->polygon.untransformedVertices[0] = Vec2(-0.3f, 0.5f);
            asteroid->polygon.untransformedVertices[1] = Vec2(-0.6f, 0.2f);
            asteroid->polygon.untransformedVertices[2] = Vec2(-0.3f, 0.0f);
            asteroid->polygon.untransformedVertices[3] = Vec2(-0.7f, -0.2f);
            asteroid->polygon.untransformedVertices[4] = Vec2(-0.3f, -0.6f);
            asteroid->polygon.untransformedVertices[5] = Vec2(0.1f, -0.2f);
            asteroid->polygon.untransformedVertices[6] = Vec2(0.1f, -0.6f);
            asteroid->polygon.untransformedVertices[7] = Vec2(0.3f, -0.6f);
            asteroid->polygon.untransformedVertices[8] = Vec2(0.6f, -0.2f);
            asteroid->polygon.untransformedVertices[9] = Vec2(0.6f, 0.2f);
            asteroid->polygon.untransformedVertices[10] = Vec2(0.2f, 0.5f);

            // asteroid->collisionPolygonsCount = 3;

            // asteroid->collisionPolygons[0].vertexCount = 7;
            // asteroid->collisionPolygons[0].untransformedVertices[0] = Vec2(-0.3f, 0.5f);
            // asteroid->collisionPolygons[0].untransformedVertices[1] = Vec2(-0.6f, 0.2f);
            // asteroid->collisionPolygons[0].untransformedVertices[2] = Vec2(-0.3f, 0.0f);
            // asteroid->collisionPolygons[0].untransformedVertices[3] = Vec2(0.1f, -0.2f);
            // asteroid->collisionPolygons[0].untransformedVertices[4] = Vec2(0.6f, -0.2f);
            // asteroid->collisionPolygons[0].untransformedVertices[5] = Vec2(0.6f, 0.2f);
            // asteroid->collisionPolygons[0].untransformedVertices[6] = Vec2(0.2f, 0.5f);

            // asteroid->collisionPolygons[1].vertexCount = 4;
            // asteroid->collisionPolygons[1].untransformedVertices[0] = Vec2(-0.3f, 0.0f);
            // asteroid->collisionPolygons[1].untransformedVertices[1] = Vec2(-0.7f, -0.2f);
            // asteroid->collisionPolygons[1].untransformedVertices[2] = Vec2(-0.3f, -0.6f);
            // asteroid->collisionPolygons[1].untransformedVertices[3] = Vec2(0.1f, -0.2f);

            // asteroid->collisionPolygons[2].vertexCount = 4;
            // asteroid->collisionPolygons[2].untransformedVertices[0] = Vec2(0.1f, -0.2f);
            // asteroid->collisionPolygons[2].untransformedVertices[1] = Vec2(0.1f, -0.6f);
            // asteroid->collisionPolygons[2].untransformedVertices[2] = Vec2(0.3f, -0.6f);
            // asteroid->collisionPolygons[2].untransformedVertices[3] = Vec2(0.6f, -0.2f);

            break;
        }
        // convex
        // case 4: {
        //     asteroid->polygon.vertexCount = 6;
        //     asteroid->polygon.untransformedVertices[0] = Vec2(0.0f, 0.9f);
        //     asteroid->polygon.untransformedVertices[1] = Vec2(-0.8f, 0.4f);
        //     asteroid->polygon.untransformedVertices[2] = Vec2(-0.8f, -0.4f);
        //     asteroid->polygon.untransformedVertices[3] = Vec2(0.0f, -0.9f);
        //     asteroid->polygon.untransformedVertices[4] = Vec2(0.8f, -0.4f);
        //     asteroid->polygon.untransformedVertices[5] = Vec2(0.8f, 0.4f);

        //     asteroid->collisionPolygonsCount = 1;

        //     asteroid->collisionPolygons[0].vertexCount = 6;
        //     asteroid->collisionPolygons[0].untransformedVertices[0] = Vec2(0.0f, 0.9f);
        //     asteroid->collisionPolygons[0].untransformedVertices[1] = Vec2(-0.8f, 0.4f);
        //     asteroid->collisionPolygons[0].untransformedVertices[2] = Vec2(-0.8f, -0.4f);
        //     asteroid->collisionPolygons[0].untransformedVertices[3] = Vec2(0.0f, -0.9f);
        //     asteroid->collisionPolygons[0].untransformedVertices[4] = Vec2(0.8f, -0.4f);
        //     asteroid->collisionPolygons[0].untransformedVertices[5] = Vec2(0.8f, 0.4f);
        //     break;
        // }
    }
#elif 0
    asteroid->polygon.vertexCount = 4;
    asteroid->polygon.untransformedVertices[0] = Vec2(-1.0f, 1.0f);
    asteroid->polygon.untransformedVertices[1] = Vec2(-1.0f, -1.0f);
    asteroid->polygon.untransformedVertices[2] = Vec2(1.0f, -1.0f);
    asteroid->polygon.untransformedVertices[3] = Vec2(1.0f, 1.0f);
#elif 0
    asteroid->polygon.vertexCount = 5;
    asteroid->polygon.untransformedVertices[0] = Vec2(-1.0f, 1.0f);
    asteroid->polygon.untransformedVertices[1] = Vec2(-1.0f, -1.0f);
    asteroid->polygon.untransformedVertices[2] = Vec2(1.0f, -1.0f);
    asteroid->polygon.untransformedVertices[3] = Vec2(1.0f, 1.0f);
    asteroid->polygon.untransformedVertices[4] = Vec2(0.0f, 0.0f);
#else
    asteroid->polygon.vertexCount = 10;
    asteroid->polygon.untransformedVertices[0] = Vec2(-0.3f, 0.7f);
    asteroid->polygon.untransformedVertices[1] = Vec2(-0.7f, 0.5f);
    asteroid->polygon.untransformedVertices[2] = Vec2(-0.7f, -0.3f);
    asteroid->polygon.untransformedVertices[3] = Vec2(-0.3f, -0.6f);
    asteroid->polygon.untransformedVertices[4] = Vec2(0.3f, -0.6f);
    asteroid->polygon.untransformedVertices[5] = Vec2(0.9f, -0.3f);
    asteroid->polygon.untransformedVertices[6] = Vec2(0.6f, 0.1f);
    asteroid->polygon.untransformedVertices[7] = Vec2(0.7f, 0.4f);
    asteroid->polygon.untransformedVertices[8] = Vec2(0.4f, 0.6f);
    asteroid->polygon.untransformedVertices[9] = Vec2(0.1f, 0.4f);
#endif

    TriangulatePolygonResult triangulateResult = triangulatePolygon(&asteroid->polygon);
    for (int i = 0; i < triangulateResult.trianglesCount; ++i) {
        asteroid->collisionPolygons[i] = triangulateResult.triangles[i];
    }
    asteroid->collisionPolygonsCount = triangulateResult.trianglesCount;
    
    transformAsteroid(asteroid);
}

static void destroyAsteroid(Asteroid *asteroid) {
    // Split the asteroid into smaller ones.
    if (asteroid->scale > ASTEROID_SCALE_SMALL) {
        for (int j = 0; j < 2; ++j) {
            for (int i = 0; i < arrayCount(g_asteroids); ++i) {
                if (g_asteroids[i].active) {
                    continue;
                }
                vec2 randOffset = randomDirection() * randomFloat(20, 60);
                vec2 position = asteroid->position + randOffset;

                float curVelAngle = atan2(asteroid->velocity.y, asteroid->velocity.x) * RAD_TO_DEG;
                float offsetRange = 60.0f;
                float randVelAngle = randomFloat(curVelAngle - offsetRange, curVelAngle + offsetRange) * DEG_TO_RAD;
                float randSpeedCoeff = randomFloat(10.0f, 18.0f)/10.0f;
                float speed = len(asteroid->velocity) * randSpeedCoeff;
                vec2 velocity = Vec2(cosf(randVelAngle), sinf(randVelAngle)) * speed;

                float scale = 0.0f;
                if (asteroid->scale == ASTEROID_SCALE_BIG) {
                    scale = ASTEROID_SCALE_MEDIUM;
                }
                else {
                    scale = ASTEROID_SCALE_SMALL;
                }

                createAsteroid(&g_asteroids[i], position, velocity, scale);
                break;
            }
        }
    }

    // Create explosion particles.
    int numParticlesToCreate = randomInt(4, 10);
    int numParticlesCreated = 0;
    for (int particleIndex = 0;
         particleIndex < arrayCount(g_explosionParticles) && numParticlesCreated < numParticlesToCreate;
         ++particleIndex)
    {
        ExplosionParticle *particle = &g_explosionParticles[particleIndex];
        if (particle->active) {
            continue;
        }
        particle->active = true;
        particle->position = asteroid->position;
        particle->velocity = randomDirection() * randomFloat(50, 200);
        particle->distance = 0;
        particle->maxDistance = randomFloat(20, 100);

        numParticlesCreated++;
    }
    
    asteroid->active = false;
}

static void startLevel() {
    initPlayer(&g_player);
    g_levelEndTimer = 0.0f;

    for (int i = 0; i < 1 + g_currentLevel; ++i) {
        // Start position should be along the screen fringe to avoid collisions
        // with the player ship.
        float fringeWidth = SCREEN_WIDTH*0.25f;
        float fringeHeight = SCREEN_HEIGHT*0.25f;
        float x1 = fringeWidth;
        float x2 = SCREEN_WIDTH - fringeWidth;
        float y1 = fringeHeight;
        float y2 = SCREEN_HEIGHT - fringeHeight;
        float x = 0;
        float y = 0;
        int attempts = 100;
        while (attempts) {
            x = randomFloat(SCREEN_WIDTH);
            y = randomFloat(SCREEN_HEIGHT);
            if (!(x > x1 && x < x2 && y > y1 && y < y2)) {
                break;
            }
            attempts--;
        }
        vec2 position = Vec2(x, y);
        vec2 velocity = randomDirection() * randomFloat(20, 100);
        createAsteroid(&g_asteroids[i], position, velocity, ASTEROID_SCALE_BIG);
    }
}

void setViewport(float windowWidth, float windowHeight) {
    g_windowWidth = windowWidth;
    g_windowHeight = windowHeight;
    float aspectRatio = SCREEN_WIDTH / SCREEN_HEIGHT;
    float reverseAspectRatio = 1.0f / aspectRatio;
    float viewportX, viewportY;
    float viewportWidth, viewportHeight;
    if ((windowWidth * reverseAspectRatio) > windowHeight) {
        viewportHeight = windowHeight;
        viewportWidth = viewportHeight * aspectRatio;
        viewportX = 0.5f*(windowWidth - viewportWidth);
        viewportY = 0;
    }
    else {
        viewportWidth = windowWidth;
        viewportHeight = viewportWidth * reverseAspectRatio;
        viewportX = 0;
        viewportY = 0.5f*(windowHeight - viewportHeight);
    }
    g_viewportX = viewportX;
    g_viewportY = viewportY;
    g_viewportWidth = viewportWidth;
    g_viewportHeight = viewportHeight;
    glViewport((GLint)viewportX, (GLint)viewportY,
               (GLsizei)viewportWidth, (GLsizei)viewportHeight);
}

bool initGame() {
    char vertexShaderCode[] =
        "attribute vec4 position;"
        "uniform mat4 projectionMatrix;"
        "void main() {"
        "    gl_Position = projectionMatrix * position;"
        "    gl_PointSize = 4.0;"
        "}";

    char fragmentShaderCode[] =
        "precision mediump float;"
        "uniform vec4 color;"
        "void main() {"
        "    gl_FragColor = color;"
        "}";

    GLuint vertexShader = compileShader(vertexShaderCode, GL_VERTEX_SHADER);
    GLuint fragmentShader = compileShader(fragmentShaderCode, GL_FRAGMENT_SHADER);

    if (!vertexShader || !fragmentShader) {
        return false;
    }

    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    glUseProgram(shaderProgram);

    g_positionAttrib = glGetAttribLocation(shaderProgram, "position");
    glEnableVertexAttribArray(g_positionAttrib);

    mat4 projectionMatrix = createOrthoMatrix(-1.0f, SCREEN_WIDTH-1.0f,
                                         -1.0f, SCREEN_HEIGHT-1.0f,
                                         -1.0f, 1.0f);
    GLint projectionMatrixUniform = glGetUniformLocation(shaderProgram, "projectionMatrix");
    glUniformMatrix4fv(projectionMatrixUniform, 1, GL_FALSE, projectionMatrix.e);

    g_colorUniform = glGetUniformLocation(shaderProgram, "color");

    seedRandom((unsigned)time(NULL));
    //seedRandom(127);

    g_input.leftButton.position = Vec2(0, 0);
    g_input.leftButton.dimensions = Vec2(150, 150);
    g_input.rightButton.position = Vec2(g_input.leftButton.position.x + g_input.leftButton.dimensions.x, 0);
    g_input.rightButton.dimensions = g_input.leftButton.dimensions;
    g_input.fireButton.position = Vec2(SCREEN_WIDTH - g_input.leftButton.dimensions.x, 0);
    g_input.fireButton.dimensions = g_input.leftButton.dimensions;
    g_input.forwardButton.position = Vec2(g_input.fireButton.position.x - g_input.fireButton.dimensions.x, 0);
    g_input.forwardButton.dimensions = g_input.leftButton.dimensions;

    startLevel();
    return true;
}

void gameUpdateAndRender(float dt, float *touches) {
    bool asteroidsExist = false;
    for (int i = 0; i < arrayCount(g_asteroids); ++i) {
        if (g_asteroids[i].active) {
            asteroidsExist = true;
            break;
        }
    }
    if (!asteroidsExist) {
        g_levelEndTimer += dt;
        if (g_levelEndTimer > LEVEL_END_DURATION) {
            ++g_currentLevel;
            startLevel();
        }
    }

#ifndef _WIN32
    for (int i = 0; i < BUTTONS_COUNT; ++i) {
        Button *button = &g_input.buttons[i];
        button->isDown = false;

        for (int touchCoordIndex = 0; touchCoordIndex < 4; touchCoordIndex += 2) {
            float x_ = touches[touchCoordIndex];
            float flippedY = touches[touchCoordIndex + 1];

            if (x_ < 0 || flippedY < 0) {
                continue;
            }

            float y_ = g_windowHeight - flippedY;
            float x = ((x_ - g_viewportX) / g_viewportWidth) * SCREEN_WIDTH;
            float y = ((y_ - g_viewportY) / g_viewportHeight) * SCREEN_HEIGHT;
            //LOGI("x = %f, y = %f", x, y);

            if (x >= button->position.x &&
                x < button->position.x + button->dimensions.x &&
                y >= button->position.y &&
                y < button->position.y + button->dimensions.y)
            {
                button->isDown = true;
                break;
            }
        }
    }
#endif

    if (g_player.alive) {
        float rotationSpeed = 7.0f;
        if (g_input.rightButton.isDown) {
            g_player.angle -= rotationSpeed*dt;
        }
        else if (g_input.leftButton.isDown) {
            g_player.angle += rotationSpeed*dt;
        }

        // Deceleration.
        vec2 prevVelDirection = normalize(g_player.vel);
        g_player.vel -= prevVelDirection * PLAYER_DECELERATION * dt;
        float dotProduct = dot(normalize(g_player.vel), prevVelDirection);
        if (dotProduct == -1.0f) {
            // Velocity has changed its direction to the opposite, stop the
            // ship to prevent it from moving backwards.
            g_player.vel = Vec2(0.0f, 0.0f);
        }

        vec2 playerDirection = Vec2(cosf(g_player.angle), sinf(g_player.angle));
        if (g_input.forwardButton.isDown) {
            g_player.accel = playerDirection * PLAYER_ACCELERATION;
            if (!g_input.forwardButton.wasDown) {
                g_player.flameVisible = true;
                g_player.flameFlickTimer = 0;
            }
        }
        else {
            g_player.accel = Vec2(0.0f, 0.0f);
            g_player.flameVisible = false;
            g_player.flameFlickTimer = 0;
        }
        g_player.vel += g_player.accel*dt;
        if (len(g_player.vel) > MAX_PLAYER_SPEED) {
            g_player.vel = normalize(g_player.vel)*MAX_PLAYER_SPEED;
        }
        g_player.pos += g_player.vel*dt;

        // World wrapping for the player ship.
        float playerExtraSize = 20.0f;
        if (g_player.pos.x > (SCREEN_WIDTH + playerExtraSize)) {
            g_player.pos.x = -playerExtraSize;
        }
        else if (g_player.pos.x < -playerExtraSize) {
            g_player.pos.x = SCREEN_WIDTH + playerExtraSize;
        }
        if (g_player.pos.y > (SCREEN_HEIGHT + playerExtraSize)) {
            g_player.pos.y = -playerExtraSize;
        }
        else if (g_player.pos.y < -playerExtraSize) {
            g_player.pos.y = SCREEN_HEIGHT + playerExtraSize;
        }

        g_player.flameFlickTimer += dt;
        if ((len(g_player.accel) > 0.0f) && (g_player.flameFlickTimer > 0.03f)) {
            g_player.flameFlickTimer = 0.0f;
            g_player.flameVisible = !g_player.flameVisible;
        }

        if (g_input.fireButton.isDown && !g_input.fireButton.wasDown) {
            for (int i = 0; i < arrayCount(g_bullets); ++i) {
                if (!g_bullets[i].active) {
                    g_bullets[i].active = true;
                    g_bullets[i].position = g_player.pos;
                    g_bullets[i].velocity = playerDirection * BULLET_SPEED;
                    g_bullets[i].distance = 0;
                    break;
                }
            }
        }
    }

    for (int i = 0; i < arrayCount(g_asteroids); ++i) {
        if (!g_asteroids[i].active) {
            continue;
        }
        g_asteroids[i].position += g_asteroids[i].velocity * dt;

        // World wrapping.
        if (g_asteroids[i].bounds.min.x > SCREEN_WIDTH) {
            g_asteroids[i].position.x = -(g_asteroids[i].bounds.max.x - g_asteroids[i].position.x);
        }
        else if (g_asteroids[i].bounds.max.x < 0) {
            g_asteroids[i].position.x = SCREEN_WIDTH + (g_asteroids[i].position.x - g_asteroids[i].bounds.min.x);
        }
        if (g_asteroids[i].bounds.min.y > SCREEN_HEIGHT) {
            g_asteroids[i].position.y = -(g_asteroids[i].bounds.max.y - g_asteroids[i].position.y);
        }
        else if (g_asteroids[i].bounds.max.y < 0) {
            g_asteroids[i].position.y = SCREEN_HEIGHT + (g_asteroids[i].position.y - g_asteroids[i].bounds.min.y);
        }

        transformAsteroid(&g_asteroids[i]);
    }

    mat4 playerTranslationMatrix = {};
    mat4 playerScaleMatrix = {};
    mat4 playerRotationMatrix = {};
    mat4 playerModelMatrix = {};
    if (g_player.alive) {
        playerTranslationMatrix = createTranslationMatrix(g_player.pos.x, g_player.pos.y);
        playerScaleMatrix = createScaleMatrix(20.0f);
        playerRotationMatrix = createRotationMatrix(PI/2.0f - g_player.angle);
        playerModelMatrix = playerTranslationMatrix * playerRotationMatrix * playerScaleMatrix;

        for (int j = 0; j < g_player.collisionPolygon.vertexCount; ++j) {
            g_player.collisionPolygon.vertices[j] = playerModelMatrix * g_player.collisionPolygon.untransformedVertices[j];
        }

        bool collision = false;
        for (int i = 0; i < arrayCount(g_asteroids) && !collision; ++i) {
            if (!g_asteroids[i].active) {
                continue;
            }
            for (int j = 0; j < g_asteroids[i].collisionPolygonsCount && !collision; ++j) {
                if (polygonsIntersect(&g_player.collisionPolygon, &g_asteroids[i].collisionPolygons[j])) {
                    destroyAsteroid(&g_asteroids[i]);
                    g_player.alive = false;
                    g_player.reviveTimer = 0;
                    for (int k = 0; k < arrayCount(g_shipFragments); ++k) {
                        float offsetRange = 10.0f;
                        g_shipFragments[k].position = g_player.pos + Vec2(randomFloat(-offsetRange, offsetRange), randomFloat(-offsetRange, offsetRange));
                        g_shipFragments[k].velocity = randomDirection() * randomFloat(20, 50);
                        g_shipFragments[k].scale = 20.0f;
                        g_shipFragments[k].angle = randomAngle();
                    }
                    collision = true;
                }
            }
        }
    }

    if (!g_player.alive) {
        for (int i = 0; i < arrayCount(g_shipFragments); ++i) {
            g_shipFragments[i].position += g_shipFragments[i].velocity * dt;
        }
        g_player.reviveTimer += dt;
        if (g_player.reviveTimer > PLAYER_REVIVE_DURATION) {
            initPlayer(&g_player);
        }
    }

    for (int i = 0; i < arrayCount(g_bullets); ++i) {
        if (!g_bullets[i].active) {
            continue;
        }

        vec2 offset = g_bullets[i].velocity * dt;
        g_bullets[i].position += offset;
        g_bullets[i].distance += len(offset);
        if (g_bullets[i].distance > MAX_BULLET_DISTANCE) {
            g_bullets[i].active = false;
            continue;
        }

        // World wrapping.
        float extraSize = 0;
        if (g_bullets[i].position.x > (SCREEN_WIDTH + extraSize)) {
            g_bullets[i].position.x = -extraSize;
        }
        else if (g_bullets[i].position.x < -extraSize) {
            g_bullets[i].position.x = SCREEN_WIDTH + extraSize;
        }
        if (g_bullets[i].position.y > (SCREEN_HEIGHT + extraSize)) {
            g_bullets[i].position.y = -extraSize;
        }
        else if (g_bullets[i].position.y < -extraSize) {
            g_bullets[i].position.y = SCREEN_HEIGHT + extraSize;
        }

        for (int asteroidIndex = 0; asteroidIndex < arrayCount(g_asteroids); ++asteroidIndex) {
            Asteroid *asteroid = &g_asteroids[asteroidIndex];
            if (!asteroid->active) {
                continue;
            }
            if (isPointInPolygon(g_bullets[i].position, &asteroid->polygon)) {
                g_bullets[i].active = false;
                destroyAsteroid(asteroid);
                break;
            }
        }
    }

    for (int i = 0; i < arrayCount(g_explosionParticles); ++i) {
        if (!g_explosionParticles[i].active) {
            continue;
        }
        vec2 offset = g_explosionParticles[i].velocity * dt;
        g_explosionParticles[i].position += offset;
        g_explosionParticles[i].distance += len(offset);
        if (g_explosionParticles[i].distance > g_explosionParticles[i].maxDistance) {
            g_explosionParticles[i].active = false;
        }
    }

    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT);

    float whiteColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    float redColor[] = { 1.0f, 0.0f, 0.0f, 1.0f };
    float greenColor[] = { 0.0f, 1.0f, 0.0f, 1.0f };
    float blueColor[] = { 0.0f, 0.0f, 1.0f, 1.0f };
    glUniform4fv(g_colorUniform, 1, whiteColor);

#if 1
    GLfloat border[] = { 0.0f, 0.0f,
                         0.0f, SCREEN_HEIGHT-1.0f,
                         SCREEN_WIDTH-1.0f, SCREEN_HEIGHT-1.0f,
                         SCREEN_WIDTH-1.0f, 0.0f };
    glVertexAttribPointer(g_positionAttrib, 2, GL_FLOAT, GL_FALSE, 0, border);
    glDrawArrays(GL_LINE_LOOP, 0, arrayCount(border)/2);
#endif

    if (g_player.alive) {
        for (int j = 0; j < PLAYER_SHIP_VERTICES_COUNT; ++j) {
            transformedPlayerShipVertices[j] = playerModelMatrix * playerShipVertices[j];
        }
        glVertexAttribPointer(g_positionAttrib, 2, GL_FLOAT, GL_FALSE, 0, transformedPlayerShipVertices);
        glDrawArrays(GL_LINES, 0, PLAYER_SHIP_VERTICES_COUNT);

        if (g_player.flameVisible) {
            for (int j = 0; j < PLAYER_FLAME_VERTICES_COUNT; ++j) {
                transformedPlayerFlameVertices[j] = playerModelMatrix * playerFlameVertices[j];
            }
            glVertexAttribPointer(g_positionAttrib, 2, GL_FLOAT, GL_FALSE, 0, transformedPlayerFlameVertices);
            glDrawArrays(GL_LINES, 0, PLAYER_FLAME_VERTICES_COUNT);
        }
#if 0
        float velScale = 1.0f;
        GLfloat velocityVectorVertices[] = {
            playerPos.x, playerPos.y,
            playerPos.x + playerVel.x*velScale, playerPos.y + playerVel.y*velScale
        };
        glUniformMatrix4fv(modelMatrixUniform, 1, GL_FALSE, identityMatrix.e);
        glVertexAttribPointer(g_positionAttrib, 2, GL_FLOAT, GL_FALSE, 0, velocityVectorVertices);
        float velVectorColor[] = { 1.0f, 0.0f, 0.0f, 1.0f };
        glUniform4fv(g_colorUniform, 1, velVectorColor);
        glDrawArrays(GL_LINES, 0, arrayCount(velocityVectorVertices)/2);

        float accelScale = 1.0f;
        GLfloat accelVectorVertices[] = {
            playerPos.x, playerPos.y,
            playerPos.x + playerAccel.x*accelScale, playerPos.y + playerAccel.y*accelScale
        };
        glUniformMatrix4fv(modelMatrixUniform, 1, GL_FALSE, identityMatrix.e);
        glVertexAttribPointer(g_positionAttrib, 2, GL_FLOAT, GL_FALSE, 0, accelVectorVertices);
        float accelVectorColor[] = { 0.0f, 1.0f, 0.0f, 1.0f };
        glUniform4fv(g_colorUniform, 1, accelVectorColor);
        glDrawArrays(GL_LINES, 0, arrayCount(accelVectorVertices)/2);
#endif
    }

    for (int i = 0; i < arrayCount(g_asteroids); ++i) {
        if (!g_asteroids[i].active) {
            continue;
        }
#if 1
        glVertexAttribPointer(g_positionAttrib, 2, GL_FLOAT, GL_FALSE, 0, g_asteroids[i].polygon.vertices);
        glDrawArrays(GL_LINE_LOOP, 0, g_asteroids[i].polygon.vertexCount);
#endif

#if 0
        // Draw asteroid's collision polygons.
        glUniform4fv(g_colorUniform, 1, redColor);
        for (int collisionPolIndex = 0;
             collisionPolIndex < g_asteroids[i].collisionPolygonsCount;
             ++collisionPolIndex)
        {
            glVertexAttribPointer(g_positionAttrib, 2, GL_FLOAT, GL_FALSE, 0, g_asteroids[i].collisionPolygons[collisionPolIndex].vertices);
            glDrawArrays(GL_LINE_LOOP, 0, g_asteroids[i].collisionPolygons[collisionPolIndex].vertexCount);
        }
        glUniform4fv(g_colorUniform, 1, whiteColor);
#endif
    }

    if (!g_player.alive) {
        for (int i = 0; i < arrayCount(g_shipFragments); ++i) {
            mat4 translationMatrix = createTranslationMatrix(g_shipFragments[i].position.x, g_shipFragments[i].position.y);
            mat4 scaleMatrix = createScaleMatrix(g_shipFragments[i].scale);
            mat4 rotationMatrix = createRotationMatrix(g_shipFragments[i].angle);
            mat4 modelMatrix = translationMatrix * rotationMatrix * scaleMatrix;
            for (int j = 0; j < SHIP_FRAGMENT_VERTICES_COUNT; ++j) {
                transformedShipFragmentVertices[j] = modelMatrix * shipFragmentVertices[j];
            }
            glVertexAttribPointer(g_positionAttrib, 2, GL_FLOAT, GL_FALSE, 0, transformedShipFragmentVertices);
            glDrawArrays(GL_LINE_LOOP, 0, SHIP_FRAGMENT_VERTICES_COUNT);
        }
    }

    for (int i = 0; i < arrayCount(g_bullets); ++i) {
        if (!g_bullets[i].active) {
            continue;
        }
        float bulletVertices[] = {g_bullets[i].position.x, g_bullets[i].position.y};
        glVertexAttribPointer(g_positionAttrib, 2, GL_FLOAT, GL_FALSE, 0, bulletVertices);
        glDrawArrays(GL_POINTS, 0, 1);
    }

    for (int i = 0; i < arrayCount(g_explosionParticles); ++i) {
        if (!g_explosionParticles[i].active) {
            continue;
        }
        float vertices[] = {g_explosionParticles[i].position.x, g_explosionParticles[i].position.y};
        glVertexAttribPointer(g_positionAttrib, 2, GL_FLOAT, GL_FALSE, 0, vertices);
        glDrawArrays(GL_POINTS, 0, 1);
    }

    for (int i = 0; i < BUTTONS_COUNT; ++i) {
        Button *button = &g_input.buttons[i];
        float buttonVertices[] = {
            button->position.x, button->position.y,
            button->position.x + button->dimensions.x, button->position.y,
            button->position.x + button->dimensions.x, button->position.y + button->dimensions.y,
            button->position.x, button->position.y + button->dimensions.y,
        };
        glVertexAttribPointer(g_positionAttrib, 2, GL_FLOAT, GL_FALSE, 0, buttonVertices);
        if (button->isDown) {
            unsigned short indices[] = {0,1,2,2,3,0};
            glDrawElements(GL_TRIANGLES, arrayCount(indices), GL_UNSIGNED_SHORT, indices);
        }
        else {
            glDrawArrays(GL_LINE_LOOP, 0, arrayCount(buttonVertices)/2);
        }
    }

    for (int i = 0; i < BUTTONS_COUNT; ++i) {
        Button *button = &g_input.buttons[i];
        button->wasDown = button->isDown;
    }
}
