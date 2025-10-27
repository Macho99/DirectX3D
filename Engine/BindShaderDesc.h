#pragma once
#include "ConstantBuffer.h"

struct GlobalDesc
{
	Matrix V = Matrix::Identity;
	Matrix P = Matrix::Identity;
	Matrix VP = Matrix::Identity;
	Matrix VPT = Matrix::Identity;
	Matrix VInv = Matrix::Identity;
	Vec3 CamPos = Vec3(0, 0, 0);
	float padding;
};

struct TransformDesc
{
	Matrix W = Matrix::Identity;
};

// Light
struct LightDesc
{
	Color ambient = Color(1.f, 1.f, 1.f, 1.f);
	Color diffuse = Color(1.f, 1.f, 1.f, 1.f);
	Color specular = Color(1.f, 1.f, 1.f, 1.f);
	Color emissive = Color(1.f, 1.f, 1.f, 1.f);

	Vec3 direction;
	float padding0;
};

struct MaterialDesc
{
	Color ambient = Color(0.f, 0.f, 0.f, 1.f);
	Color diffuse = Color(1.f, 1.f, 1.f, 1.f);
	Color specular = Color(0.f, 0.f, 0.f, 1.f);
	Color emissive = Color(0.f, 0.f, 0.f, 1.f);
};

// Bone
#define MAX_MODEL_TRANSFORMS 250
#define MAX_MODEL_KEYFRAMES 500
#define MAX_MODEL_INSTANCE 500

struct BoneDesc
{
	Matrix transforms[MAX_MODEL_TRANSFORMS];
};

// Animation
struct KeyframeDesc
{
	int32 animIndex = 0;
	uint32 curFrame = 0;

	// TODO
	uint32 nextFrame = 0;
	float ratio = 0.f;
	float sumTime = 0.f;
	float speed = 1.f;
	Vec2 padding;
};

struct TweenDesc
{
	TweenDesc()
	{
		cur.animIndex = 0;
		next.animIndex = -1;
	}

	void ClearNextAnim()
	{
		next.animIndex = -1;
		next.curFrame = 0;
		next.nextFrame = 0;
		next.sumTime = 0;
		tweenSumTime = 0.f;
		tweenRatio = 0.f;
	}

	float tweenDuration = 1.0f;
	float tweenRatio = 0.f;
	float tweenSumTime = 0.f;
	float padding = 0.f;
	KeyframeDesc cur;
	KeyframeDesc next;
};

struct InstancedTweenDesc
{
	TweenDesc tweens[MAX_MODEL_INSTANCE];
};

struct SnowBillboardDesc
{
	Color color = Color(1, 1, 1, 1);

	Vec3 velocity = Vec3(0, -5, 0);
	float drawDistance = 0;

	Vec3 origin = Vec3(0, 0, 0);
	float turbulence = 5;

	Vec3 extent = Vec3(0, 0, 0);
	float time = 0;
};

struct ParticleDesc
{
	Vec3 emitPosW = Vec3(0, 0, 0);
	float timeStep = 0.f;
	Vec3 emitDirW = Vec3(0, 0, 0);
	float gameTime = 0.f;
};

struct SsaoDesc
{
	Matrix viewToTexSpace;
	Vec4 offsetVectors[14];
	Vec4 frustumCorners[4];

	// Coordinates given in view space.
	float gOcclusionRadius = 0.5f;
	float gOcclusionFadeStart = 0.2f;
	float gOcclusionFadeEnd = 2.0f;
	float gSurfaceEpsilon = 0.05f;
};

struct BlurDesc
{
	float gTexelWidth;
	float gTexelHeight;
	Vec2 dummy;
};

struct TerrainDesc
{
	//DirectionalLight gDirLights[3];
	//float3 gEyePosW;
	
	//float  gFogStart;
	//float  gFogRange;
	//float4 gFogColor;

	// When distance is minimum, the tessellation is maximum.
	// When distance is maximum, the tessellation is minimum.
	float gMinDist;
	float gMaxDist;

	// Exponents for power of 2 tessellation.  The tessellation
	// range is [2^(gMinTess), 2^(gMaxTess)].  Since the maximum
	// tessellation is 64, this means gMaxTess can be at most 6
	// since 2^6 = 64.
	float gMinTess;
	float gMaxTess;

	float gTexelCellSpaceU;
	float gTexelCellSpaceV;
	float gWorldCellSpace;
	Vec2 gTexScale = Vec2(50.0f, 50.0f);

	Vec4 gWorldFrustumPlanes[6];
};