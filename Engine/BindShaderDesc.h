#pragma once
#include "ConstantBuffer.h"
#include "ResourceRef.h"

class Texture;

struct GlobalDesc
{
	Matrix V = Matrix::Identity;
	Matrix P = Matrix::Identity;
	Matrix VP = Matrix::Identity;
	Matrix VPT = Matrix::Identity;
	Matrix VInv = Matrix::Identity;
	Vec3 CamPos = Vec3(0, 0, 0);
	float Time;
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

    template<typename Archive>
	void serialize(Archive& archive)
	{
		archive(CEREAL_NVP(ambient));
		archive(CEREAL_NVP(diffuse));
		archive(CEREAL_NVP(specular));
		archive(CEREAL_NVP(emissive));
	}
};

struct MaterialDesc
{
	Color ambient = Color(1.f, 1.f, 1.f, 1.f);
	Color diffuse = Color(1.f, 1.f, 1.f, 1.f);
	Color specular = Color(1.f, 1.f, 1.f, 1.f);
	Color emissive = Color(0.f, 0.f, 0.f, 1.f);

    template<typename Archive>
    void serialize(Archive& archive)
    {
		archive(CEREAL_NVP(ambient));
        archive(CEREAL_NVP(diffuse));
        archive(CEREAL_NVP(specular));
        archive(CEREAL_NVP(emissive));
    }
};

// Bone
#define MAX_MODEL_TRANSFORMS 100
#define MAX_MODEL_KEYFRAMES 500
#define MAX_MODEL_INSTANCE 200
#define MAX_BLEND_ANIMATIONS 3

struct BoneDesc
{
	Matrix transforms[MAX_MODEL_TRANSFORMS];
};

// Animation
struct AnimationFrameDesc
{
	int32 animIndex = -1;
	uint32 curFrame = 0;
	uint32 nextFrame = 0;
	float ratio = 0.f;
};

// 일반 애니메이션 하나 또는 블렌드 애니메이션 최대 3개의 프레임 상태를 담는다.
struct KeyframeDesc
{
	array<AnimationFrameDesc, MAX_BLEND_ANIMATIONS> animations;
	Vec3 blendWeights = Vec3(1.f, 0.f, 0.f);
	float sumTime = 0.f;
	uint32 isBlendSpace = 0;
	uint32 padding[3] = {};

	void SetSingleAnimation(int32 animIndex)
	{
		*this = {};
		animations[0].animIndex = animIndex;
		blendWeights = Vec3(1.f, 0.f, 0.f);
	}

	bool HasAnimation() const
	{
		for (const AnimationFrameDesc& animation : animations)
		{
			if (animation.animIndex >= 0)
				return true;
		}
		return false;
	}

    int32 GetSingleAnimationIndex() const
    {
        if (!HasAnimation())
            return -1;
        if (isBlendSpace)
            return -1;

        return animations[0].animIndex;
    }
};

enum class ShaderPropertyType : uint8
{
	Float,
	Color,
	Int,
	Bool,
	Texture2D,
	Float2,
	Float3,
	Float4,
	Matrix,
};

// 머티리얼마다 저장되는 셰이더 프로퍼티 값이다.
struct MaterialPropertyValue
{
	static constexpr int CurrentVersion = 5;

	int _version = CurrentVersion;
	string name;
	ShaderPropertyType type = ShaderPropertyType::Float;
	Vec4 float4Value = Vec4(0.f, 0.f, 0.f, 0.f);
	int32 intValue = 0;
	bool boolValue = false;
	ResourceRef<Texture> textureValue;
	Matrix matrixValue = Matrix::Identity;

	template<typename Archive>
	void serialize(Archive& archive)
	{
		if (Archive::is_saving::value)
			_version = CurrentVersion;

		archive(CEREAL_NVP(_version));
		archive(CEREAL_NVP(name));
		archive(CEREAL_NVP(type));

		if (type == ShaderPropertyType::Float ||
			type == ShaderPropertyType::Float2 ||
			type == ShaderPropertyType::Float3 ||
			type == ShaderPropertyType::Float4 ||
			type == ShaderPropertyType::Color)
		{
			archive(CEREAL_NVP(float4Value));
		}
		else if (type == ShaderPropertyType::Int)
		{
			archive(CEREAL_NVP(intValue));
		}
		else if (type == ShaderPropertyType::Bool)
		{
			archive(CEREAL_NVP(boolValue));
		}
		else if (type == ShaderPropertyType::Texture2D)
		{
			archive(CEREAL_NVP(textureValue));
		}
		else if (type == ShaderPropertyType::Matrix)
		{
			archive(CEREAL_NVP(matrixValue));
		}
	}
};

struct TweenDesc
{
	TweenDesc()
	{
		cur.SetSingleAnimation(0);
	}

	void ClearNextAnim()
	{
		next = {};
		tweenSumTime = 0.f;
		tweenRatio = 0.f;
	}

	float tweenDuration = 0.1f;
	float tweenRatio = 0.f;
	float tweenSumTime = 0.f;
	float speed = 1.f;
	KeyframeDesc cur;
	KeyframeDesc next;

	template<typename Archive>
	void serialize(Archive& ar)
	{
        ar(CEREAL_NVP(tweenDuration));
	}
};

struct InstancedTweenDesc
{
	TweenDesc tweens[MAX_MODEL_INSTANCE];
};

struct SnowBillboardDesc
{
	Color color = Color(1, 1, 1, 1);

	Vec3 velocity = Vec3(0, -20, 0);
	float drawDistance = 0;

	Vec3 origin = Vec3(0, 0, 0);
	float turbulence = 5;

	Vec3 extent = Vec3(0, 0, 0);
	float time = 0;

    template<typename Archive>
    void serialize(Archive& archive)
    {
        archive(CEREAL_NVP(color));
        archive(CEREAL_NVP(velocity));
        archive(CEREAL_NVP(drawDistance));
        archive(CEREAL_NVP(origin));
        archive(CEREAL_NVP(turbulence));
        archive(CEREAL_NVP(extent));
    }
};

struct ParticleDesc
{
	Vec3 emitPosW = Vec3(0, 0, 0);
	float timeStep = 0.f;
	Vec3 emitDirW = Vec3(0, 0, 0);
	float gameTime = 0.f;
	Vec2 emitSize = Vec2(1, 1);
	float lifeTime = 1.f;
	float emitInterval = 0.005f;
    Vec3 emitDirRandomness = Vec3(0, 0, 0);
	int32 emitCount = 1;
	Vec3 gAccelW = Vec3(0.f, 7.8f, 0.f);
	float particlePadding = 0.f;
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
	float gTexelWidth = 0.f;
	float gTexelHeight = 0.f;
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
	float gMinDist = 20.0f;
	float gMaxDist = 500.0f;

	// Exponents for power of 2 tessellation.  The tessellation
	// range is [2^(gMinTess), 2^(gMaxTess)].  Since the maximum
	// tessellation is 64, this means gMaxTess can be at most 6
	// since 2^6 = 64.
	float gMinTess = 0.0f;
	float gMaxTess = 6.0f;

	float gTexelCellSpaceU;
	float gTexelCellSpaceV;
	float gWorldCellSpace;

	float dummy;

	Vec4 gWorldFrustumPlanes[6];
	Vec2 gTexScale = Vec2(50.0f, 50.0f);
	float brushRadius;
	float gUseLayerNormalMap;
	Vec3 brushPos;
	float dummy3;
	// x: strength, y: full-detail distance, z: fade-out distance, w: enabled
	Vec4 gLayerHeightParams = Vec4(0.25f, 20.0f, 80.0f, 0.0f);
};

struct WindDesc
{
	Vec3 windDirection = Vec3(1.f,0.f,0.f); // 바람 방향 (정규화된 벡터)
	float windStrength = 1.f; // 바람 강도 (흔들림 정도 조절)
	float waveFrequency = 1.f; // 흔들림 파동의 빈도
};

struct FoliageDesc
{
	float time; // 현재 시간 (애니메이션 구동용)
    WindDesc wind; // 바람 정보
	float bendFactor = 0.f; // 수풀 하단 고정/상단 흔들림 정도 조절
	float stiffness = 0.95f;
};

struct ShadowDesc
{
	Matrix Transforms[NUM_SHADOW_CASCADES];
	Vec4 cascadeEnds;
	float farLength;
	float shaodwPadding[3];
};

static_assert(sizeof(InstancedTweenDesc) <= 65536, "Tween 상수 버퍼가 D3D11의 64KB 제한을 초과합니다.");
