#include <math.h>
#include <stdlib.h>
#include "mathutil.h"

float randf()
{
    return (float)rand() / (float)(RAND_MAX);
}

float symrandf()
{
    return randf() * 2.0f - 1.0f;
}

Vec2 slerp(const Vec2 a, const Vec2 b, const float t)
{
	const float k = acosf(clampf(dot(a, b), -1, 1));
	const float t0 = sinf(k * (1 - t));
	const float t1 = sinf(k * t);
	return norm(a * t0 + b * t1);
}

float projectPtSegSq(const Vec2 pt, const Vec2 start, const Vec2 end)
{
    const Vec2 seg = end - start;
    const Vec2 dir = pt - start;
    const float d = lenSq(seg);
    const float t = dot(seg, dir);
    if (t < 0.0f) return 0;
    if (t > d) return 1;
    return d > 0.0f ? (t / d) : 0.0f;
}
