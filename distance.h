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

#include "mathutil.h"
#include <stdint.h>

enum class ColliderType : uint8_t
{
    Circle,
    Pill,
    Rect,
};

struct Collider
{

    static Collider MakeCircle(const Vec2 p, const float r)
    {
        Collider col;
        col.pos = p;
        col.up = Vec2(0,1);
        col.ext = Vec2();
        col.rad = r;
        col.type = ColliderType::Circle;
        return col;
    }

    static Collider MakePill(const Vec2 p, const Vec2 up, const float hh, const float r)
    {
        Collider col;
        col.pos = p;
        col.up = up;
        col.ext = Vec2(0,hh);
        col.rad = r;
        col.type = ColliderType::Pill;
        return col;
    }

    static Collider MakeRect(const Vec2 p, const Vec2 up, const float hw, const float hh, const float r = 0.0f)
    {
        Collider col;
        col.pos = p;
        col.up = up;
        col.ext = Vec2(hw,hh);
        col.rad = r;
        col.type = ColliderType::Rect;
        return col;
    }

    Vec2 pos;
    Vec2 up;
    Vec2 ext;
    float rad;
    ColliderType type = ColliderType::Circle;
};

bool circleSegmentBodyCPA(const Vec2 pos, const Vec2 vel, const float rad,
							const Vec2 segStart, const Vec2 segEnd, float& s, float& t);

bool circleCircleCPA(const Vec2 pos, const Vec2 vel, const float rad, const Vec2 center, float& t);

bool circleSegmentCPA(const Vec2 pos, const Vec2 vel, const float rad,
                        const Vec2 segStart, const Vec2 segEnd, float& s, float& t);

float projectPtSeg(const Vec2 pt, const Vec2 start, const Vec2 end);

int makeChain(const Collider& col, const Vec2 dir, Vec2 chain[3]);

int minkowskiChain(const Vec2* chainA, const int numA, const Vec2* chainB, const int numB,
					Vec2* res, uint8_t* resColIdx, uint8_t* resSegIdx, const int maxRes);


struct DistanceRes
{
    Vec2 norm;
    float dist = 0.0f;
};

DistanceRes nearestDistance(const Collider& colA, const Vec2 offsetA, const Collider& colB, const Vec2 offsetB);

struct ApproachRes
{
    float t = 0.0f;
    bool hit = false;
};

ApproachRes closestPointOfApproach(const Collider& colA, const Vec2 velA, const Collider& colB, const Vec2 velB, const float maxTime);