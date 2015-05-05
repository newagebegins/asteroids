#include <stdio.h>
#include <time.h>
#include <assert.h>
#include <string.h>
#ifdef _WIN32
#include "gl3w.h"
#else
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#endif
#include "mygame.h"
#include "mygame_triangulator.h"
#include "mygame_random.h"

#define SCREEN_WIDTH 960.0f
#define SCREEN_HEIGHT 540.0f
#define MAX_PLAYER_SPEED 500.0f
#define PLAYER_ACCELERATION 250.0f
#define PLAYER_DECELERATION 50.0f
#define PLAYER_REVIVE_DURATION 1.0f
#define BULLET_SPEED 600.0f
#define MAX_BULLET_DISTANCE 400.0f
#define ASTEROID_SCALE_SMALL 20
#define ASTEROID_SCALE_MEDIUM 60
#define ASTEROID_SCALE_BIG 90
#define LEVEL_END_DURATION 2.0f

#define arrayCount(array) (sizeof(array) / sizeof((array)[0]))

void(*platformLog)(const char *, ...);

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

#define SHIP_FRAGMENT_VERTICES_COUNT 2
static vec2 shipFragmentVertices[SHIP_FRAGMENT_VERTICES_COUNT] = {
	Vec2(-0.5f, 0.0f),
	Vec2(0.5f, 0.0f),
};
static vec2 transformedShipFragmentVertices[SHIP_FRAGMENT_VERTICES_COUNT] = { 0 };

struct Player {
	bool alive;
	vec2 pos;
	vec2 vel;
	vec2 accel;
	float angle;
	float reviveTimer;
	float flameFlickTimer;
	bool flameVisible;
	
	vec2 collisionPolygon[3];
	vec2 transformedCollisionPolygon[3];
	
	vec2 shipVertices[6];
	vec2 transformedShipVertices[6];

	vec2 flameVertices[4];
	vec2 transformedFlameVertices[4];
};
static void transformPlayer(Player *player) {
	mat4 translationMatrix = createTranslationMatrix(player->pos.x, player->pos.y);
	mat4 scaleMatrix = createScaleMatrix(20.0f);
	mat4 rotationMatrix = createRotationMatrix(PI / 2.0f - player->angle);
	mat4 modelMatrix = translationMatrix * rotationMatrix * scaleMatrix;

	for (int i = 0; i < arrayCount(player->collisionPolygon); ++i) {
		player->transformedCollisionPolygon[i] = modelMatrix * player->collisionPolygon[i];
	}
	for (int i = 0; i < arrayCount(player->shipVertices); ++i) {
		player->transformedShipVertices[i] = modelMatrix * player->shipVertices[i];
	}
	for (int i = 0; i < arrayCount(player->flameVertices); ++i) {
		player->transformedFlameVertices[i] = modelMatrix * player->flameVertices[i];
	}
}
static void initPlayer(Player *player) {
	player->alive = true;
	player->pos = Vec2(SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2);
	player->vel = Vec2(0.0f, 0.0f);
	player->accel = Vec2(0.0f, 0.0f);
	player->angle = PI / 2;
	player->flameFlickTimer = 0.0f;
	player->flameVisible = false;

	player->collisionPolygon[0] = Vec2(0.0f, 0.9f);
	player->collisionPolygon[1] = Vec2(-0.54f, -0.49f);
	player->collisionPolygon[2] = Vec2(0.54f, -0.49f);

	transformPlayer(player);
}

#define MAX_ASTEROID_COLLISION_VERTEX_COUNT 64
struct Asteroid {
	bool active;
	vec2 position;
	vec2 velocity;
	float scale;
	vec2 *vertices;
	int vertexCount;
	vec2 transformedPolygon[64];
	vec2 *collisionTriangles;
	vec2 transformedCollisionTriangles[MAX_ASTEROID_COLLISION_VERTEX_COUNT];
	int collisionVertexCount;
	MyRectangle bounds;
};

static vec2 g_asteroidVertices1[] = {
	Vec2(-0.5f, 0.8f),
	Vec2(-0.3f, 0.4f),
	Vec2(-0.9f, 0.4f),
	Vec2(-0.9f, -0.2f),
	Vec2(-0.3f, -0.8f),
	Vec2(0.2f, -0.7f),
	Vec2(0.5f, -0.8f),
	Vec2(0.9f, -0.3f),
	Vec2(0.1f, 0.0f),
	Vec2(0.9f, 0.2f),
	Vec2(0.9f, 0.4f),
	Vec2(0.2f, 0.8f),
};
static vec2 g_asteroidCollisionTriangles1[MAX_ASTEROID_COLLISION_VERTEX_COUNT];
static int g_asteroidCollisionVertexCount1;

static vec2 g_asteroidVertices2[] = {
	Vec2(-0.4f, 0.7f),
	Vec2(-0.7f, 0.4f),
	Vec2(-0.6f, 0.0f),
	Vec2(-0.7f, -0.3f),
	Vec2(-0.5f, -0.6f),
	Vec2(-0.2f, -0.5f),
	Vec2(0.4f, -0.6f),
	Vec2(0.8f, -0.1f),
	Vec2(0.5f, 0.3f),
	Vec2(0.7f, 0.5f),
	Vec2(0.4f, 0.7f),
	Vec2(0.0f, 0.6f),
};
static vec2 g_asteroidCollisionTriangles2[MAX_ASTEROID_COLLISION_VERTEX_COUNT];
static int g_asteroidCollisionVertexCount2;

static vec2 g_asteroidVertices3[] = {
	Vec2(-0.3f, 0.7f),
	Vec2(-0.7f, 0.5f),
	Vec2(-0.7f, -0.3f),
	Vec2(-0.3f, -0.6f),
	Vec2(0.3f, -0.6f),
	Vec2(0.9f, -0.3f),
	Vec2(0.6f, 0.1f),
	Vec2(0.7f, 0.4f),
	Vec2(0.4f, 0.6f),
	Vec2(0.1f, 0.4f),
};
static vec2 g_asteroidCollisionTriangles3[MAX_ASTEROID_COLLISION_VERTEX_COUNT];
static int g_asteroidCollisionVertexCount3;

static vec2 g_asteroidVertices4[] = {
	Vec2(-0.3f, 0.5f),
	Vec2(-0.6f, 0.2f),
	Vec2(-0.3f, 0.0f),
	Vec2(-0.7f, -0.2f),
	Vec2(-0.3f, -0.6f),
	Vec2(0.1f, -0.2f),
	Vec2(0.1f, -0.6f),
	Vec2(0.3f, -0.6f),
	Vec2(0.6f, -0.2f),
	Vec2(0.6f, 0.2f),
	Vec2(0.2f, 0.5f),
};
static vec2 g_asteroidCollisionTriangles4[MAX_ASTEROID_COLLISION_VERTEX_COUNT];
static int g_asteroidCollisionVertexCount4;

static vec2 g_letterVertices0[] = {
	Vec2(1.0f, 0.6f), Vec2(0.6f, 1.0f),
	Vec2(0.6f, 1.0f), Vec2(-0.6f, 1.0f),
	Vec2(-0.6f, 1.0f), Vec2(-1.0f, 0.6f),
	Vec2(-1.0f, 0.6f), Vec2(-1.0f, -0.6f),
	Vec2(-1.0f, -0.6f), Vec2(-0.6f, -1.0f),
	Vec2(-0.6f, -1.0f), Vec2(0.6f, -1.0f),
	Vec2(0.6f, -1.0f), Vec2(1.0f, -0.6f),
	Vec2(1.0f, -0.6f), Vec2(1.0f, 0.6f),
	Vec2(-0.8f, -0.8f), Vec2(0.8f, 0.8f),
};
static vec2 g_letterVertices1[] = {
	Vec2(-1.0f, -1.0f), Vec2(1.0f, -1.0f),
	Vec2(0.0f, -1.0f), Vec2(0.0f, 1.0f),
	Vec2(0.0f, 1.0f), Vec2(-0.4f, 0.6f),
};
static vec2 g_letterVertices2[] = {
	Vec2(-1.0f, 0.7f), Vec2(-0.7f, 1.0f),
	Vec2(-0.7f, 1.0f), Vec2(0.7f, 1.0f),
	Vec2(0.7f, 1.0f), Vec2(1.0f, 0.7f),
	Vec2(1.0f, 0.7f), Vec2(1.0f, 0.2f),
	Vec2(1.0f, 0.2f), Vec2(0.7f, -0.1f),
	Vec2(0.7f, -0.1f), Vec2(-0.7f, -0.1f),
	Vec2(-0.7f, -0.1f), Vec2(-1.0f, -0.4f),
	Vec2(-1.0f, -0.4f), Vec2(-1.0f, -1.0f),
	Vec2(-1.0f, -1.0f), Vec2(1.0f, -1.0f),
};
static vec2 g_letterVertices3[] = {
	Vec2(-1.0f, 0.7f), Vec2(-0.7f, 1.0f),
	Vec2(-0.7f, 1.0f), Vec2(0.7f, 1.0f),
	Vec2(0.7f, 1.0f), Vec2(1.0f, 0.6f),
	Vec2(1.0f, 0.6f), Vec2(0.7f, 0.2f),
	Vec2(0.7f, 0.2f), Vec2(0.0f, 0.2f),
	Vec2(0.7f, 0.2f), Vec2(1.0f, -0.1f),
	Vec2(1.0f, -0.1f), Vec2(1.0f, -0.7f),
	Vec2(1.0f, -0.7f), Vec2(0.7f, -1.0f),
	Vec2(0.7f, -1.0f), Vec2(-0.7f, -1.0f),
	Vec2(-0.7f, -1.0f), Vec2(-1.0f, -0.7f),
};
static vec2 g_letterVertices4[] = {
	Vec2(0.0f, -1.0f), Vec2(0.0f, 1.0f),
	Vec2(0.0f, 1.0f), Vec2(-1.0f, 0.0f),
	Vec2(-1.0f, 0.0f), Vec2(0.8f, 0.0f),
};
static vec2 g_letterVertices5[] = {
	Vec2(1.0f, 1.0f), Vec2(-1.0f, 1.0f),
	Vec2(-1.0f, 1.0f), Vec2(-1.0f, 0.2f),
	Vec2(-1.0f, 0.2f), Vec2(0.7f, 0.2f),
	Vec2(0.7f, 0.2f), Vec2(1.0f, -0.1f),
	Vec2(1.0f, -0.1f), Vec2(1.0f, -0.7f),
	Vec2(1.0f, -0.7f), Vec2(0.7f, -1.0f),
	Vec2(0.7f, -1.0f), Vec2(-0.7f, -1.0f),
	Vec2(-0.7f, -1.0f), Vec2(-1.0f, -0.7f),
};
static vec2 g_letterVertices6[] = {
	Vec2(0.7f, 1.0f), Vec2(-0.7f, 1.0f),
	Vec2(-0.7f, 1.0f), Vec2(-1.0f, 0.7f),
	Vec2(-1.0f, 0.7f), Vec2(-1.0f, -0.7f),
	Vec2(-1.0f, -0.7f), Vec2(-0.7f, -1.0f),
	Vec2(-0.7f, -1.0f), Vec2(0.7f, -1.0f),
	Vec2(0.7f, -1.0f), Vec2(1.0f, -0.7f),
	Vec2(1.0f, -0.7f), Vec2(1.0f, -0.1f),
	Vec2(1.0f, -0.1f), Vec2(0.7f, 0.2f),
	Vec2(0.7f, 0.2f), Vec2(-1.0f, 0.2f),
};
static vec2 g_letterVertices7[] = {
	Vec2(-1.0f, 1.0f), Vec2(1.0f, 1.0f),
	Vec2(1.0f, 1.0f), Vec2(1.0f, 0.7f),
	Vec2(1.0f, 0.7f), Vec2(-0.4f, -0.6f),
	Vec2(-0.4f, -0.6f), Vec2(-0.4f, -1.0f),
};
static vec2 g_letterVertices8[] = {
	Vec2(-0.7f, 0.4f), Vec2(-1.0f, 0.7f),
	Vec2(-1.0f, 0.7f), Vec2(-0.7f, 1.0f),
	Vec2(-0.7f, 1.0f), Vec2(0.7f, 1.0f),
	Vec2(0.7f, 1.0f), Vec2(1.0f, 0.7f),
	Vec2(1.0f, 0.7f), Vec2(0.7f, 0.4f),
	Vec2(0.7f, 0.4f), Vec2(-0.7f, 0.4f),
	Vec2(-0.7f, 0.4f), Vec2(-1.0f, 0.1f),
	Vec2(-1.0f, 0.1f), Vec2(-1.0f, -0.7f),
	Vec2(-1.0f, -0.7f), Vec2(-0.7f, -1.0f),
	Vec2(-0.7f, -1.0f), Vec2(0.7f, -1.0f),
	Vec2(0.7f, -1.0f), Vec2(1.0f, -0.7f),
	Vec2(1.0f, -0.7f), Vec2(1.0f, 0.1f),
	Vec2(1.0f, 0.1f), Vec2(0.7f, 0.4f),
};
static vec2 g_letterVertices9[] = {
	Vec2(-0.7f, -1.0f), Vec2(0.7f, -1.0f),
	Vec2(0.7f, -1.0f), Vec2(1.0f, -0.7f),
	Vec2(1.0f, -0.7f), Vec2(1.0f, 0.7f),
	Vec2(1.0f, 0.7f), Vec2(0.7f, 1.0f),
	Vec2(0.7f, 1.0f), Vec2(-0.7f, 1.0f),
	Vec2(-0.7f, 1.0f), Vec2(-1.0f, 0.7f),
	Vec2(-1.0f, 0.7f), Vec2(-1.0f, 0.1f),
	Vec2(-1.0f, 0.1f), Vec2(-0.7f, -0.2f),
	Vec2(-0.7f, -0.2f), Vec2(1.0f, -0.2f),
};
static vec2 g_letterVerticesA[] = {
	Vec2(1.0f, -1.0f), Vec2(1.0f, 0.6f),
	Vec2(1.0f, 0.6f), Vec2(0.6f, 1.0f),
	Vec2(0.6f, 1.0f), Vec2(-0.6f, 1.0f),
	Vec2(-0.6f, 1.0f), Vec2(-1.0f, 0.6f),
	Vec2(-1.0f, 0.6f), Vec2(-1.0f, -1.0f),
	Vec2(-1.0f, 0.0f), Vec2(1.0f, 0.0f),
};
static vec2 g_letterVerticesE[] = {
	Vec2(1.0f, 1.0f), Vec2(-1.0f, 1.0f),
	Vec2(-1.0f, 1.0f), Vec2(-1.0f, -1.0f),
	Vec2(-1.0f, -1.0f), Vec2(1.0f, -1.0f),
	Vec2(-1.0f, 0.2f), Vec2(0.6f, 0.2f),
};
static vec2 g_letterVerticesG[] = {
	Vec2(1.0f, 0.6f), Vec2(0.6f, 1.0f),
	Vec2(0.6f, 1.0f), Vec2(-0.6f, 1.0f),
	Vec2(-0.6f, 1.0f), Vec2(-1.0f, 0.6f),
	Vec2(-1.0f, 0.6f), Vec2(-1.0f, -0.6f),
	Vec2(-1.0f, -0.6f), Vec2(-0.6f, -1.0f),
	Vec2(-0.6f, -1.0f), Vec2(0.6f, -1.0f),
	Vec2(0.6f, -1.0f), Vec2(1.0f, -0.6f),
	Vec2(1.0f, -0.6f), Vec2(1.0f, 0.0f),
	Vec2(1.0f, 0.0f), Vec2(0.0f, 0.0f),
};
static vec2 g_letterVerticesI[] = {
	Vec2(-1.0f, 1.0f), Vec2(1.0f, 1.0f),
	Vec2(-1.0f, -1.0f), Vec2(1.0f, -1.0f),
	Vec2(0.0f, 1.0f), Vec2(0.0f, -1.0f),
};
static vec2 g_letterVerticesM[] = {
	Vec2(1.0f, -1.0f), Vec2(1.0f, 1.0f),
	Vec2(1.0f, 1.0f), Vec2(0.0f, 0.2f),
	Vec2(0.0f, 0.2f), Vec2(-1.0f, 1.0f),
	Vec2(-1.0f, 1.0f), Vec2(-1.0f, -1.0f),
};
static vec2 g_letterVerticesN[] = {
	Vec2(-1.0f, -1.0f), Vec2(-1.0f, 1.0f),
	Vec2(-1.0f, 1.0f), Vec2(1.0f, -1.0f),
	Vec2(1.0f, -1.0f), Vec2(1.0f, 1.0f),
};
static vec2 g_letterVerticesO[] = {
	Vec2(1.0f, 0.6f), Vec2(0.6f, 1.0f),
	Vec2(0.6f, 1.0f), Vec2(-0.6f, 1.0f),
	Vec2(-0.6f, 1.0f), Vec2(-1.0f, 0.6f),
	Vec2(-1.0f, 0.6f), Vec2(-1.0f, -0.6f),
	Vec2(-1.0f, -0.6f), Vec2(-0.6f, -1.0f),
	Vec2(-0.6f, -1.0f), Vec2(0.6f, -1.0f),
	Vec2(0.6f, -1.0f), Vec2(1.0f, -0.6f),
	Vec2(1.0f, -0.6f), Vec2(1.0f, 0.6f),
};
static vec2 g_letterVerticesR[] = {
	Vec2(-1.0f, 0.0f), Vec2(0.6f, 0.0f),
	Vec2(0.6f, 0.0f), Vec2(1.0f, 0.3f),
	Vec2(1.0f, 0.3f), Vec2(1.0f, 0.7f),
	Vec2(1.0f, 0.7f), Vec2(0.6f, 1.0f),
	Vec2(0.6f, 1.0f), Vec2(-1.0f, 1.0f),
	Vec2(-1.0f, 1.0f), Vec2(-1.0f, -1.0f),
	Vec2(0.2f, 0.0f), Vec2(1.0f, -1.0f),
};
static vec2 g_letterVerticesS[] = {
	Vec2(0.7f, 1.0f), Vec2(-0.7f, 1.0f),
	Vec2(-0.7f, 1.0f), Vec2(-1.0f, 0.7f),
	Vec2(-1.0f, 0.7f), Vec2(-1.0f, 0.4f),
	Vec2(-1.0f, 0.4f), Vec2(-0.7f, 0.1f),
	Vec2(-0.7f, 0.1f), Vec2(0.7f, 0.1f),
	Vec2(0.7f, 0.1f), Vec2(1.0f, -0.2f),
	Vec2(1.0f, -0.2f), Vec2(1.0f, -0.7f),
	Vec2(1.0f, -0.7f), Vec2(0.7f, -1.0f),
	Vec2(0.7f, -1.0f), Vec2(-0.7f, -1.0f),
	Vec2(-0.7f, -1.0f), Vec2(-1.0f, -0.7f),
};
static vec2 g_letterVerticesT[] = {
	Vec2(1.0f, 1.0f), Vec2(-1.0f, 1.0f),
	Vec2(0.0f, 1.0f), Vec2(0.0f, -1.0f),
};
static vec2 g_letterVerticesV[] = {
	Vec2(-1.0f, 1.0f), Vec2(-1.0f, -0.2f),
	Vec2(-1.0f, -0.2f), Vec2(0.0f, -1.0f),
	Vec2(0.0f, -1.0f), Vec2(1.0f, -0.2f),
	Vec2(1.0f, -0.2f), Vec2(1.0f, 1.0f),
};
static vec2 g_letterVerticesExclamationMark[] = {
	Vec2(-0.3f, 1.0f), Vec2(0.3f, 1.0f),
	Vec2(0.3f, 1.0f), Vec2(0.0f, -0.3f),
	Vec2(0.0f, -0.3f), Vec2(-0.3f, 1.0f),

	Vec2(-0.2f, -0.6f), Vec2(0.2f, -0.6f),
	Vec2(0.2f, -0.6f), Vec2(0.2f, -1.0f),
	Vec2(0.2f, -1.0f), Vec2(-0.2f, -1.0f),
	Vec2(-0.2f, -1.0f), Vec2(-0.2f, -0.6f),
};

struct ShipFragment {
	vec2 position;
	vec2 velocity;
	float scale;
	float angle;
};

struct Bullet {
	bool active;
	bool player;
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

struct Ufo {
	bool active;
	vec2 position;
	vec2 velocity;
	float scale;
	vec2 outlineVertices[8];
	vec2 transformedOutlineVertices[8];
	vec2 innerLines[4];
	vec2 transformedInnerLines[4];
	vec2 collisionTriangles[64];
	vec2 transformedCollisionTriangles[64];
	int collisionVertexCount;
	MyRectangle bounds;
	float changeDirectionTimer;
	float changeDirectionDuration;
	float nextShotTimer;
	float nextShotDuration;
};

Input g_input;

static GLint g_positionAttrib;
static GLint g_colorUniform;
static Player g_player;
static Asteroid g_asteroids[20];
static ShipFragment g_shipFragments[5];
static Bullet g_bullets[32];
static float g_levelEndTimer;
static int g_currentLevel;
static float g_windowWidth;
static float g_windowHeight;
static float g_viewportX;
static float g_viewportY;
static float g_viewportWidth;
static float g_viewportHeight;
static ExplosionParticle g_explosionParticles[30];
static Ufo g_ufo;
static float g_nextUfoTimer;
static float g_nextUfoDuration;
static int g_playerLivesCount;

#define GAME_OVER_DURATION 1.0f
static float g_gameOverTimer;

static int g_restartingCounter;
#define RESTARTING_DURATION 1.0f
static float g_restartingTimer;

static void transformAsteroid(Asteroid *asteroid) {
	mat4 translationMatrix = createTranslationMatrix(asteroid->position.x, asteroid->position.y);
	mat4 scaleMatrix = createScaleMatrix(asteroid->scale);
	mat4 modelMatrix = translationMatrix * scaleMatrix;
	for (int i = 0; i < asteroid->vertexCount; ++i) {
		asteroid->transformedPolygon[i] = modelMatrix * asteroid->vertices[i];
	}
	for (int i = 0; i < asteroid->collisionVertexCount; ++i) {
		asteroid->transformedCollisionTriangles[i] = modelMatrix * asteroid->collisionTriangles[i];
	}
	asteroid->bounds = getPolygonBounds(asteroid->transformedPolygon, asteroid->vertexCount);
}

static void transformUfo(Ufo *ufo) {
	mat4 translationMatrix = createTranslationMatrix(ufo->position.x, ufo->position.y);
	mat4 scaleMatrix = createScaleMatrix(ufo->scale);
	mat4 modelMatrix = translationMatrix * scaleMatrix;
	for (int i = 0; i < arrayCount(ufo->outlineVertices); ++i) {
		ufo->transformedOutlineVertices[i] = modelMatrix * ufo->outlineVertices[i];
	}
	for (int i = 0; i < arrayCount(ufo->innerLines); ++i) {
		ufo->transformedInnerLines[i] = modelMatrix * ufo->innerLines[i];
	}
	for (int i = 0; i < ufo->collisionVertexCount; ++i) {
		ufo->transformedCollisionTriangles[i] = modelMatrix * ufo->collisionTriangles[i];
	}
	ufo->bounds = getPolygonBounds(ufo->transformedOutlineVertices, arrayCount(ufo->outlineVertices));
}

static void createAsteroid(Asteroid *asteroid, vec2 position, vec2 velocity, float scale) {
	asteroid->active = true;
	asteroid->position = position;
	asteroid->velocity = velocity;
	asteroid->scale = scale;
	int type = randomInt(1, 4);

	switch (type) {
	case 1:
		asteroid->vertices = g_asteroidVertices1;
		asteroid->vertexCount = arrayCount(g_asteroidVertices1);
		asteroid->collisionTriangles = g_asteroidCollisionTriangles1;
		asteroid->collisionVertexCount = g_asteroidCollisionVertexCount1;
		break;

	case 2:
		asteroid->vertices = g_asteroidVertices2;
		asteroid->vertexCount = arrayCount(g_asteroidVertices2);
		asteroid->collisionTriangles = g_asteroidCollisionTriangles2;
		asteroid->collisionVertexCount = g_asteroidCollisionVertexCount2;
		break;

	case 3:
		asteroid->vertices = g_asteroidVertices3;
		asteroid->vertexCount = arrayCount(g_asteroidVertices3);
		asteroid->collisionTriangles = g_asteroidCollisionTriangles3;
		asteroid->collisionVertexCount = g_asteroidCollisionVertexCount3;
		break;

	case 4:
		asteroid->vertices = g_asteroidVertices4;
		asteroid->vertexCount = arrayCount(g_asteroidVertices4);
		asteroid->collisionTriangles = g_asteroidCollisionTriangles4;
		asteroid->collisionVertexCount = g_asteroidCollisionVertexCount4;
		break;
	}

	transformAsteroid(asteroid);
}

static void destroyPlayer() {
	assert(g_playerLivesCount > 0);
	g_player.alive = false;
	g_player.reviveTimer = 0;
	g_playerLivesCount--;
	for (int k = 0; k < arrayCount(g_shipFragments); ++k) {
		float offsetRange = 10.0f;
		g_shipFragments[k].position = g_player.pos + Vec2(randomFloat(-offsetRange, offsetRange), randomFloat(-offsetRange, offsetRange));
		g_shipFragments[k].velocity = randomDirection() * randomFloat(20, 50);
		g_shipFragments[k].scale = 20.0f;
		g_shipFragments[k].angle = randomAngle();
	}
}

static void createExplosion(vec2 position) {
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
		particle->position = position;
		particle->velocity = randomDirection() * randomFloat(50, 200);
		particle->distance = 0;
		particle->maxDistance = randomFloat(20, 100);

		numParticlesCreated++;
	}
}

static void destroyAsteroid(Asteroid *asteroid) {
	// Split the asteroid into smaller ones.
	if (asteroid->scale > ASTEROID_SCALE_SMALL) {
		for (int j = 0; j < 2; ++j) {
			for (int i = 0; i < arrayCount(g_asteroids); ++i) {
				if (g_asteroids[i].active) {
					continue;
				}
				float curVelAngle = atan2(asteroid->velocity.y, asteroid->velocity.x) * RAD_TO_DEG;
				float offsetRange = 60.0f;
				float randVelAngle = randomFloat(curVelAngle - offsetRange, curVelAngle + offsetRange) * DEG_TO_RAD;
				float randSpeedCoeff = randomFloat(10.0f, 18.0f) / 10.0f;
				float speed = len(asteroid->velocity) * randSpeedCoeff;
				vec2 velocity = Vec2(cosf(randVelAngle), sinf(randVelAngle)) * speed;

				float scale = 0.0f;
				if (asteroid->scale == ASTEROID_SCALE_BIG) {
					scale = ASTEROID_SCALE_MEDIUM;
				}
				else {
					scale = ASTEROID_SCALE_SMALL;
				}

				createAsteroid(&g_asteroids[i], asteroid->position, velocity, scale);
				break;
			}
		}
	}
	createExplosion(asteroid->position);
	asteroid->active = false;
}

static void startLevel() {
	initPlayer(&g_player);
	g_levelEndTimer = 0.0f;

	for (int i = 0; i < arrayCount(g_asteroids); ++i) {
		g_asteroids[i].active = false;
	}
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

	for (int i = 0; i < arrayCount(g_bullets); ++i) {
		g_bullets[i].active = false;
	}
	for (int i = 0; i < arrayCount(g_explosionParticles); ++i) {
		g_explosionParticles[i].active = false;
	}
	
	g_ufo.active = false;
	g_nextUfoTimer = 0;
}

static void startGame() {
	g_currentLevel = 2;
	g_playerLivesCount = 1;
	g_gameOverTimer = 0;
	g_restartingCounter = 3;
	g_restartingTimer = 0;
	startLevel();
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
	glViewport((GLint)viewportX, (GLint)viewportY, (GLsizei)viewportWidth, (GLsizei)viewportHeight);
}

static void drawString(char *str, int length, float x, float y) {
	for (int i = 0; i < length; ++i) {
		int vertexCount = 0;
		vec2 *vertices = 0;
		switch (str[i]) {
		case '0':
			vertexCount = arrayCount(g_letterVertices0);
			vertices = g_letterVertices0;
			break;

		case '1':
			vertexCount = arrayCount(g_letterVertices1);
			vertices = g_letterVertices1;
			break;

		case '2':
			vertexCount = arrayCount(g_letterVertices2);
			vertices = g_letterVertices2;
			break;

		case '3':
			vertexCount = arrayCount(g_letterVertices3);
			vertices = g_letterVertices3;
			break;

		case '4':
			vertexCount = arrayCount(g_letterVertices4);
			vertices = g_letterVertices4;
			break;

		case '5':
			vertexCount = arrayCount(g_letterVertices5);
			vertices = g_letterVertices5;
			break;

		case '6':
			vertexCount = arrayCount(g_letterVertices6);
			vertices = g_letterVertices6;
			break;

		case '7':
			vertexCount = arrayCount(g_letterVertices7);
			vertices = g_letterVertices7;
			break;

		case '8':
			vertexCount = arrayCount(g_letterVertices8);
			vertices = g_letterVertices8;
			break;

		case '9':
			vertexCount = arrayCount(g_letterVertices9);
			vertices = g_letterVertices9;
			break;

		case 'A':
			vertexCount = arrayCount(g_letterVerticesA);
			vertices = g_letterVerticesA;
			break;

		case 'E':
			vertexCount = arrayCount(g_letterVerticesE);
			vertices = g_letterVerticesE;
			break;

		case 'G':
			vertexCount = arrayCount(g_letterVerticesG);
			vertices = g_letterVerticesG;
			break;

		case 'I':
			vertexCount = arrayCount(g_letterVerticesI);
			vertices = g_letterVerticesI;
			break;

		case 'M':
			vertexCount = arrayCount(g_letterVerticesM);
			vertices = g_letterVerticesM;
			break;

		case 'N':
			vertexCount = arrayCount(g_letterVerticesN);
			vertices = g_letterVerticesN;
			break;

		case 'O':
			vertexCount = arrayCount(g_letterVerticesO);
			vertices = g_letterVerticesO;
			break;

		case 'R':
			vertexCount = arrayCount(g_letterVerticesR);
			vertices = g_letterVerticesR;
			break;

		case 'S':
			vertexCount = arrayCount(g_letterVerticesS);
			vertices = g_letterVerticesS;
			break;

		case 'T':
			vertexCount = arrayCount(g_letterVerticesT);
			vertices = g_letterVerticesT;
			break;

		case 'V':
			vertexCount = arrayCount(g_letterVerticesV);
			vertices = g_letterVerticesV;
			break;

		case '!':
			vertexCount = arrayCount(g_letterVerticesExclamationMark);
			vertices = g_letterVerticesExclamationMark;
			break;
		}
		if (vertices) {
			mat4 translationMatrix = createTranslationMatrix(x + i * 27, y);
			mat4 scaleMatrix = createScaleMatrix(10.0f);
			mat4 modelMatrix = translationMatrix * scaleMatrix;
			vec2 transformedLetterVertices[64];
			for (int j = 0; j < vertexCount; ++j) {
				transformedLetterVertices[j] = modelMatrix * vertices[j];
			}
			glVertexAttribPointer(g_positionAttrib, 2, GL_FLOAT, GL_FALSE, 0, transformedLetterVertices);
			glDrawArrays(GL_LINES, 0, vertexCount);
		}
	}
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

	mat4 projectionMatrix = createOrthoMatrix(-1.0f, SCREEN_WIDTH - 1.0f,
		-1.0f, SCREEN_HEIGHT - 1.0f,
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

	// ship left side
	g_player.shipVertices[0] = Vec2(0.0f, 0.9f);
	g_player.shipVertices[1] = Vec2(-0.7f, -0.9f);
	// ship right side
	g_player.shipVertices[2] = Vec2(0.0f, 0.9f);
	g_player.shipVertices[3] = Vec2(0.7f, -0.9f);
	// ship bottom side
	g_player.shipVertices[4] = Vec2(-0.54f, -0.49f);
	g_player.shipVertices[5] = Vec2(0.54f, -0.49f);

	// flame left side
	g_player.flameVertices[0] = Vec2(-0.3f, -0.49f);
	g_player.flameVertices[1] = Vec2(0.0f, -1.0f);
	// flame right side
	g_player.flameVertices[2] = Vec2(0.3f, -0.49f);
	g_player.flameVertices[3] = Vec2(0.0f, -1.0f);

	triangulatePolygon(g_asteroidVertices1, arrayCount(g_asteroidVertices1), g_asteroidCollisionTriangles1, &g_asteroidCollisionVertexCount1);
	triangulatePolygon(g_asteroidVertices2, arrayCount(g_asteroidVertices2), g_asteroidCollisionTriangles2, &g_asteroidCollisionVertexCount2);
	triangulatePolygon(g_asteroidVertices3, arrayCount(g_asteroidVertices3), g_asteroidCollisionTriangles3, &g_asteroidCollisionVertexCount3);
	triangulatePolygon(g_asteroidVertices4, arrayCount(g_asteroidVertices4), g_asteroidCollisionTriangles4, &g_asteroidCollisionVertexCount4);

	g_ufo.outlineVertices[0] = Vec2(-0.3f, 0.4f);
	g_ufo.outlineVertices[1] = Vec2(-0.4f, 0.1f);
	g_ufo.outlineVertices[2] = Vec2(-0.9f, -0.1f);
	g_ufo.outlineVertices[3] = Vec2(-0.6f, -0.4f);
	g_ufo.outlineVertices[4] = Vec2(0.6f, -0.4f);
	g_ufo.outlineVertices[5] = Vec2(0.9f, -0.1f);
	g_ufo.outlineVertices[6] = Vec2(0.4f, 0.1f);
	g_ufo.outlineVertices[7] = Vec2(0.3f, 0.4f);

	g_ufo.innerLines[0] = Vec2(-0.4f, 0.1f);
	g_ufo.innerLines[1] = Vec2(0.4f, 0.1f);

	g_ufo.innerLines[2] = Vec2(-0.9f, -0.1f);
	g_ufo.innerLines[3] = Vec2(0.9f, -0.1f);

	triangulatePolygon(g_ufo.outlineVertices, arrayCount(g_ufo.outlineVertices), g_ufo.collisionTriangles, &g_ufo.collisionVertexCount);
	g_nextUfoDuration = 0.0f;

	startGame();
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

		transformPlayer(&g_player);

		// World wrapping for the player ship.
		float playerExtraSize = 20.0f;
		if (g_player.pos.x > (SCREEN_WIDTH + playerExtraSize)) {
			g_player.pos.x = -playerExtraSize;
		}
		else if (g_player.pos.x < -playerExtraSize) {
			g_player.pos.x = SCREEN_WIDTH + playerExtraSize;
		}
		if (g_player.pos.y >(SCREEN_HEIGHT + playerExtraSize)) {
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
					g_bullets[i].player = true;
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
		transformAsteroid(&g_asteroids[i]);

		// World wrapping for asteroids.
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

		if (g_player.alive && polygonTrianglesIntersect(g_asteroids[i].transformedCollisionTriangles, g_asteroids[i].collisionVertexCount / 3, g_player.transformedCollisionPolygon, arrayCount(g_player.transformedCollisionPolygon) / 3)) {
			// Collision between the asteroid and the player.
			destroyAsteroid(&g_asteroids[i]);
			destroyPlayer();
		}
		else if (g_ufo.active && polygonTrianglesIntersect(g_asteroids[i].transformedCollisionTriangles, g_asteroids[i].collisionVertexCount / 3, g_ufo.transformedCollisionTriangles, g_ufo.collisionVertexCount / 3)) {
			// Collision between the asteroid and the UFO.
			destroyAsteroid(&g_asteroids[i]);
			g_ufo.active = false;
			createExplosion(g_ufo.position);
		}
	}

	if (g_player.alive && g_ufo.active && polygonTrianglesIntersect(g_ufo.transformedCollisionTriangles, g_ufo.collisionVertexCount / 3, g_player.transformedCollisionPolygon, arrayCount(g_player.transformedCollisionPolygon) / 3)) {
		// Collision between the player and the UFO.
		destroyPlayer();
	}

	if (!g_ufo.active) {
		g_nextUfoTimer += dt;
		if (g_nextUfoTimer > g_nextUfoDuration) {
			g_nextUfoTimer = 0;
			g_nextUfoDuration = 1.0f;

			g_ufo.active = true;
			bool moveFromLeftToRight = randomInt(1) == 1;
			float x = 0.0f;
			float y = randomFloat(0.0f, SCREEN_HEIGHT);
			if (moveFromLeftToRight) {
				x = 0;
				g_ufo.velocity = Vec2(100.0f, 0.0f);
			}
			else {
				x = SCREEN_WIDTH;
				g_ufo.velocity = Vec2(-100.0f, 0.0f);
			}
			g_ufo.position = Vec2(x, y);
			g_ufo.scale = 40;
			g_ufo.changeDirectionDuration = 2.0f;
			g_ufo.nextShotDuration = 0.5f;
			transformUfo(&g_ufo);
		}
	}
	else {
		g_ufo.position += g_ufo.velocity * dt;

		if (g_ufo.bounds.max.x < 0 || g_ufo.bounds.min.x > SCREEN_WIDTH) {
			g_ufo.active = false;
		}
		else {
			// Wrap around the screen only vertically.
			if (g_ufo.bounds.min.y > SCREEN_HEIGHT) {
				g_ufo.position.y = -(g_ufo.bounds.max.y - g_ufo.position.y);
			}
			else if (g_ufo.bounds.max.y < 0) {
				g_ufo.position.y = SCREEN_HEIGHT + (g_ufo.position.y - g_ufo.bounds.min.y);
			}

			g_ufo.changeDirectionTimer += dt;
			if (g_ufo.changeDirectionTimer > g_ufo.changeDirectionDuration) {
				float speed = len(g_ufo.velocity);
				g_ufo.velocity = normalize(g_ufo.velocity);
				if (g_ufo.velocity.y == 0.0f) {
					g_ufo.velocity.y = randomInt(1) ? 1.0f : -1.0f;
				}
				else {
					g_ufo.velocity.y = 0;
				}
				g_ufo.velocity = normalize(g_ufo.velocity) * speed;
				g_ufo.changeDirectionTimer = 0;
				g_ufo.changeDirectionDuration = randomFloat(5, 20) / 10.0f;
			}

			g_ufo.nextShotTimer += dt;
			if (g_ufo.nextShotTimer > g_ufo.nextShotDuration) {
				for (int i = 0; i < arrayCount(g_bullets); i++) {
					Bullet *bullet = &g_bullets[i];
					if (!bullet->active) {
						bullet->active = true;
						bullet->player = false;
						bullet->position = g_ufo.position;
						bullet->velocity = randomDirection() * BULLET_SPEED;
						bullet->distance = 0;
						break;
					}
				}
				g_ufo.nextShotTimer = 0;
				g_ufo.nextShotDuration = randomFloat(5, 20) / 10.0f;
			}

			transformUfo(&g_ufo);
		}
	}

	if (!g_player.alive) {
		for (int i = 0; i < arrayCount(g_shipFragments); ++i) {
			g_shipFragments[i].position += g_shipFragments[i].velocity * dt;
		}
		if (g_playerLivesCount > 0) {
			g_player.reviveTimer += dt;
			if (g_player.reviveTimer > PLAYER_REVIVE_DURATION) {
				initPlayer(&g_player);
			}
		}
	}

	for (int i = 0; i < arrayCount(g_bullets); ++i) {
		if (!g_bullets[i].active) {
			continue;
		}

		vec2 offset = g_bullets[i].velocity * dt;

		// Check for collisions between enemy bullets and the player.
		if (g_player.alive && !g_bullets[i].player) {
			if (lineIntersectsPolygon(g_bullets[i].position, g_bullets[i].position + offset, g_player.transformedCollisionPolygon, arrayCount(g_player.transformedCollisionPolygon))) {
				g_bullets[i].active = false;
				destroyPlayer();
				continue;
			}
		}

		g_bullets[i].position += offset;
		g_bullets[i].distance += len(offset);
		if (g_bullets[i].distance > MAX_BULLET_DISTANCE) {
			g_bullets[i].active = false;
			continue;
		}

		// World wrapping for bullets.
		if (g_bullets[i].position.x > SCREEN_WIDTH) {
			g_bullets[i].position.x = 0;
		}
		else if (g_bullets[i].position.x < 0) {
			g_bullets[i].position.x = SCREEN_WIDTH;
		}
		if (g_bullets[i].position.y > SCREEN_HEIGHT) {
			g_bullets[i].position.y = 0;
		}
		else if (g_bullets[i].position.y < 0) {
			g_bullets[i].position.y = SCREEN_HEIGHT;
		}

		// Check for collisions between bullets and asteroids.
		for (int asteroidIndex = 0; asteroidIndex < arrayCount(g_asteroids); ++asteroidIndex) {
			Asteroid *asteroid = &g_asteroids[asteroidIndex];
			if (!asteroid->active) {
				continue;
			}
			if (isPointInPolygon(g_bullets[i].position, asteroid->transformedPolygon, asteroid->vertexCount)) {
				g_bullets[i].active = false;
				destroyAsteroid(asteroid);
				break;
			}
		}

		// Check for collisions between player bullets and the UFO.
		if (g_ufo.active && g_bullets[i].player && isPointInPolygon(g_bullets[i].position, g_ufo.transformedOutlineVertices, arrayCount(g_ufo.transformedOutlineVertices))) {
			g_bullets[i].active = false;
			g_ufo.active = false;
			createExplosion(g_ufo.position);
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

#if 0
	GLfloat border[] = { 0.0f, 0.0f,
		0.0f, SCREEN_HEIGHT - 1.0f,
		SCREEN_WIDTH - 1.0f, SCREEN_HEIGHT - 1.0f,
		SCREEN_WIDTH - 1.0f, 0.0f };
	glVertexAttribPointer(g_positionAttrib, 2, GL_FLOAT, GL_FALSE, 0, border);
	glDrawArrays(GL_LINE_LOOP, 0, arrayCount(border) / 2);
#endif

	if (g_player.alive) {
		glVertexAttribPointer(g_positionAttrib, 2, GL_FLOAT, GL_FALSE, 0, g_player.transformedShipVertices);
		glDrawArrays(GL_LINES, 0, arrayCount(g_player.transformedShipVertices));

		if (g_player.flameVisible) {
			glVertexAttribPointer(g_positionAttrib, 2, GL_FLOAT, GL_FALSE, 0, g_player.transformedFlameVertices);
			glDrawArrays(GL_LINES, 0, arrayCount(g_player.transformedFlameVertices));
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
		glVertexAttribPointer(g_positionAttrib, 2, GL_FLOAT, GL_FALSE, 0, g_asteroids[i].transformedPolygon);
		glDrawArrays(GL_LINE_LOOP, 0, g_asteroids[i].vertexCount);
#endif

#if 0
		// Draw asteroid's collision polygons.
		glUniform4fv(g_colorUniform, 1, redColor);
		int triangleCount = g_asteroids[i].collisionVertexCount / 3;
		for (int j = 0;	j < triangleCount; ++j) {
			vec2 triangle[3];
			triangle[0] = g_asteroids[i].transformedCollisionTriangles[j * 3 + 0];
			triangle[1] = g_asteroids[i].transformedCollisionTriangles[j * 3 + 1];
			triangle[2] = g_asteroids[i].transformedCollisionTriangles[j * 3 + 2];
			glVertexAttribPointer(g_positionAttrib, 2, GL_FLOAT, GL_FALSE, 0, triangle);
			glDrawArrays(GL_LINE_LOOP, 0, 3);
		}
		glUniform4fv(g_colorUniform, 1, whiteColor);
#endif
	}

	if (g_ufo.active) {
		glVertexAttribPointer(g_positionAttrib, 2, GL_FLOAT, GL_FALSE, 0, g_ufo.transformedOutlineVertices);
		glDrawArrays(GL_LINE_LOOP, 0, arrayCount(g_ufo.transformedOutlineVertices));

		glVertexAttribPointer(g_positionAttrib, 2, GL_FLOAT, GL_FALSE, 0, g_ufo.transformedInnerLines);
		glDrawArrays(GL_LINES, 0, arrayCount(g_ufo.transformedInnerLines));

#if 0
		// Draw ufo's collision polygons.
		glUniform4fv(g_colorUniform, 1, redColor);
		int triangleCount = g_ufo.collisionVertexCount / 3;
		for (int j = 0; j < triangleCount; ++j) {
			vec2 triangle[3];
			triangle[0] = g_ufo.transformedCollisionTriangles[j * 3 + 0];
			triangle[1] = g_ufo.transformedCollisionTriangles[j * 3 + 1];
			triangle[2] = g_ufo.transformedCollisionTriangles[j * 3 + 2];
			glVertexAttribPointer(g_positionAttrib, 2, GL_FLOAT, GL_FALSE, 0, triangle);
			glDrawArrays(GL_LINE_LOOP, 0, 3);
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
		float bulletVertices[] = { g_bullets[i].position.x, g_bullets[i].position.y };
		glVertexAttribPointer(g_positionAttrib, 2, GL_FLOAT, GL_FALSE, 0, bulletVertices);
		glDrawArrays(GL_POINTS, 0, 1);
	}

	for (int i = 0; i < arrayCount(g_explosionParticles); ++i) {
		if (!g_explosionParticles[i].active) {
			continue;
		}
		float vertices[] = { g_explosionParticles[i].position.x, g_explosionParticles[i].position.y };
		glVertexAttribPointer(g_positionAttrib, 2, GL_FLOAT, GL_FALSE, 0, vertices);
		glDrawArrays(GL_POINTS, 0, 1);
	}

#ifndef _WIN32
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
			unsigned short indices[] = { 0, 1, 2, 2, 3, 0 };
			glDrawElements(GL_TRIANGLES, arrayCount(indices), GL_UNSIGNED_SHORT, indices);
		}
		else {
			glDrawArrays(GL_LINE_LOOP, 0, arrayCount(buttonVertices) / 2);
		}
	}
#endif

	for (int i = 0; i < g_playerLivesCount; i++) {
		mat4 translationMatrix = createTranslationMatrix(30 + i * 21, 480);
		mat4 scaleMatrix = createScaleMatrix(12.0f);
		mat4 modelMatrix = translationMatrix * scaleMatrix;
		vec2 transformedShipVertices[arrayCount(g_player.shipVertices)];
		for (int j = 0; j < arrayCount(g_player.shipVertices); ++j) {
			transformedShipVertices[j] = modelMatrix * g_player.shipVertices[j];
		}
		glVertexAttribPointer(g_positionAttrib, 2, GL_FLOAT, GL_FALSE, 0, transformedShipVertices);
		glDrawArrays(GL_LINES, 0, arrayCount(transformedShipVertices));
	}

	for (int i = 0; i < BUTTONS_COUNT; ++i) {
		Button *button = &g_input.buttons[i];
		button->wasDown = button->isDown;
	}

	if (g_playerLivesCount <= 0) {
		if (g_restartingCounter > 0) {
			char gameOverStr[] = "GAME OVER";
			drawString(gameOverStr, arrayCount(gameOverStr) - 1, 370, 340);
		}

		g_gameOverTimer += dt;
		if (g_gameOverTimer > GAME_OVER_DURATION) {
			char restartingStr[] = "RESTARTING IN 3";
			float x = 290, y = 280;
			switch (g_restartingCounter) {
			case 2:
				restartingStr[14] = '2';
				break;
			case 1:
				restartingStr[14] = '1';
				break;
			case 0:
				restartingStr[0] = 'G';
				restartingStr[1] = 'O';
				restartingStr[2] = '!';
				restartingStr[3] = '\0';
				x = 460;
				break;
			}
			
			if (restartingStr) {
				drawString(restartingStr, strlen(restartingStr), x, y);
			}

			g_restartingTimer += dt;
			if (g_restartingTimer > RESTARTING_DURATION) {
				g_restartingTimer = 0;
				g_restartingCounter--;
				if (g_restartingCounter < 0) {
					startGame();
				}
			}
		}
	}
}
