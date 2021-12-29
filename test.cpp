#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdint.h>
#define GLFW_INCLUDE_GLEXT
#define GLFW_INCLUDE_GLCOREARB
#include <GLFW/glfw3.h>
#include "nanovg.h"
#define NANOVG_GL3_IMPLEMENTATION
#include "nanovg_gl.h"
#include "mathutil.h"
#include "distance.h"

#define CUTE_C2_IMPLEMENTATION
#include "cute_c2.h"

#define NOTUSED(v)  (void)sizeof(v)

struct View {
	View() {}
	View(Vec2 c, float s) : center(c), scale(s) {}
	Vec2 center;
	float scale;
};

View view(Vec2(0,0), 100.0f);

Vec2 xformPos(const View& view, Vec2 pos)
{
	return (pos - view.center) * view.scale;
}

Vec2 xformDir(const View& view, Vec2 dir)
{
	return dir * view.scale;
}

int sketchIdx = 0;
int colAType = 0;
int colBType = 0;

static void errorcb(int error, const char* desc)
{
	printf("GLFW error %d: %s\n", error, desc);
}

static void key(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	NOTUSED(scancode);
	NOTUSED(mods);
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GL_TRUE);
	if (key == GLFW_KEY_1 && action == GLFW_PRESS)
		colAType = (colAType + 1) % 3;
	if (key == GLFW_KEY_2 && action == GLFW_PRESS)
		colBType = (colBType + 1) % 3;
	if (key == GLFW_KEY_LEFT && action == GLFW_PRESS)
		sketchIdx--;
	if (key == GLFW_KEY_RIGHT && action == GLFW_PRESS)
		sketchIdx++;
}

void scroll(GLFWwindow* window, double xoffset, double yoffset)
{
	NOTUSED(window);
	NOTUSED(xoffset);
	if (yoffset > 0.01f) {
		view.scale *= 1.0f + yoffset * 0.01f;
	}
	if (yoffset < 0.01f) {
		view.scale /= 1.0f + -yoffset * 0.01f;
	}
}

void drawLine(NVGcontext* vg, const Vec2 p, const Vec2 q, NVGcolor color)
{
	nvgBeginPath(vg);

	nvgMoveTo(vg, p.x, p.y);
	nvgLineTo(vg, q.x, q.y);

	nvgStrokeColor(vg, color);
	nvgStroke(vg);
}

void drawCircle(NVGcontext* vg, const Vec2 p, const float rad, NVGcolor color)
{
	nvgBeginPath(vg);

	nvgCircle(vg, p.x, p.y, rad);

	nvgStrokeColor(vg, color);
	nvgStroke(vg);
}

void drawCircleFilled(NVGcontext* vg, const Vec2 p, const float rad, NVGcolor color)
{
	nvgBeginPath(vg);

	nvgCircle(vg, p.x, p.y, rad);

	nvgFillColor(vg, color);
	nvgFill(vg);
}

void drawRectFilled(NVGcontext* vg, const Vec2 p, const float w, const float h, NVGcolor color)
{
	nvgBeginPath(vg);

	nvgRect(vg, p.x, p.y, w, h);

	nvgFillColor(vg, color);
	nvgFill(vg);
}

void drawTick(NVGcontext* vg, const Vec2 p, const float s, NVGcolor color)
{
	nvgBeginPath(vg);

	nvgMoveTo(vg, p.x-s, p.y);
	nvgLineTo(vg, p.x+s, p.y);
	
	nvgMoveTo(vg, p.x, p.y-s);
	nvgLineTo(vg, p.x, p.y+s);

	nvgStrokeColor(vg, color);
	nvgStroke(vg);
}

void drawArrow(NVGcontext* vg, const Vec2 p, const Vec2 q, float w, NVGcolor color)
{
	Vec2 dir = norm(q - p);
	Vec2 side = left(dir);
	Vec2 left = q - dir*w - side * w * 0.3f;
	Vec2 right = q - dir*w + side * w * 0.3f;

	nvgBeginPath(vg);

	nvgMoveTo(vg, p.x, p.y);
	nvgLineTo(vg, q.x, q.y);

	nvgMoveTo(vg, left.x, left.y);
	nvgLineTo(vg, q.x, q.y);
	nvgLineTo(vg, right.x, right.y);

	nvgStrokeColor(vg, color);
	nvgStroke(vg);
}

void drawChevron(NVGcontext* vg, const Vec2 p, const Vec2 q, float w, NVGcolor color)
{
	Vec2 dir = norm(q - p);
	Vec2 side = left(dir);
	Vec2 left = p - side * w;
	Vec2 right = p + side * w;

	nvgBeginPath(vg);

	nvgMoveTo(vg, left.x, left.y);
	nvgLineTo(vg, q.x, q.y);
	nvgLineTo(vg, right.x, right.y);

	nvgFillColor(vg, color);
	nvgFill(vg);
}

static NVGcolor getColor(int i) 
{
	static const int numColors = 10;
	static const int colors[numColors * 3] = {
		255, 127, 14,
		214, 39, 40,
		148, 103, 189,
		44, 160, 44,
		31, 119, 180,
		227, 119, 194,
		188, 189, 34,
		140, 86, 75,
		127, 127, 127,
		23, 190, 207,
	};
	i = absi(i) % numColors;
	const int* c = &colors[i*3];
	return nvgRGBA(c[0], c[1], c[2], 255);
};


NVGcolor getPtColor(NVGcolor col, bool isDrag, bool isHit)
{
	if (isDrag)
		return nvgRGBA(255,192,0,255);
	else if (isHit)
		return nvgRGBA(255,255,255,255);
	return col;
}


void drawRoundedRect(NVGcontext* vg, const Vec2 pos, const Vec2 forw, const Vec2 size, const float rad, NVGcolor color)
{
	const float angle = atan2f(-forw.x, forw.y);

	nvgSave(vg);

	nvgBeginPath(vg);

	nvgTranslate(vg, pos.x, pos.y);
	nvgRotate(vg, angle);

	nvgRoundedRect(vg, -size.x - rad, -size.y - rad, (size.x + rad) * 2, (size.y + rad) * 2, rad);

	nvgStrokeColor(vg, color);
	nvgStroke(vg);

	nvgRestore(vg);
}

void drawChain(NVGcontext* vg, const Vec2 offset, const Vec2* chain, int numChain, const NVGcolor col)
{
	for (int i = 0; i < numChain-1; i++)
	{
		const Vec2 p = offset + chain[i];
		const Vec2 q = offset + chain[i+1];
		drawArrow(vg, p, q, 8, col);
	}
}

void drawChainSum(NVGcontext* vg, const Vec2 offset, const Vec2* chain, const uint8_t* chainIdx, int numChain,
					const NVGcolor col0, const NVGcolor col1)
{
	for (int i = 0; i < numChain-1; i++)
	{
		const Vec2 p = offset + chain[i];
		const Vec2 q = offset + chain[i+1];
		drawArrow(vg, p, q, 8, chainIdx[i+1] == 0 ? col0 : col1);
	}
}

void drawCollider(NVGcontext* vg, const Vec2 offset, const Collider& col, const NVGcolor color)
{
	drawRoundedRect(vg, offset + col.pos, col.up, col.ext, col.rad, color);
}

void drawSketch_ClosestPointOfApproach(NVGcontext* vg, const Vec2 agentPos, const Vec2 agentVel, const float agentRad,
										const Vec2 otherPos, const Vec2 otherVel, const float otherRad, float dt)
{
	char msg[64];
	nvgFontSize(vg, 12.0f);
	nvgFillColor(vg, nvgRGBA(255,255,255,128));

	const Vec2 chainOffsetW = xformDir(view, Vec2(0,2));

	const Vec2 agentPosW = xformPos(view, agentPos);
	const Vec2 otherPosW = xformPos(view, otherPos);

	const float agentRadW = view.scale * agentRad;
	const float otherRadW = view.scale * otherRad;

	Vec2 agentDir = norm(agentVel);
	Vec2 otherDir = norm(otherVel);

	Collider colA;
	Collider colB;

	if (colAType == 0)
		colA = Collider::MakeCircle(agentPosW, agentRadW);
	else if (colAType == 1)
		colA = Collider::MakePill(agentPosW, agentDir, agentRadW*2.0f, agentRadW);
	else
		colA = Collider::MakeRect(agentPosW, agentDir, agentRadW, agentRadW*2.0f, agentRadW);

	if (colBType == 0)
		colB = Collider::MakeCircle(otherPosW, otherRadW);
	else if (colBType == 1)
		colB = Collider::MakePill(otherPosW, otherDir, otherRadW*2.0f, otherRadW);
	else
		colB = Collider::MakeRect(otherPosW, otherDir, otherRadW, otherRadW*2.0f, otherRadW);

	const Vec2 velA = agentVel * view.scale;
	const Vec2 velB = otherVel * view.scale;

	ApproachRes cpa = closestPointOfApproach(colA, velA, colB, velB, 10.0f);
	DistanceRes res = nearestDistance(colA, velA * cpa.t, colB, velB * cpa.t);

	nvgStrokeWidth(vg,2.0);
	drawCollider(vg, Vec2(), colA, nvgRGBA(255,255,255,128));
	drawCollider(vg, Vec2(), colB, nvgRGBA(255,255,255,128));

	const Vec2 relVel = velA - velB;
	const Vec2 relPos = colA.pos - colB.pos;
	const float ts = dot(relPos, relVel) > 0.0f ? -1 : 1;
	const Vec2 testVel = relVel * ts;

	Vec2 chainA[3];
	Vec2 chainB[3];
	Vec2 sum[5];
	uint8_t sumColIdx[5]; 
	uint8_t sumSegIdx[5]; 

	const int numA = makeChain(colA, testVel, chainA);
	const int numB = makeChain(colB, testVel, chainB);
	const int numSum = minkowskiChain(chainA, numA, chainB, numB, sum, sumColIdx, sumSegIdx, 5);

	// Chains
	drawCollider(vg, chainOffsetW, colA, nvgRGBA(0,0,0,64));
	drawCollider(vg, chainOffsetW, colB, nvgRGBA(0,0,0,64));

	nvgStrokeWidth(vg,2.0);

	drawTick(vg, chainOffsetW + colA.pos, 8, nvgRGBA(0,128,255,196));
	drawTick(vg, chainOffsetW + colB.pos, 8, nvgRGBA(255,128,0,196));

	drawChain(vg, chainOffsetW + colA.pos, chainA, numA, nvgRGBA(0,128,255,128));
	drawChain(vg, chainOffsetW + colB.pos, chainB, numB, nvgRGBA(255,128,0,128));
	drawChainSum(vg, chainOffsetW + colB.pos, sum, sumColIdx, numSum, nvgRGBA(0,128,255,255), nvgRGBA(255,128,0,255));

	// Collided position
	drawCollider(vg, velA * cpa.t, colA, nvgRGBA(0,128,255,196));
	drawCollider(vg, velB * cpa.t, colB, nvgRGBA(255,128,0,196));

	// Circle cast result
	nvgStrokeWidth(vg,1.0);
	drawLine(vg, chainOffsetW + colA.pos, chainOffsetW + colA.pos + relVel * cpa.t, nvgRGBA(0,0,0,64));
	const Vec2 relHitPos =  chainOffsetW + colA.pos + relVel * cpa.t;
	drawCircleFilled(vg, relHitPos, colA.rad + colB.rad, nvgRGBA(0,0,0,32));
	drawLine(vg, relHitPos, relHitPos + -res.norm * (colA.rad + colB.rad), nvgRGBA(0,0,0,64));

	// Relative velocity
	nvgStrokeWidth(vg,2.0);
	drawArrow(vg, chainOffsetW + colA.pos, chainOffsetW + colA.pos + relVel, 10.0f, nvgRGBA(64,255,0,255));

	// Hit result
	Vec2 hitPos = colA.pos + velA * cpa.t;
	drawTick(vg, hitPos, 8, nvgRGBA(255,255,255,128));
	drawArrow(vg, hitPos, hitPos + res.norm * 30, 8, nvgRGBA(64,255,0,255));

	nvgFillColor(vg, cpa.hit ? nvgRGBA(255,128,64,196) : nvgRGBA(255,255,255,196));
	const Vec2 dpos = hitPos + res.norm * 40;
	snprintf(msg, 64, "D = %.1f T = %.1f", res.dist, cpa.t);
	nvgText(vg, dpos.x, dpos.y, msg, NULL);

	nvgStrokeWidth(vg,1.0);
}

void drawSketch_Distance(NVGcontext* vg, const Vec2 agentPos, const Vec2 agentVel, const float agentRad,
				const Vec2 otherPos, const Vec2 otherVel, const float otherRad, float dt)
{
	char msg[64];
	nvgFontSize(vg, 12.0f);

	const Vec2 chainOffsetW = xformDir(view, Vec2(0,2));

	const Vec2 agentPosW = xformPos(view, agentPos);
	const Vec2 otherPosW = xformPos(view, otherPos);

	const float agentRadW = view.scale * agentRad;
	const float otherRadW = view.scale * otherRad;

	Vec2 agentDir = norm(agentVel);
	Vec2 otherDir = norm(otherVel);

	Collider colA;
	Collider colB;

	if (colAType == 0)
		colA = Collider::MakeCircle(agentPosW, agentRadW);
	else if (colAType == 1)
		colA = Collider::MakePill(agentPosW, agentDir, agentRadW*2.0f, agentRadW);
	else
		colA = Collider::MakeRect(agentPosW, agentDir, agentRadW, agentRadW*2.0f, agentRadW);

	if (colBType == 0)
		colB = Collider::MakeCircle(otherPosW, otherRadW);
	else if (colBType == 1)
		colB = Collider::MakePill(otherPosW, otherDir, otherRadW*2.0f, otherRadW);
	else
		colB = Collider::MakeRect(otherPosW, otherDir, otherRadW, otherRadW*2.0f, otherRadW);

	DistanceRes res = nearestDistance(colA, Vec2(), colB, Vec2());

	nvgStrokeWidth(vg,2.0);
	drawCollider(vg, Vec2(), colA, nvgRGBA(255,255,255,128));
	drawCollider(vg, Vec2(), colB, nvgRGBA(255,255,255,128));

	const Vec2 relPos = colA.pos - colB.pos;

	Vec2 chainA[3];
	Vec2 chainB[3];
	Vec2 sum[5];
	uint8_t sumColIdx[5]; 
	uint8_t sumSegIdx[5]; 

	const int numA = makeChain(colA, -relPos, chainA);
	const int numB = makeChain(colB, -relPos, chainB);
	const int numSum = minkowskiChain(chainA, numA, chainB, numB, sum, sumColIdx, sumSegIdx, 5);

	nvgStrokeWidth(vg,2.0);

	// Chains
	drawCollider(vg, chainOffsetW, colA, nvgRGBA(0,0,0,64));
	drawCollider(vg, chainOffsetW, colB, nvgRGBA(0,0,0,64));

	nvgStrokeWidth(vg,2.0);

	drawTick(vg, chainOffsetW + colA.pos, 8, nvgRGBA(0,128,255,196));
	drawTick(vg, chainOffsetW + colB.pos, 8, nvgRGBA(255,128,0,196));

	drawChain(vg, chainOffsetW + colA.pos, chainA, numA, nvgRGBA(0,128,255,128));
	drawChain(vg, chainOffsetW + colB.pos, chainB, numB, nvgRGBA(255,128,0,128));
	drawChainSum(vg, chainOffsetW + colB.pos, sum, sumColIdx, numSum, nvgRGBA(0,128,255,255), nvgRGBA(255,128,0,255));

	// Test radius
	drawCircleFilled(vg, chainOffsetW + colA.pos, colA.rad + colB.rad, nvgRGBA(0,0,0,32));
	drawLine(vg, chainOffsetW + colA.pos, chainOffsetW + colA.pos + -res.norm * (colA.rad + colB.rad), nvgRGBA(0,0,0,64));

	// Result
	drawTick(vg, colB.pos, 8, nvgRGBA(255,255,255,128));
	drawArrow(vg, colB.pos, colB.pos + res.norm * 30, 8, nvgRGBA(64,255,0,255));

	nvgFillColor(vg, res.dist < 1e-6f ? nvgRGBA(255,128,64,196) : nvgRGBA(255,255,255,196));
	const Vec2 dpos = colB.pos + res.norm * 40;
	snprintf(msg, 64, "D = %.1f", res.dist);
	nvgText(vg, dpos.x, dpos.y, msg, NULL);

	nvgStrokeWidth(vg,1.0);
}

void steer(Collider& col, Vec2& vel, const float speed, const Vec2 tgt, const ApproachRes cpa, const DistanceRes nd, const float dt)
{
	Vec2 force;

	// steering.
	const float reactionTime = 0.5f;
	const float distToTarget = len(tgt - col.pos);
	const float speedScale = sqrf(clampf(distToTarget / 100.0f, 0.0f, 1.0f));
	const Vec2 dvel = norm(tgt - col.pos) * speed * speedScale;
	force += (dvel - vel) / reactionTime;

	// avoidance
	const float separationRad = 20.0f;
	const float avoid = 1.0f - minf(1.0f, nd.dist / separationRad);
	force += 100.0f * avoid * nd.norm;

	vel += force * dt;

	col.pos += vel * dt;
}


void drawSketch_Avoid(NVGcontext* vg, const Vec2 agentPos, const Vec2 agentVel, const float agentRad,
										const Vec2 otherPos, const Vec2 otherVel, const float otherRad, float dt)
{
	char msg[64];
	nvgFontSize(vg, 12.0f);
	nvgFillColor(vg, nvgRGBA(255,255,255,128));

	const Vec2 chainOffsetW = xformDir(view, Vec2(0,2));

	const Vec2 agentPosW = xformPos(view, agentPos);
	const Vec2 otherPosW = xformPos(view, otherPos);

	const float agentRadW = view.scale * agentRad;
	const float otherRadW = view.scale * otherRad;

	Vec2 agentDir = norm(agentVel);
	Vec2 otherDir = norm(otherVel);

	Collider colA;
	Collider colB;

	if (colAType == 0)
		colA = Collider::MakeCircle(agentPosW, agentRadW);
	else if (colAType == 1)
		colA = Collider::MakePill(agentPosW, agentDir, agentRadW*2.0f, agentRadW);
	else
		colA = Collider::MakeRect(agentPosW, agentDir, agentRadW, agentRadW*2.0f, agentRadW);

	if (colBType == 0)
		colB = Collider::MakeCircle(otherPosW, otherRadW);
	else if (colBType == 1)
		colB = Collider::MakePill(otherPosW, otherDir, otherRadW*2.0f, otherRadW);
	else
		colB = Collider::MakeRect(otherPosW, otherDir, otherRadW, otherRadW*2.0f, otherRadW);

	Vec2 velA = agentVel * view.scale;
	Vec2 velB = otherVel * view.scale;

	nvgStrokeWidth(vg,2.0);

	const float speedA = len(velA);
	const float speedB = len(velB);
	const Vec2 tgtA = colB.pos;
	const Vec2 tgtB = colA.pos;

	static float time = 0.0f;

	time += dt;

	const float ddt = 1.0f / 25.0f;
	const int frame = (int)(time / ddt) % 150;

	for (int i = 0; i < 150; i++)
	{
		if ((i % 5) == 0)
		{
			drawCollider(vg, Vec2(), colA, nvgRGBA(255,255,255,32));
			drawCollider(vg, Vec2(), colB, nvgRGBA(255,255,255,32));
		}

		ApproachRes cpaA = closestPointOfApproach(colA, velA, colB, velB, 2.5f);
		ApproachRes cpaB = closestPointOfApproach(colB, velB, colA, velA, 2.5f);
		DistanceRes ndA = nearestDistance(colA, velA * cpaA.t, colB, velB * cpaB.t);
		DistanceRes ndB = nearestDistance(colB, velB * cpaB.t, colA, velA * cpaA.t);

		steer(colA, velA, speedA, tgtA, cpaA, ndA, ddt);
		steer(colB, velB, speedB, tgtB, cpaB, ndB, ddt);

		if (i == frame)
		{
		drawArrow(vg, colA.pos, colA.pos + ndA.norm * 100.0f, 8, nvgRGBA(0,128,255,255));
		drawArrow(vg, colB.pos, colB.pos + ndB.norm * 100.0f, 8, nvgRGBA(255,128,0,255));

			drawCollider(vg, velA * cpaA.t, colA, nvgRGBA(0,0,0,64));
			drawCollider(vg, velB * cpaB.t, colB, nvgRGBA(0,0,0,64));

			drawCollider(vg, Vec2(), colA, nvgRGBA(0,128,255,255));
			drawCollider(vg, Vec2(), colB, nvgRGBA(255,128,0,255));
		}
	}

//	drawCollider(vg, Vec2(), colA, nvgRGBA(0,128,255,255));
//	drawCollider(vg, Vec2(), colB, nvgRGBA(255,128,0,255));

	nvgStrokeWidth(vg,1.0);
}


void drawSketch_CPARing(NVGcontext* vg, const Vec2 agentPos, const Vec2 agentVel, const float agentRad,
							const Vec2 otherPos, const Vec2 otherVel, const float otherRad, float dt)
{
	char msg[64];
	nvgFontSize(vg, 12.0f);
	nvgFillColor(vg, nvgRGBA(255,255,255,128));

	const Vec2 chainOffsetW = xformDir(view, Vec2(0,2));

	const Vec2 agentPosW = xformPos(view, agentPos);
	const Vec2 otherPosW = xformPos(view, otherPos);

	const float agentRadW = view.scale * agentRad;
	const float otherRadW = view.scale * otherRad;

	Vec2 agentDir = norm(agentVel);
	Vec2 otherDir = norm(otherVel);

	Collider colA;
	Collider colB;

	if (colAType == 0)
		colA = Collider::MakeCircle(agentPosW, agentRadW);
	else if (colAType == 1)
		colA = Collider::MakePill(agentPosW, agentDir, agentRadW*2.0f, agentRadW);
	else
		colA = Collider::MakeRect(agentPosW, agentDir, agentRadW, agentRadW*2.0f, agentRadW);

	if (colBType == 0)
		colB = Collider::MakeCircle(otherPosW, otherRadW);
	else if (colBType == 1)
		colB = Collider::MakePill(otherPosW, otherDir, otherRadW*2.0f, otherRadW);
	else
		colB = Collider::MakeRect(otherPosW, otherDir, otherRadW, otherRadW*2.0f, otherRadW);

	const Vec2 velA = agentVel * view.scale;
	const Vec2 velB = otherVel * view.scale;

	ApproachRes cpa = closestPointOfApproach(colA, velA, colB, velB, 3.0f);
	DistanceRes res = nearestDistance(colA, velA * cpa.t, colB, velB * cpa.t);

	const int numSamples = 400;
	Vec2 samplePos[numSamples];

	const float speedA = len(velA);

	for (int i = 0; i < numSamples; i++)
	{
		const float a = (float)i / (float)numSamples * M_PI * 2.0f;
		const Vec2 velA2(cosf(a) * speedA, sinf(a) * speedA);
		const ApproachRes cpa2 = closestPointOfApproach(colA, velA2, colB, velB, 3.0f);
		samplePos[i] = colA.pos + velA2 * cpa2.t;
	}

	nvgStrokeWidth(vg,2.0);

	nvgBeginPath(vg);
	nvgMoveTo(vg, samplePos[numSamples-1].x, samplePos[numSamples-1].y);
	for (int i = 0; i < numSamples; i++)
		nvgLineTo(vg, samplePos[i].x, samplePos[i].y);
	nvgStrokeColor(vg, nvgRGBA(0,0,0,128));
	nvgStroke(vg);

	nvgStrokeWidth(vg,2.0);
	drawCollider(vg, Vec2(), colA, nvgRGBA(255,255,255,128));
	drawCollider(vg, Vec2(), colB, nvgRGBA(255,255,255,128));

	// Collided position
	drawCollider(vg, velA * cpa.t, colA, nvgRGBA(0,128,255,196));
	drawCollider(vg, velB * cpa.t, colB, nvgRGBA(255,128,0,196));

	nvgStrokeWidth(vg,1.0);
}

void drawSketch_CPAField(NVGcontext* vg, const Vec2 agentPos, const Vec2 agentVel, const float agentRad,
							const Vec2 otherPos, const Vec2 otherVel, const float otherRad, float dt)
{
	char msg[64];
	nvgFontSize(vg, 12.0f);
	nvgFillColor(vg, nvgRGBA(255,255,255,128));

	const Vec2 chainOffsetW = xformDir(view, Vec2(0,2));

	const Vec2 agentPosW = xformPos(view, agentPos);
	const Vec2 otherPosW = xformPos(view, otherPos);

	const float agentRadW = view.scale * agentRad;
	const float otherRadW = view.scale * otherRad;

	Vec2 agentDir = norm(agentVel);
	Vec2 otherDir = norm(otherVel);

	Collider colA;
	Collider colB;

	if (colAType == 0)
		colA = Collider::MakeCircle(agentPosW, agentRadW);
	else if (colAType == 1)
		colA = Collider::MakePill(agentPosW, agentDir, agentRadW*2.0f, agentRadW);
	else
		colA = Collider::MakeRect(agentPosW, agentDir, agentRadW, agentRadW*2.0f, agentRadW);

	if (colBType == 0)
		colB = Collider::MakeCircle(otherPosW, otherRadW);
	else if (colBType == 1)
		colB = Collider::MakePill(otherPosW, otherDir, otherRadW*2.0f, otherRadW);
	else
		colB = Collider::MakeRect(otherPosW, otherDir, otherRadW, otherRadW*2.0f, otherRadW);

	const Vec2 velA = agentVel * view.scale;
	const Vec2 velB = otherVel * view.scale;

	ApproachRes cpa = closestPointOfApproach(colA, velA, colB, velB, 3.0f);
	DistanceRes res = nearestDistance(colA, velA * cpa.t, colB, velB * cpa.t);

	const int numSamples = 10;
	const float speedA = len(velA);

	const float s = (1.0f / (float)numSamples) * 4 * speedA;

	for (int i = -numSamples; i <= numSamples; i++)
	{
		const float y = velA.y*2 + i * s;
		for (int j = -numSamples; j <= numSamples; j++)
		{
			const float x = velA.x*2 + j * s;
			const Vec2 velA2(x, y);
			const ApproachRes cpa2 = closestPointOfApproach(colA, velA2, colB, velB, 3.0f);
			DistanceRes res2 = nearestDistance(colA, velA2 * cpa2.t, colB, velB * cpa2.t);

			const Vec2 pos = colA.pos + velA2;
			const float u = 1 - clampf(res2.dist / 150.0f, 0, 1); // (cpa2.t / 3.0f);

			drawRectFilled(vg, pos - Vec2(s/2, s/2), s, s, nvgRGBA(255,64,0,128*u));

			drawArrow(vg, pos, pos + res2.norm * 15, 5, nvgRGBA(0,0,0,128));
		}
	}

	nvgStrokeWidth(vg,2.0);

	nvgStrokeWidth(vg,2.0);
	drawCollider(vg, Vec2(), colA, nvgRGBA(255,255,255,128));
	drawCollider(vg, Vec2(), colB, nvgRGBA(255,255,255,128));

	// Collided position
	drawCollider(vg, velA * cpa.t, colA, nvgRGBA(0,128,255,196));
	drawCollider(vg, velB * cpa.t, colB, nvgRGBA(255,128,0,196));

	nvgStrokeWidth(vg,1.0);
}



typedef void (*SketchFunc)(NVGcontext* vg, const Vec2 agentPos, const Vec2 agentVel, const float agentRad,
				const Vec2 otherPos, const Vec2 otherVel, const float otherRad, float dt);

struct Sketch {
	SketchFunc sketch;
	const char* name;
};

static Sketch sketches[] = {
	{ drawSketch_ClosestPointOfApproach, "Closest Point of Approach" },	
	{ drawSketch_Distance, "Distance" },
	{ drawSketch_Avoid, "Avoid" },	
	{ drawSketch_CPARing, "CPA Ring" },	
	{ drawSketch_CPAField, "CPA Field" },	
};
static const int numSketches = sizeof(sketches) / sizeof(sketches[0]);


constexpr int RandMax = (1 << 23) - 1;
int rseed = 0;

inline int rand()
{
	return rseed = (rseed * 1103515245 + 12345) & RandMax;
}

inline float randf(const float rmin, const float rmax)
{
	rseed = (rseed * 1103515245 + 12345) & RandMax;
	return rmin + (rseed / (float)RandMax) * (rmax - rmin);
}


struct TestPair
{
	Collider colA;
	Collider colB;
	Vec2 velA;
	Vec2 velB;
};

Vec2 randomDir()
{
	float a = randf(-M_PI, M_PI);
	return Vec2(cosf(a), sinf(a));
}

Vec2 randomPos(const Vec2 pmin, const Vec2 pmax)
{
	return Vec2(randf(pmin.x, pmax.x), randf(pmin.y, pmax.y));
}


Collider randomCircleCollider()
{	
	const Vec2 pos = randomPos(Vec2(0, 0), Vec2(4, 4));
	return Collider::MakeCircle(pos, randf(0.1f, 1.0f));
}

Collider randomPillCollider()
{	
	const Vec2 pos = randomPos(Vec2(0, 0), Vec2(4, 4));
	const Vec2 dir = randomDir();
	return Collider::MakePill(pos, dir, randf(0.1f, 1.0f), randf(0.1f, 1.0f));
}

Collider randomRectCollider()
{	
	const Vec2 pos = randomPos(Vec2(0, 0), Vec2(4, 4));
	const Vec2 dir = randomDir();
	return Collider::MakeRect(pos, dir, randf(0.1f, 1.0f), randf(0.1f, 1.0f), randf(0.1f, 0.5f));
}

Collider randomCollider()
{
	const float type = randf(0,3);
	if (type < 1)
		return randomCircleCollider();
	else if (type < 2)
		return randomPillCollider();
	else
		return randomRectCollider();
}

void testPairsCPA(TestPair* pairs, const int numPairs)
{
	for (int i = 0; i < numPairs; i++)
	{
		const TestPair& p = pairs[i];
		Collider colA = p.colA;
		Collider colB = p.colB;
		ApproachRes cpa = closestPointOfApproach(colA, p.velA, colB, p.velB, 10.0f);
		DistanceRes dist = nearestDistance(colA, p.velA * cpa.t, colB, p.velB * cpa.t);
	}
}

struct c2Col
{
	union {
		c2Circle circle;
		c2AABB aabb;
		c2Capsule capsule;
	};
	C2_TYPE type;
};

c2Col colliderToCute(const Collider& col)
{
	c2Col c;
	if (col.type == ColliderType::Circle)
	{
		c.circle.p.x = 0;
		c.circle.p.y = 0;
		c.circle.r = col.rad;
		c.type = C2_TYPE_CIRCLE;
	}
	else if (col.type == ColliderType::Pill)
	{
		c.capsule.a.x = -col.up.x * col.ext.y;
		c.capsule.a.y = -col.up.y * col.ext.y;
		c.capsule.b.x = col.up.x * col.ext.y;
		c.capsule.b.y = col.up.y * col.ext.y;
		c.capsule.r = col.rad;
		c.type = C2_TYPE_CAPSULE;
	}
	else
	{
		c.aabb.min.x = -col.ext.x;
		c.aabb.min.y = -col.ext.y;
		c.aabb.max.x = col.ext.x;
		c.aabb.max.y = col.ext.y;
		c.type = C2_TYPE_AABB;
	}
	return c;
}

ApproachRes testCute(const Collider& colA, const Vec2 velA, const Collider& colB, const Vec2 velB)
{

	c2Col ca = colliderToCute(colA);
	c2x xa;
	xa.p.x = colA.pos.x;
	xa.p.y = colA.pos.y;
	xa.r.c = colA.up.x;
	xa.r.s = colA.up.y;
	c2v va;
	va.x = velA.x;
	va.y = velA.y;

	c2Col cb = colliderToCute(colB);
	c2x xb;
	xb.p.x = colB.pos.x;
	xb.p.y = colB.pos.y;
	xb.r.c = colB.up.x;
	xb.r.s = colB.up.y;
	c2v vb;
	vb.x = velB.x;
	vb.y = velB.y;

	ApproachRes res;
	res.t = c2TOI(&ca, ca.type, &xa, va, &cb, cb.type, &xb, vb, 1, NULL);

	xa.p.x += va.x * res.t;
	xa.p.y += va.y * res.t;

	xb.p.x += vb.x * res.t;
	xb.p.y += vb.y * res.t;

	c2v outA, outB;
	float d = c2GJK(&ca, ca.type, &xa, &cb, cb.type, &xb, &outA, &outB, 1, NULL, NULL);

	return res;
}


void testPairsCPACute(TestPair* pairs, const int numPairs)
{
	for (int i = 0; i < numPairs; i++)
	{
		const TestPair& p = pairs[i];
		Collider colA = p.colA;
		Collider colB = p.colB;
		ApproachRes res = testCute(colA, p.velA, colB, p.velB);
	}
}


void runTests()
{
	// Ballpark test against a GJK/CA

	const int numPairs = 1000;
	TestPair circlePairs[numPairs];
	TestPair circlePillPairs[numPairs];
	TestPair pillPairs[numPairs];
	TestPair rectPairs[numPairs];
	TestPair mixedPairs[numPairs];

	for (int i = 0; i < numPairs; i++)
	{
		TestPair& p = circlePairs[i];
		p.colA = randomCircleCollider();
		p.colB = randomCircleCollider();
		p.velA = randomDir() * randf(0.1f, 2.5f);
		p.velB = randomDir() * randf(0.1f, 2.5f);
	}

	for (int i = 0; i < numPairs; i++)
	{
		TestPair& p = circlePillPairs[i];
		if (rand() > RandMax/2) {
			p.colA = randomCircleCollider();
			p.colB = randomPillCollider();
		} else {
			p.colA = randomPillCollider();
			p.colB = randomCircleCollider();
		}
		p.velA = randomDir() * randf(0.1f, 2.5f);
		p.velB = randomDir() * randf(0.1f, 2.5f);
	}

	for (int i = 0; i < numPairs; i++)
	{
		TestPair& p = pillPairs[i];
		p.colA = randomPillCollider();
		p.colB = randomPillCollider();
		p.velA = randomDir() * randf(0.1f, 2.5f);
		p.velB = randomDir() * randf(0.1f, 2.5f);
	}

	for (int i = 0; i < numPairs; i++)
	{
		TestPair& p = rectPairs[i];
		p.colA = randomRectCollider();
		p.colB = randomRectCollider();
		p.velA = randomDir() * randf(0.1f, 2.5f);
		p.velB = randomDir() * randf(0.1f, 2.5f);
	}

	for (int i = 0; i < numPairs; i++)
	{
		TestPair& p = mixedPairs[i];
		p.colA = randomCollider();
		p.colB = randomCollider();
		p.velA = randomDir() * randf(0.1f, 2.5f);
		p.velB = randomDir() * randf(0.1f, 2.5f);
	}


	double t0, t1;
	printf("Circle-Circle\n");
	t0 = glfwGetTime();
	for (int i = 0; i < 10; i++)
		testPairsCPA(circlePairs, numPairs);
	t1 = glfwGetTime();
	printf(" - %.3f ms\n", (t1-t0) * 1000.0 / 10.0);

	t0 = glfwGetTime();
	for (int i = 0; i < 10; i++)
		testPairsCPACute(circlePairs, numPairs);
	t1 = glfwGetTime();
	printf(" - Cute: %.3f ms\n", (t1-t0) * 1000.0 / 10.0);


	printf("Circle-Pill\n");
	t0 = glfwGetTime();
	for (int i = 0; i < 10; i++)
		testPairsCPA(circlePillPairs, numPairs);
	t1 = glfwGetTime();
	printf(" - %.3f ms\n", (t1-t0) * 1000.0 / 10.0);

	t0 = glfwGetTime();
	for (int i = 0; i < 10; i++)
		testPairsCPACute(circlePillPairs, numPairs);
	t1 = glfwGetTime();
	printf(" - Cute: %.3f ms\n", (t1-t0) * 1000.0 / 10.0);


	printf("Pill-Pill\n");
	t0 = glfwGetTime();
	for (int i = 0; i < 10; i++)
		testPairsCPA(pillPairs, numPairs);
	t1 = glfwGetTime();
	printf(" - %.3f ms\n", (t1-t0) * 1000.0 / 10.0);

	t0 = glfwGetTime();
	for (int i = 0; i < 10; i++)
		testPairsCPACute(pillPairs, numPairs);
	t1 = glfwGetTime();
	printf(" - Cute: %.3f ms\n", (t1-t0) * 1000.0 / 10.0);


	printf("Rect-Rect\n");
	t0 = glfwGetTime();
	for (int i = 0; i < 10; i++)
		testPairsCPA(rectPairs, numPairs);
	t1 = glfwGetTime();
	printf(" - %.3f ms\n", (t1-t0) * 1000.0 / 10.0);

	t0 = glfwGetTime();
	for (int i = 0; i < 10; i++)
		testPairsCPACute(rectPairs, numPairs);
	t1 = glfwGetTime();
	printf(" - Cute: %.3f ms\n", (t1-t0) * 1000.0 / 10.0);


	printf("Mixed\n");
	t0 = glfwGetTime();
	for (int i = 0; i < 10; i++)
		testPairsCPA(mixedPairs, numPairs);
	t1 = glfwGetTime();
	printf(" - %.3f ms\n", (t1-t0) * 1000.0 / 10.0);

	t0 = glfwGetTime();
	for (int i = 0; i < 10; i++)
		testPairsCPACute(mixedPairs, numPairs);
	t1 = glfwGetTime();
	printf(" - Cute: %.3f ms\n", (t1-t0) * 1000.0 / 10.0);

}



int main(int argc, char** argv)
{
	NOTUSED(argc);
	NOTUSED(argv);

	if (!glfwInit()) {
		printf("Failed to init GLFW.");
		return -1;
	}

	runTests();
	return 0;


	glfwSetErrorCallback(errorcb);

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, 1);

	const GLFWvidmode*mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
	int width = mode->width - 40;
	int height = mode->height - 80;
    GLFWwindow*window = glfwCreateWindow(width, height, "crowd", NULL, NULL);
	if (!window) {
		glfwTerminate();
		return -1;
	}

	glfwSetKeyCallback(window, key);
	glfwSetScrollCallback(window, scroll);
	glfwMakeContextCurrent(window);

	NVGcontext* vg = nvgCreateGL3(NVG_ANTIALIAS | NVG_STENCIL_STROKES | NVG_DEBUG);
	if (vg == NULL) {
		printf("Could not init nanovg.\n");
		return -1;
	}

	int fontNormal = nvgCreateFont(vg, "sans", "../Karla-Regular.ttf");
	if (fontNormal == -1) {
		printf("Could not add font reguler.\n");
		glfwTerminate();
		return -1;
	}

	glfwSetTime(0);
	double prevt = glfwGetTime();

	Vec2 points[4] = {
		Vec2(-2, -1),
		Vec2( 1, 0),
		Vec2( 2, 0.1f-1),
		Vec2( -1, 0),
	};
	const int numPoints = 4;
	int prevMouseBut = 0;
	int drag = -1;
	Vec2 dragStartPt;
	Vec2 dragStartMousePos;

	float tickTimeAcc = 0.0f;

	while (!glfwWindowShouldClose(window))
	{
		int winWidth, winHeight;
		glfwGetWindowSize(window, &winWidth, &winHeight);

		int fbWidth, fbHeight;
		glfwGetFramebufferSize(window, &fbWidth, &fbHeight);

		double mx, my;
		glfwGetCursorPos(window, &mx, &my);
		Vec2 mousePos(mx - winWidth/2, my - winHeight/2);
		const Vec2 localMouse = (mousePos / view.scale) + view.center;

		int hit = -1;
		if (drag == -1) {
			for (int i = 0; i < numPoints; i += 2) {
				Vec2 pt = points[i];
				if (dist(pt, localMouse) < 0.25f) {
					hit = i;
				}
				pt = points[i] + points[i+1];
				if (dist(pt, localMouse) < 0.1f) {
					hit = i + 1;
				}
			}
		}

		int mouseBut = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT);
		if (prevMouseBut == GLFW_RELEASE && mouseBut == GLFW_PRESS) {
			// mouse pressed
			dragStartMousePos = mousePos;
			if (hit == -1) {
				drag = -2;
				dragStartPt = view.center;
			} else {
				drag = hit;
				dragStartPt = points[hit];
			}
		}
		if (prevMouseBut == GLFW_PRESS && mouseBut == GLFW_RELEASE) {
			drag = -1;
		}
		if (mouseBut == GLFW_PRESS) {
			const Vec2 delta = (dragStartMousePos - mousePos) / view.scale;
			if (drag == -2) {
				view.center = dragStartPt + delta;
			} else {
				points[drag] = dragStartPt - delta;
			}
		}

		prevMouseBut = mouseBut;

		// Calculate pixel ration for hi-dpi devices.
		float pxRatio = (float)fbWidth / (float)winWidth;

		double ct = glfwGetTime();
		float dt = (float)(ct - prevt);
		prevt = ct;
	
		// Update and render
		glViewport(0, 0, fbWidth, fbHeight);
		glClearColor(0.3f, 0.3f, 0.32f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

		nvgBeginFrame(vg, winWidth, winHeight, pxRatio);

		nvgFontSize(vg, 17.0f);
		nvgFontFace(vg, "sans");
		nvgFillColor(vg, nvgRGBA(255,255,255,255));

/*			if (reset) {
			reset = 0;
			points[0] = Vec2(-2, 0);
			points[1] = Vec2( 1, 0);
			points[2] = Vec2( 2, 0.1f);
			points[3] = Vec2( -1, 0);
		}*/

		if (sketchIdx < 0 || sketchIdx >= numSketches)
			sketchIdx = 0;

		float ty = 100;
		nvgTextAlign(vg,NVG_ALIGN_LEFT|NVG_ALIGN_MIDDLE);
		nvgText(vg, 100, ty, sketches[sketchIdx].name, NULL);
		ty += 20;

		nvgTranslate(vg, winWidth/2, winHeight/2);

		const float rad[2] = { 0.2f, 0.3f };
		const float radW[2] = { rad[0] * view.scale, rad[1] * view.scale };

		for (int i = 0; i < 4; i += 2) {
			const Vec2 pos = xformPos(view, points[i]);
			const Vec2 vel = xformDir(view, points[i+1]);

			nvgBeginPath(vg);
			nvgCircle(vg, pos.x, pos.y, 5);
			nvgFillColor(vg, getPtColor(nvgRGBA(128,128,128,255), i == drag, i == hit));
			nvgFill(vg);

			nvgStrokeWidth(vg, 2);
			drawArrow(vg, pos, pos + vel, radW[i/2] * 0.3f, getPtColor(nvgRGBA(128,128,128,255), (i+1) == drag, (i+1) == hit));
			nvgStrokeWidth(vg, 1);
		}

		sketches[sketchIdx].sketch(vg, points[0], points[1], rad[0], points[2], points[3], rad[1], dt);

		nvgEndFrame(vg);

		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	nvgDeleteGL3(vg);

	glfwTerminate();

	return 0;
}
