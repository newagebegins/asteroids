#ifndef MYGAME_RANDOM_H
#define MYGAME_RANDOM_H

#include "mygame_math.h"

#define RANDOM_UINT_MAX 65535

static unsigned long sRandomSeed = 0;

inline void seedRandom(unsigned long seed) {
	sRandomSeed = seed;
}

inline unsigned long randomUInt() {
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

inline vec2 randomDirection() {
	float angle = randomAngle();
	vec2 result = Vec2(cosf(angle), sinf(angle));
	return result;
}

#endif
