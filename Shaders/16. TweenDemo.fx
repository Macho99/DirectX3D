#include "00. Global.fx"
#include "00. Light.fx"

#define MAX_MODEL_TRANSFORMS 250
#define MAX_MODEL_KEYFRAMES 500

struct KeyframeDesc
{
	int animIndex;
	uint curFrame;
	uint nextFrame;
	float ratio;
	float sumTime;
	float speed;
	float2 padding;
};

struct TweenframeDesc
{
	float tweenDuration;
	float tweenRatio;
	float tweenSumTime;
	float padding;
	KeyframeDesc cur;
	KeyframeDesc next;
};

cbuffer TweenBuffer
{
	TweenframeDesc Tweenframes;
};

cbuffer BoneBuffer
{
	matrix BoneTransform[MAX_MODEL_TRANSFORMS];
};

uint BoneIndex;
Texture2DArray TransformMap;

matrix GetAnimationMatrix(VertexTextureNormalTangentBlend input)
{
	float indices[4] = { input.blendIndices.x, input.blendIndices.y, input.blendIndices.z, input.blendIndices.w };
	float weights[4] = { input.blendWeights.x, input.blendWeights.y, input.blendWeights.z, input.blendWeights.w };

	int animIndex[2];
	int curFrame[2];
	int nextFrame[2];
	float ratio[2];

	animIndex[0] = Tweenframes.cur.animIndex;
	curFrame[0] = Tweenframes.cur.curFrame;
	nextFrame[0] = Tweenframes.cur.nextFrame;
	ratio[0] = Tweenframes.cur.ratio;

	animIndex[1] = Tweenframes.next.animIndex;
	curFrame[1] = Tweenframes.next.curFrame;
	nextFrame[1] = Tweenframes.next.nextFrame;
	ratio[1] = Tweenframes.next.ratio;

	float4 c0, c1, c2, c3; // current
	float4 n0, n1, n2, n3; // next
	matrix cur = 0;
	matrix next = 0;
	matrix transform = 0;

	for (int i = 0; i < 4; i++)
	{
		c0 = TransformMap.Load(int4((indices[i] * 4 + 0), curFrame[0], animIndex[0], 0));
		c1 = TransformMap.Load(int4((indices[i] * 4 + 1), curFrame[0], animIndex[0], 0));
		c2 = TransformMap.Load(int4((indices[i] * 4 + 2), curFrame[0], animIndex[0], 0));
		c3 = TransformMap.Load(int4((indices[i] * 4 + 3), curFrame[0], animIndex[0], 0));
		cur = matrix(c0, c1, c2, c3);

		n0 = TransformMap.Load(int4((indices[i] * 4 + 0), nextFrame[0], animIndex[0], 0));
		n1 = TransformMap.Load(int4((indices[i] * 4 + 1), nextFrame[0], animIndex[0], 0));
		n2 = TransformMap.Load(int4((indices[i] * 4 + 2), nextFrame[0], animIndex[0], 0));
		n3 = TransformMap.Load(int4((indices[i] * 4 + 3), nextFrame[0], animIndex[0], 0));
		next = matrix(n0, n1, n2, n3);

		matrix result = lerp(cur, next, ratio[0]);

		// 다음 애니메이션
		if (animIndex[1] >= 0)
		{
			c0 = TransformMap.Load(int4(indices[i] * 4 + 0, curFrame[1], animIndex[1], 0));
			c1 = TransformMap.Load(int4(indices[i] * 4 + 1, curFrame[1], animIndex[1], 0));
			c2 = TransformMap.Load(int4(indices[i] * 4 + 2, curFrame[1], animIndex[1], 0));
			c3 = TransformMap.Load(int4(indices[i] * 4 + 3, curFrame[1], animIndex[1], 0));
			cur = matrix(c0, c1, c2, c3);

			n0 = TransformMap.Load(int4(indices[i] * 4 + 0, nextFrame[1], animIndex[1], 0));
			n1 = TransformMap.Load(int4(indices[i] * 4 + 1, nextFrame[1], animIndex[1], 0));
			n2 = TransformMap.Load(int4(indices[i] * 4 + 2, nextFrame[1], animIndex[1], 0));
			n3 = TransformMap.Load(int4(indices[i] * 4 + 3, nextFrame[1], animIndex[1], 0));
			next = matrix(n0, n1, n2, n3);

			matrix nextResult = lerp(cur, next, ratio[1]);
			result = lerp(result, nextResult, Tweenframes.tweenRatio);
		}

		transform += mul(weights[i], result);
	}

	return transform;
}

MeshOutput VS(VertexTextureNormalTangentBlend input)
{
	MeshOutput output;

	matrix m = GetAnimationMatrix(input);

	output.position = mul(input.position, m);
	output.position = mul(output.position, W);
	output.worldPosition = output.position.xyz;
	output.position = mul(output.position, VP);	

	output.uv = input.uv;
	output.normal = mul(input.normal, (float3x3)W);
	output.tangent = mul(input.tangent, (float3x3)W);

	return output;
}

float4 PS(MeshOutput input) : SV_TARGET
{
	//ComputeNormalMapping(input.normal, input.tangent, input.uv);
	//float4 color = ComputeLight(input.normal, input.uv, input.worldPosition);
	float4 color = DiffuseMap.Sample(LinearSampler, input.uv);
	return color;
}

float4 PS_RED(MeshOutput input) : SV_TARGET
{
	return float4(1, 0, 0, 1);
}

technique11 T0
{
	PASS_VP(P0, VS, PS)
	PASS_RS_VP(P1, FillModeWireFrame, VS, PS_RED)
};