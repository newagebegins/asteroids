#ifndef MYGAME_MATH_H
#define MYGAME_MATH_H

#include <math.h>

#define PI 3.14159265f
#define DEG_TO_RAD 0.0174532925f
#define RAD_TO_DEG 57.2957795f;

struct vec2 {
	float x, y;
};

inline int signum(float x) {
	if (x > 0) {
		return 1;
	}
	if (x < 0) {
		return -1;
	}
	return 0;
}

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
		result = a*(1.0f / len(a));
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
	r[0] = a[0] * b[0] + a[4] * b[1] + a[8] * b[2] + a[12] * b[3];
	r[1] = a[1] * b[0] + a[5] * b[1] + a[9] * b[2] + a[13] * b[3];
	r[2] = a[2] * b[0] + a[6] * b[1] + a[10] * b[2] + a[14] * b[3];
	r[3] = a[3] * b[0] + a[7] * b[1] + a[11] * b[2] + a[15] * b[3];
	r[4] = a[0] * b[4] + a[4] * b[5] + a[8] * b[6] + a[12] * b[7];
	r[5] = a[1] * b[4] + a[5] * b[5] + a[9] * b[6] + a[13] * b[7];
	r[6] = a[2] * b[4] + a[6] * b[5] + a[10] * b[6] + a[14] * b[7];
	r[7] = a[3] * b[4] + a[7] * b[5] + a[11] * b[6] + a[15] * b[7];
	r[8] = a[0] * b[8] + a[4] * b[9] + a[8] * b[10] + a[12] * b[11];
	r[9] = a[1] * b[8] + a[5] * b[9] + a[9] * b[10] + a[13] * b[11];
	r[10] = a[2] * b[8] + a[6] * b[9] + a[10] * b[10] + a[14] * b[11];
	r[11] = a[3] * b[8] + a[7] * b[9] + a[11] * b[10] + a[15] * b[11];
	r[12] = a[0] * b[12] + a[4] * b[13] + a[8] * b[14] + a[12] * b[15];
	r[13] = a[1] * b[12] + a[5] * b[13] + a[9] * b[14] + a[13] * b[15];
	r[14] = a[2] * b[12] + a[6] * b[13] + a[10] * b[14] + a[14] * b[15];
	r[15] = a[3] * b[12] + a[7] * b[13] + a[11] * b[14] + a[15] * b[15];
	return r;
}
inline vec2 operator*(mat4 a, vec2 b) {
	vec2 r = {};
	r.x = a[0] * b.x + a[4] * b.y + a[12];
	r.y = a[1] * b.x + a[5] * b.y + a[13];
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

#endif
