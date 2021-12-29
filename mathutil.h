//
// Copyright (c) 2021 Mikko Mononen memon@inside.org
//
// This software is provided 'as-is', without any express or implied
// warranty.  In no event will the authors be held liable for any damages
// arising from the use of this software.
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely, subject to the following restrictions:
// 1. The origin of this software must not be misrepresented; you must not
//    claim that you wrote the original software. If you use this software
//    in a product, an acknowledgment in the product documentation would be
//    appreciated but is not required.
// 2. Altered source versions must be plainly marked as such, and must not be
//    misrepresented as being the original software.
// 3. This notice may not be removed or altered from any source distribution.
//

#include <math.h>

#ifndef MATHUTIL_H
#define MATHUTIL_H

static inline int mini(int a, int b) {
	return a < b ? a : b;
}

static inline int maxi(int a, int b) {
	return a < b ? b : a;
}

static inline int absi(int a) {
	return a < 0 ? -a : a;
}

static inline float minf(float a, float b) {
	return a < b ? a : b;
}

static inline float maxf(float a, float b) {
	return a < b ? b : a;
}

static inline float clampf(float x, float bmin, float bmax) {
	return minf(maxf(x, bmin), bmax);
}

static inline int iszero(float x) {
	return fabsf(x) < 1e-6f;
}

static inline float sqrf(float x) {
	return x*x;
}

static inline float lerpf(float a, float b, float t) {
	return a + (b-a)*t;
}

static inline float pow2f(const float x) {
	return x * x;
}

static inline float pow3f(const float x) {
	return x * x * x;
}

static inline float pow4f(const float x) {
	return x * x * x * x;
}

static inline float pow5f(const float x) {
	return x * x * x * x * x;
}

static inline float pow6f(const float x) {
	return x * x * x * x * x * x;
}

static inline float smoothf(float x) {
	return x * x * (3 - 2 * x);
}

static inline float smootherf(float x) {
	return x * x * x * (x * (x * 6 - 15) + 10);
}

static inline float signf(float x) {
	return x < 0.0f ? -1.0f : 1.0f;
}

static inline float safeinv(float x) {
    return fabsf(x) > 1e-6f ? (1.0f / x) : 0.0f;
}

struct Vec2 {
	Vec2() : x(0), y(0) {};
	Vec2(float _x, float _y) : x(_x), y(_y) {}
    float x = 0.0f;
	float y = 0.0f;

	Vec2& operator+=(const Vec2& b) {
		x += b.x;
		y += b.y;
		return *this;
	} 

	Vec2& operator-=(const Vec2& b) {
		x -= b.x;
		y -= b.y;
		return *this;
	} 

	Vec2& operator*=(const float b) {
		x *= b;
		y *= b;
		return *this;
	} 

	Vec2& operator/=(const float b) {
		x /= b;
		y /= b;
		return *this;
	} 
};

inline Vec2 operator+(const Vec2 a, const Vec2 b) {
	return Vec2(a.x + b.x, a.y + b.y);
}

inline Vec2 operator-(const Vec2 a, const Vec2 b) {
	return Vec2(a.x - b.x, a.y - b.y);
}

inline Vec2 operator-(const Vec2 a) {
	return Vec2(-a.x, -a.y);
}

inline Vec2 operator*(const Vec2 a, const float b) {
	return Vec2(a.x * b, a.y * b);
}

inline Vec2 operator*(const float a, const Vec2 b) {
	return Vec2(a * b.x, a * b.y);
}

inline Vec2 operator/(const Vec2 a, const float b) {
	return Vec2(a.x / b, a.y / b);
}

 inline float len(const Vec2 a) {
	return sqrtf(a.x*a.x + a.y*a.y);
}

 inline float lenSq(const Vec2 a) {
	return a.x*a.x + a.y*a.y;
}

inline float dist(const Vec2 a, const Vec2 b) {
	float dx = a.x - b.x;
	float dy = a.y - b.y;
	return sqrtf(dx*dx + dy*dy);
}

inline float distSq(const Vec2 a, const Vec2 b) {
	float dx = a.x - b.x;
	float dy = a.y - b.y;
	return dx*dx + dy*dy;
}

inline Vec2 norm(const Vec2 a) {
    float s = sqrtf(a.x*a.x + a.y*a.y);
    s = (s > 1e-6f) ? 1.0f / s : 0.0f;
	return Vec2(a.x * s, a.y * s);
}

inline float dot(const Vec2 a, const Vec2 b) {
    return a.x*b.x + a.y*b.y;
}

inline float perp(const Vec2 a, const Vec2 b) {
    return a.y*b.x - a.x*b.y;
}

inline Vec2 left(const Vec2 a) {
	return Vec2(a.y, -a.x);
}

inline Vec2 lerp(const Vec2 a, const Vec2 b, const float t) {
	return Vec2(a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t);
}

inline Vec2 clamp(const Vec2 a, const float mag) {
	const float len = lenSq(a);
	if (len > sqrf(mag)) {
		return a * mag / sqrtf(len);
	}
	return a;
}

Vec2 slerp(const Vec2 a, const Vec2 b, const float t);

float projectPtSegSq(const Vec2 pt, const Vec2 start, const Vec2 end);


float randf();
float symrandf();

#endif // MATHUTIL_H
