#ifndef _RENDER_FX_
#define _RENDER_FX_

#include "00. Global.fx"
#include "00. Light.fx"

#define MAX_MODEL_TRANSFORMS 100
#define MAX_MODEL_KEYFRAMES 500
#define MAX_MODEL_INSTANCE 200
#define MAX_BLEND_ANIMATIONS 3

// ************** MeshRender ****************

struct VertexMesh_NotInst
{
    float4 position : POSITION;
    float2 uv : TEXCOORD;
    float3 normal : NORMAL;
    float3 tangent : TANGENT;
};

MeshOutput VS_Mesh_NotInst(VertexMesh_NotInst input)
{
    MeshOutput output;

    float4 worldPos = mul(input.position, W);
    output.position = worldPos; // W
    output.worldPosition = output.position;
    output.position = mul(output.position, VP);
    output.uv = input.uv;
	
    output.normal = mul(input.normal, (float3x3) W);
    output.normalV = mul(output.normal, (float3x3) V);
    output.positionV = mul(worldPos, V);
    output.viewZ = output.positionV.z;
    output.ssaoPosH = mul(worldPos, VPT);
	
    output.tangent = input.tangent; //mul(input.tangent, (float3x3) W);
	
    return output;
}

struct VertexMesh
{
	float4 position : POSITION;
	float2 uv : TEXCOORD;
	float3 normal : NORMAL;
	float3 tangent : TANGENT;
	// INSTANCING;
	uint instanceID : SV_INSTANCEID;
	matrix world : INST;
};

MeshOutput VS_Mesh(VertexMesh input)
{
	MeshOutput output;

	float4 worldPos = mul(input.position, input.world);
	output.position = worldPos; // W
	output.worldPosition = output.position;
	output.position = mul(output.position, VP);
	output.uv = input.uv;
	
    output.normal = mul(input.normal, (float3x3) input.world);
    output.normalV = mul(output.normal, (float3x3) V);
    output.positionV = mul(worldPos, V);
    output.viewZ = output.positionV.z;
    output.ssaoPosH = mul(worldPos, VPT);
	
    output.tangent = mul(input.tangent, (float3x3) W);
	
	return output;
}

// ************** ModelRender ****************

struct VertexModel
{
	float4 position : POSITION;
	float2 uv : TEXCOORD;
	float3 normal : NORMAL;
	float3 tangent : TANGENT;
	float4 blendIndices : BLEND_INDICES;
	float4 blendWeights : BLEND_WEIGHTS;
	// INSTANCING;
	uint instanceID : SV_INSTANCEID;
	matrix world : INST;
};

cbuffer BoneBuffer
{
	matrix BoneTransforms[MAX_MODEL_TRANSFORMS];
};

uint BoneIndex;

MeshOutput VS_Model(VertexModel input)
{
	MeshOutput output;

	//float4 worldPos = mul(input.position, BoneTransforms[BoneIndex]);
    float4 worldPos = mul(input.position, input.world); // W
	//output.position = mul(input.position, BoneTransforms[BoneIndex]); // Model Global
	//output.position = mul(output.position, input.world); // W
	output.position = worldPos;
	output.worldPosition = output.position;
	output.position = mul(output.position, VP);
    output.uv = input.uv;
	
    matrix worldMat = input.world;
    output.normal = mul(input.normal, (float3x3) worldMat);
    output.normalV = mul(output.normal, (float3x3) V);
    output.positionV = mul(worldPos, V);
    output.viewZ = output.positionV.z;
    output.ssaoPosH = mul(worldPos, VPT);
    output.tangent = mul(input.tangent, (float3x3) worldMat);
	
	return output;
}

// ************** AnimRender ****************

struct AnimationFrameDesc
{
	int animIndex;
	uint currFrame;
	uint nextFrame;
	float ratio;
};

struct KeyframeDesc
{
	// 한 재생 상태에서 최대 3개 애니메이션을 좌표 가중치로 합성한다.
	AnimationFrameDesc animations[MAX_BLEND_ANIMATIONS];
	float3 blendWeights;
	float sumTime;
    uint isBlendSpace;
    uint3 padding;
};

struct TweenFrameDesc
{
	float tweenDuration;
	float tweenRatio;
	float tweenSumTime;
	float padding;
	KeyframeDesc curr;
	KeyframeDesc next;
};

cbuffer TweenBuffer
{
	TweenFrameDesc TweenFrames[MAX_MODEL_INSTANCE];
};

Texture2DArray TransformMap;

matrix LoadAnimationMatrix(AnimationFrameDesc animation, float boneIndex)
{
	float4 c0 = TransformMap.Load(int4(boneIndex * 4 + 0, animation.currFrame, animation.animIndex, 0));
	float4 c1 = TransformMap.Load(int4(boneIndex * 4 + 1, animation.currFrame, animation.animIndex, 0));
	float4 c2 = TransformMap.Load(int4(boneIndex * 4 + 2, animation.currFrame, animation.animIndex, 0));
	float4 c3 = TransformMap.Load(int4(boneIndex * 4 + 3, animation.currFrame, animation.animIndex, 0));
	
	float4 n0 = TransformMap.Load(int4(boneIndex * 4 + 0, animation.nextFrame, animation.animIndex, 0));
	float4 n1 = TransformMap.Load(int4(boneIndex * 4 + 1, animation.nextFrame, animation.animIndex, 0));
	float4 n2 = TransformMap.Load(int4(boneIndex * 4 + 2, animation.nextFrame, animation.animIndex, 0));
	float4 n3 = TransformMap.Load(int4(boneIndex * 4 + 3, animation.nextFrame, animation.animIndex, 0));
	
	return lerp(matrix(c0, c1, c2, c3), matrix(n0, n1, n2, n3), animation.ratio);
}

matrix LoadKeyframeMatrix(KeyframeDesc keyframe, float boneIndex)
{
	// 슬롯 번호별 하드코딩 없이 유효한 애니메이션만 순회하여 합성한다.
	matrix result = 0;
	for (int i = 0; i < MAX_BLEND_ANIMATIONS; ++i)
	{
		if (keyframe.animations[i].animIndex >= 0 && keyframe.blendWeights[i] > 0.0f)
			result += keyframe.blendWeights[i] * LoadAnimationMatrix(keyframe.animations[i], boneIndex);
	}
	return result;
}

matrix GetAnimationMatrix(VertexModel input)
{
	TweenFrameDesc tween = TweenFrames[input.instanceID];
    matrix skinningTransform = 0;
	
    bool hasNextAnimation = false;

    for (int i = 0; i < MAX_BLEND_ANIMATIONS; ++i)
    {
        if (tween.next.animations[i].animIndex >= 0 &&
        tween.next.blendWeights[i] > 0.0f)
        {
            hasNextAnimation = true;
            break;
        }
    }

	for (int influenceIndex = 0; influenceIndex < 4; ++influenceIndex)
	{
		matrix blendedAnimation = LoadKeyframeMatrix(tween.curr, input.blendIndices[influenceIndex]);
		
        if (hasNextAnimation)
        {
            blendedAnimation = lerp(blendedAnimation,
				LoadKeyframeMatrix(tween.next, input.blendIndices[influenceIndex]), tween.tweenRatio);
        }
		skinningTransform += input.blendWeights[influenceIndex] * blendedAnimation;
	}

	return skinningTransform;
}

MeshOutput VS_Animation(VertexModel input)
{
	MeshOutput output;

	//output.position = mul(input.position, BoneTransforms[BoneIndex]); // Model Global

	matrix m = GetAnimationMatrix(input);
    matrix worldMat = mul(m, input.world);
	
    float4 worldPos = mul(input.position, worldMat);
	//output.position = mul(input.position, m);
	//output.position = mul(output.position, input.world); // W
	output.position = worldPos;
	output.worldPosition = output.position;
	output.position = mul(output.position, VP);
	output.uv = input.uv;
    output.normal = mul(input.normal, (float3x3) worldMat);
    output.normal = normalize(output.normal);
    output.normalV = mul(output.normal, (float3x3) V);
    output.positionV = mul(worldPos, V);
    output.tangent = mul(input.tangent, (float3x3) worldMat);
    output.viewZ = output.positionV.z;
    output.ssaoPosH = mul(worldPos, VPT);

	return output;
}

#endif

