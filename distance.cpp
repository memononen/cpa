#include "distance.h"
#include "mathutil.h"
#include <stdio.h>


bool circleCircleCPA(const Vec2 pos, const Vec2 vel, const float rad, const Vec2 center, float& t)
{
	const Vec2 relPos = pos - center;
    const float a = dot(vel, vel);
	const float b = dot(vel, relPos);
	const float c = dot(relPos, relPos) - sqrf(rad);
	const float h = maxf(0.0f, b*b - a * c);
	t = (-b - sqrtf(maxf(0.0f, h))) / a;
	return h > 0.0f;
}

bool circleSegmentBodyTOI(const Vec2 pos, const Vec2 vel, const float rad, const Vec2 segStart, const Vec2 segEnd, float& t)
{
    const Vec2 segDir = segEnd - segStart;
    const Vec2 relPos = pos - segStart;
    const float velSq = dot(vel,vel);
    const float segDirSq = dot(segDir, segDir);
    const float dirVelSq = dot(segDir, vel);
    const float dirRelPosSq = dot(segDir, relPos);
    const float velRelPosSq = dot(vel, relPos);
    const float relPosSq = dot(relPos, relPos);
    const float a = segDirSq*velSq - dirVelSq*dirVelSq;
    const float b = segDirSq*velRelPosSq - dirRelPosSq*dirVelSq;
    const float c = segDirSq*relPosSq - dirRelPosSq*dirRelPosSq - sqrf(rad)*segDirSq;
    const float h = maxf(0.0f, b*b - a*c);
	if (h >= 0 && fabsf(a) > 1e-6f)
    {
        const float t0 = (-b - sqrtf(h)) / a;
        const float y = dirRelPosSq + t0 * dirVelSq;
        if (y > 0.0 && y < segDirSq)
        {
            t = t0;
            return true;
        }
    }

    return false;
}

bool circleSegmentCPA(const Vec2 pos, const Vec2 vel, const float rad,
                        const Vec2 segStart, const Vec2 segEnd, float& t)
{
    const Vec2 segDir = segEnd - segStart;
    const Vec2 relPos = pos - segStart;
    const float velSq = dot(vel,vel);
    const float segDirSq = dot(segDir, segDir);
    const float dirVelSq = dot(segDir, vel);
    const float dirRelPosSq = dot(segDir, relPos);
    const float velRelPosSq = dot(vel, relPos);
    const float relPosSq = dot(relPos, relPos);
    const float a = segDirSq*velSq - dirVelSq*dirVelSq;
    const float b = segDirSq*velRelPosSq - dirRelPosSq*dirVelSq;
    const float c = segDirSq*relPosSq - dirRelPosSq*dirRelPosSq - sqrf(rad)*segDirSq;
    const float h = maxf(0.0f, b*b - a*c);
    const float inva = fabsf(a) > 1e-6f ? 1.0f / a : 0.0f;
    const float t0 = (-b - sqrtf(h)) * inva;
    const float y = dirRelPosSq + t0 * dirVelSq;

    // body
    if (y > 0.0 && y < segDirSq)
    {
        t = t0;
        return true;
    }
    
    // caps
    const Vec2 capRelPos = (y <= 0.0) ? relPos : pos - segEnd;
    const float cb = dot(vel, capRelPos);
    const float cc = dot(capRelPos, capRelPos) - sqrf(rad);
    const float ch = cb*cb - velSq * cc;
	const float t1 = (-cb - sqrtf(maxf(0.0f, ch))) / velSq;
    t = t1;

    return ch > 0.0f;
}

float projectPtSeg(const Vec2 pt, const Vec2 start, const Vec2 end)
{
    const Vec2 seg = end - start;
    const Vec2 dir = pt - start;
    const float d = lenSq(seg);
    const float t = dot(seg, dir);
    if (t < 0.0f || d < 1e-6f) return 0;
    if (t > d) return 1;
    return d > 0.0f ? (t / d) : 0.0f;
}

int makeChain(const Collider& col, const Vec2 dir, Vec2 chain[3])
{
    const Vec2 offset(0,0);
    int n = 0;

    if (col.type == ColliderType::Circle)
    {
        chain[n++] = offset;
    }
    else if (col.type == ColliderType::Pill)
    {
        // orint pill spine
        const float cy = signf(perp(col.up, -dir));
        const Vec2 dy = col.up * col.ext.y;

        chain[n++] = offset + dy*cy;
        chain[n++] = offset - dy*cy;
    }
    else if (col.type == ColliderType::Rect)
    {
        // find corner that is pointing most towards the direction.
        const float cx = signf(perp(col.up, -dir));
        const float cy = signf(dot(col.up, -dir));
        const Vec2 dx = left(col.up) * col.ext.x;
        const Vec2 dy = col.up * col.ext.y;

        // 2 Segments around the corner 
        chain[n++] = offset + dx*-cy + dy*cx;
        chain[n++] = offset + dx*cx + dy*cy; // corner
        chain[n++] = offset + dx*cy + dy*-cx;
    }

	return n;
}

int minkowskiChain(const Vec2* chainA, const int numA, const Vec2* chainB, const int numB,
					Vec2* res, uint8_t* resColIdx, uint8_t* resSegIdx, const int maxRes)
{
	int ia = 0;
	int ib = 0;
	int n = 0;

	// Add initial point
	res[n] = chainA[ia] + chainB[ib];
	resColIdx[n] = 0;
	resSegIdx[n] = 0;
	n++;

	while (n < maxRes)
	{
		const int ian = ia + 1;
		const int ibn = ib + 1;

		if (ian >= numA && ibn >= numB)
		{
			// both sides have been conumSumed, stop.
			break;
		}
		else if (ian >= numA)
		{
			// A is empty, keep on adding B.
			res[n] = res[n-1] + (chainB[ibn] - chainB[ib]);
			resColIdx[n] = 1;
			resSegIdx[n] = ibn;
			ib = ibn;
		}
		else if (ibn >= numB)
		{
			// B is empty, keep on adding A.
			res[n] = res[n-1] + (chainA[ian] - chainA[ia]);
			resColIdx[n] = 0;
			resSegIdx[n] = ian;
			ia = ian;
		}
		else 
		{
			// A and B are valid, add based on facing.
			const float c = perp(chainA[ian] - chainA[ia], chainB[ibn] - chainB[ib]);

			if (c >= 0.0f)
			{
				res[n] = res[n-1] + (chainA[ian] - chainA[ia]);
                resColIdx[n] = 0;
                resSegIdx[n] = ian;
				ia = ian;
			}
			else
			{
				res[n] = res[n-1] + (chainB[ibn] - chainB[ib]);
                resColIdx[n] = 1;
                resSegIdx[n] = ibn;
				ib = ibn;
			}
		}
		n++;
	}

	return n;
}


DistanceRes nearestDistance(const Collider& colA, const Vec2 offsetA, const Collider& colB, const Vec2 offsetB)
{
	const Vec2 relPos = (colA.pos + offsetA) - (colB.pos + offsetB);
	const float totalRad = colA.rad + colB.rad;

    DistanceRes res;

    // Handle trivial cases early.
    if (colA.type == ColliderType::Circle && colB.type == ColliderType::Circle)
    {
        const float dist = len(relPos);
        res.dist = dist - totalRad;
        res.norm = dist > 1e-6f ? (relPos / dist) : Vec2(1,0);

        return res;
    }
    else if ((colA.type == ColliderType::Circle && colB.type == ColliderType::Pill) ||
             (colA.type == ColliderType::Pill && colB.type == ColliderType::Circle))
    {
        const Vec2 up = colA.type == ColliderType::Pill ? colA.up : colB.up;
        const float hh = colA.type == ColliderType::Pill ? colA.ext.y : colB.ext.y;

        const float s = clampf(dot(up, relPos), -hh, hh);
        const Vec2 sp = up * s;
        const Vec2 diff = relPos - sp;
        const float dist = len(diff);

        res.dist = dist - totalRad;
        res.norm = dist > 1e-6f ? (diff / dist) : Vec2(1,0);

        return res;
    }

	Vec2 chainA[3];
	Vec2 chainB[3];
	Vec2 sum[5];
	uint8_t sumColIdx[5]; 
	uint8_t sumSegIdx[5]; 

	const int numA = makeChain(colA, -relPos, chainA);
	const int numB = makeChain(colB, -relPos, chainB);
	const int numSum = minkowskiChain(chainA, numA, chainB, numB, sum, sumColIdx, sumSegIdx, 5);

	Vec2 nearestNorm;
	float nearestDist = 1e6f;
    int nearestPt = -1;

    // Test internal vertices.
    // Since the chains are always oriented towards the direction between the bodies,
    // the first and last point does not contribute.
    for (int i = 0; i < numSum; i++)
    {
        const Vec2 sp = sum[i];
        const Vec2 diff = relPos - sp;
        const float dist = lenSq(diff);
        if (dist < nearestDist)
        {
            nearestDist = dist;
            nearestNorm = diff;
            nearestPt = i;
        }
    }

    // Test the 2 segment bodies around the nearest point.
    for (int i = maxi(0, nearestPt-1); i < mini(nearestPt+1, numSum); i++)
    {
        const Vec2 p = sum[i];
        const Vec2 q = sum[i+1];
        const float s = projectPtSeg(relPos, p, q);
        if (s > 0.0f && s < 1.0f)
        {
            const Vec2 segPos = lerp(p, q, s);
            const Vec2 diff = relPos - segPos;
            const float dist = lenSq(diff);
            if (dist < nearestDist)
            {
                const float sign = perp(q - p, diff) >= 0.0f ? -1 : 1;
                nearestDist = dist;
                nearestNorm = diff * sign;
            }
        }
    }

    nearestDist = sqrtf(nearestDist);
    res.norm = nearestDist > 1e-6f ? (nearestNorm / nearestDist) : Vec2(1,0);
    res.dist = nearestDist - totalRad;

    return res;
}

ApproachRes closestPointOfApproach(const Collider& colA, const Vec2 velA, const Collider& colB, const Vec2 velB, const float maxTime)
{
	Vec2 relVel = velA - velB;
	Vec2 relPos = colA.pos - colB.pos;
	const float totalRad = colA.rad + colB.rad;

    ApproachRes res;

    // Handle trivial cases early.
    if (colA.type == ColliderType::Circle && colB.type == ColliderType::Circle)
    {
        res.hit = circleCircleCPA(relPos, relVel, totalRad, Vec2(0,0), res.t);
        res.t = clampf(res.t, 0.0f, maxTime);
        return res;
    }
    else if ((colA.type == ColliderType::Circle && colB.type == ColliderType::Pill) ||
             (colA.type == ColliderType::Pill && colB.type == ColliderType::Circle))
    {
        const Vec2 stem = colA.type == ColliderType::Pill ? (colA.up * colA.ext.y) : (colB.up * colB.ext.y);
        res.hit = circleSegmentCPA(relPos, relVel, totalRad, -stem, stem, res.t);
        res.t = clampf(res.t, 0.0f, maxTime);
        return res;
    }

	Vec2 chainA[3];
	Vec2 chainB[3];
	Vec2 sum[5];
	uint8_t sumColIdx[5];
	uint8_t sumSegIdx[5];

	const int numA = makeChain(colA, relVel, chainA);
	const int numB = makeChain(colB, relVel, chainB);
	const int numSum = minkowskiChain(chainA, numA, chainB, numB, sum, sumColIdx, sumSegIdx, 5);

    // check if the ray can hit the sum at all.
    const Vec2 testDir = norm(relVel);
    const float firstDist = perp(testDir, sum[0] - relPos) + totalRad;
    const float lastDist = perp(testDir, sum[numSum-1] - relPos) - totalRad;

    if ((firstDist * lastDist) > 0.0f)
    {
        // Not hit, return closes point of approach.
		const Vec2 p = fabsf(firstDist) < fabsf(lastDist) ? sum[0] : sum[numSum-1];

		float t;
		circleCircleCPA(relPos, relVel, totalRad, p, t);
        res.t = clampf(t, 0.0f, maxTime);
        res.hit = false;

        return res;
    }

    res.t = 1e6f;
    res.hit = false;

    // Hit test segments
    for (int i = 0; i < numSum-1; i++)
    {
        const Vec2 p = sum[i];
        const Vec2 q = sum[i+1];

        float t;
        if (circleSegmentBodyTOI(relPos, relVel, totalRad, p, q, t))
        {
            // Segments cannot overlap, we can early out as soon as we find a hit.
            res.t = t;
            res.hit = true;
            break;
        }
    }

    // Hit test caps.
    if (!res.hit)
    {
        for (int i = 0; i < numSum; i++)
        {
            const Vec2 p = sum[i];

            float t;
            if (circleCircleCPA(relPos, relVel, totalRad, p, t))
            {
                // There can be multiple hits for circles, pick the nearest hit.
                if (t < res.t)
                {
                    res.t = t;
                    res.hit = true;
                }
            }
        }
    }

    res.t = clampf(res.t, 0.0f, maxTime);

    return res;
}