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
#include "mygame_triangulator.h"
#include "mygame_random.h"

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
};
inline void initPlayer(Player *player) {
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

vec2 asteroidVertices1[] = {
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
vec2 asteroidCollisionTriangles1[MAX_ASTEROID_COLLISION_VERTEX_COUNT];
int asteroidCollisionVertexCount1;

vec2 asteroidVertices2[] = {
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
vec2 asteroidCollisionTriangles2[MAX_ASTEROID_COLLISION_VERTEX_COUNT];
int asteroidCollisionVertexCount2;

vec2 asteroidVertices3[] = {
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
vec2 asteroidCollisionTriangles3[MAX_ASTEROID_COLLISION_VERTEX_COUNT];
int asteroidCollisionVertexCount3;

vec2 asteroidVertices4[] = {
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
vec2 asteroidCollisionTriangles4[MAX_ASTEROID_COLLISION_VERTEX_COUNT];
int asteroidCollisionVertexCount4;

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
	for (int i = 0; i < asteroid->vertexCount; ++i) {
		asteroid->transformedPolygon[i] = modelMatrix * asteroid->vertices[i];
	}
	for (int i = 0; i < asteroid->collisionVertexCount; ++i) {
		asteroid->transformedCollisionTriangles[i] = modelMatrix * asteroid->collisionTriangles[i];
	}
	asteroid->bounds = getPolygonBounds(asteroid->transformedPolygon, asteroid->vertexCount);
}

static void createAsteroid(Asteroid *asteroid, vec2 position, vec2 velocity, float scale) {
	asteroid->active = true;
	asteroid->position = position;
	asteroid->velocity = velocity;
	asteroid->scale = scale;
	int type = randomInt(1, 4);

	switch (type) {
	case 1:
		asteroid->vertices = asteroidVertices1;
		asteroid->vertexCount = arrayCount(asteroidVertices1);
		asteroid->collisionTriangles = asteroidCollisionTriangles1;
		asteroid->collisionVertexCount = asteroidCollisionVertexCount1;
		break;

	case 2:
		asteroid->vertices = asteroidVertices2;
		asteroid->vertexCount = arrayCount(asteroidVertices2);
		asteroid->collisionTriangles = asteroidCollisionTriangles2;
		asteroid->collisionVertexCount = asteroidCollisionVertexCount2;
		break;

	case 3:
		asteroid->vertices = asteroidVertices3;
		asteroid->vertexCount = arrayCount(asteroidVertices3);
		asteroid->collisionTriangles = asteroidCollisionTriangles3;
		asteroid->collisionVertexCount = asteroidCollisionVertexCount3;
		break;

	case 4:
		asteroid->vertices = asteroidVertices4;
		asteroid->vertexCount = arrayCount(asteroidVertices4);
		asteroid->collisionTriangles = asteroidCollisionTriangles4;
		asteroid->collisionVertexCount = asteroidCollisionVertexCount4;
		break;
	}

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
	glViewport((GLint)viewportX, (GLint)viewportY, (GLsizei)viewportWidth, (GLsizei)viewportHeight);
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

	triangulatePolygon(asteroidVertices1, arrayCount(asteroidVertices1), asteroidCollisionTriangles1, &asteroidCollisionVertexCount1);
	triangulatePolygon(asteroidVertices2, arrayCount(asteroidVertices2), asteroidCollisionTriangles2, &asteroidCollisionVertexCount2);
	triangulatePolygon(asteroidVertices3, arrayCount(asteroidVertices3), asteroidCollisionTriangles3, &asteroidCollisionVertexCount3);
	triangulatePolygon(asteroidVertices4, arrayCount(asteroidVertices4), asteroidCollisionTriangles4, &asteroidCollisionVertexCount4);

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
		playerRotationMatrix = createRotationMatrix(PI / 2.0f - g_player.angle);
		playerModelMatrix = playerTranslationMatrix * playerRotationMatrix * playerScaleMatrix;

		for (int j = 0; j < arrayCount(g_player.collisionPolygon); ++j) {
			g_player.transformedCollisionPolygon[j] = playerModelMatrix * g_player.collisionPolygon[j];
		}

		bool collision = false;
		for (int i = 0; i < arrayCount(g_asteroids) && !collision; ++i) {
			if (!g_asteroids[i].active) {
				continue;
			}
			int trianglesCount = g_asteroids[i].collisionVertexCount/3;
			for (int j = 0; j < trianglesCount && !collision; ++j) {
				vec2 triangle[3];
				triangle[0] = g_asteroids[i].transformedCollisionTriangles[j * 3 + 0];
				triangle[1] = g_asteroids[i].transformedCollisionTriangles[j * 3 + 1];
				triangle[2] = g_asteroids[i].transformedCollisionTriangles[j * 3 + 2];
				if (polygonsIntersect(g_player.transformedCollisionPolygon, arrayCount(g_player.transformedCollisionPolygon), triangle, 3)) {
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
		if (g_bullets[i].position.y >(SCREEN_HEIGHT + extraSize)) {
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
			if (isPointInPolygon(g_bullets[i].position, asteroid->transformedPolygon, asteroid->vertexCount)) {
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
		0.0f, SCREEN_HEIGHT - 1.0f,
		SCREEN_WIDTH - 1.0f, SCREEN_HEIGHT - 1.0f,
		SCREEN_WIDTH - 1.0f, 0.0f };
	glVertexAttribPointer(g_positionAttrib, 2, GL_FLOAT, GL_FALSE, 0, border);
	glDrawArrays(GL_LINE_LOOP, 0, arrayCount(border) / 2);
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
		glVertexAttribPointer(g_positionAttrib, 2, GL_FLOAT, GL_FALSE, 0, g_asteroids[i].transformedPolygon);
		glDrawArrays(GL_LINE_LOOP, 0, g_asteroids[i].vertexCount);
#endif

#if 1
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

	for (int i = 0; i < BUTTONS_COUNT; ++i) {
		Button *button = &g_input.buttons[i];
		button->wasDown = button->isDown;
	}
}
