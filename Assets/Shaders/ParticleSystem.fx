#include "00. Global.fx"
#include "00. Light.fx"
#include "00. Render.fx"

cbuffer ParticleBuffer
{
	float3 emitPosW;
	float timeStep;
	float3 emitDirW;
	float gameTime;
    float2 emitSize;
	float lifeTime;
	float emitInterval;
    float3 emitDirRandom;
	int emitCount;
	float3 gAccelW;
	float particlePadding;
};

struct VertexInput
{
	float3 InitialPos	: POSITION;
	float3 InitialVel	: VELOCITY;
	float2 Size			: SIZE;
	float Age			: AGE;
	uint Type			: TYPE;
};

cbuffer cbFixed
{
	float2 gQuadTexC[4] =
	{
		float2(0.0f, 1.0f),
		float2(1.0f, 1.0f),
		float2(0.0f, 0.0f),
		float2(1.0f, 0.0f)
	};
}; 

//***********************************************
// STREAM-OUT TECH                              *
//***********************************************

#define PT_EMITTER 0
#define PT_FLARE 1
#define MAX_EMIT_PER_FRAME 100

VertexInput StreamOutVS(VertexInput vin)
{
	return vin;
}

// The stream-out GS is just responsible for emitting 
// new particles and destroying old particles.  The logic
// programed here will generally vary from particle system
// to particle system, as the destroy/spawn rules will be 
// different.
[maxvertexcount(101)]
void StreamOutGS(point VertexInput gin[1],
	inout PointStream<VertexInput> ptStream)
{
	if (gin[0].Type == PT_EMITTER)
	{
		float interval = max(emitInterval, 0.001f);
		gin[0].Age = gin[0].Age < 0.0f ? interval : gin[0].Age + timeStep;

		uint particlesPerEmission = (uint)clamp(emitCount, 1, MAX_EMIT_PER_FRAME);
		uint emittedCount = 0;

		// Preserve unprocessed Age as backlog when the geometry shader output limit is reached.
		while (gin[0].Age >= interval &&
			emittedCount + particlesPerEmission <= MAX_EMIT_PER_FRAME)
		{
			gin[0].Age -= interval;

			for (uint i = 0; i < particlesPerEmission; ++i)
			{
				float seed = gameTime + (float)(emittedCount + i) * 0.61803398875f;
				float3 vRandom = RandUnitVec3(seed, 0.0f) * emitDirRandom;
				float3 vRandom2 = RandUnitVec3(seed, 1.0f);

				VertexInput p;
				p.InitialPos = emitPosW.xyz;
				p.InitialVel = emitDirW + vRandom;
				p.Size = emitSize + vRandom2.xy;
				p.Age = gin[0].Age;
				p.Type = PT_FLARE;

				if (p.Age <= lifeTime)
					ptStream.Append(p);
			}

			emittedCount += particlesPerEmission;
		}

		// always keep emitters
		ptStream.Append(gin[0]);
	}
	else
	{
		gin[0].Age += timeStep;

		// Specify conditions to keep particle; this may vary from system to system.
		if (gin[0].Age <= lifeTime)
			ptStream.Append(gin[0]);
	}
}

GeometryShader gsStreamOut = ConstructGSWithSO(
	CompileShader(gs_5_0, StreamOutGS()),
	"POSITION.xyz; VELOCITY.xyz; SIZE.xy; AGE.x; TYPE.x");

technique11 StreamOutTech
{
	pass P0
	{
		SetVertexShader(CompileShader(vs_5_0, StreamOutVS()));
		SetGeometryShader(gsStreamOut);

		// disable pixel shader for stream-out only
		SetPixelShader(NULL);

		// we must also disable the depth buffer for stream-out only
		SetDepthStencilState(DisableDepth, 0);
	}
}

//***********************************************
// DRAW TECH                                    *
//***********************************************

struct VertexOut
{
	float4 PosW : POSITION;
	float2 SizeW : SIZE;
	float4 Color : COLOR;
	uint Type : TYPE;
};

VertexOut DrawVS(VertexInput vin)
{
	VertexOut vout;

	float t = vin.Age;

	// constant acceleration equation
	vout.PosW = float4(0.5f * t * t * gAccelW + t * vin.InitialVel + vin.InitialPos, 1.0f);

	// fade color with time
	float opacity = 1.0f - smoothstep(0.0f, 1.0f, t / max(lifeTime, 0.0001f));
	vout.Color = float4(1.0f, 1.0f, 1.0f, opacity);

	vout.SizeW = vin.Size;
	vout.Type = vin.Type;

	return vout;
}

struct GeoOut
{
	float4 PosH : SV_POSITION;
	float4 Color : COLOR;
	float2 Tex : TEXCOORD;
};

// The draw GS just expands points into camera facing quads.
[maxvertexcount(4)]
void DrawGS(point VertexOut gin[1],
	inout TriangleStream<GeoOut> triStream)
{
	// do not draw emitter particles.
	if (gin[0].Type != PT_EMITTER)
	{
		//
		// Compute world matrix so that billboard faces the camera.
		//
		float3 look = normalize(CameraPosition() - gin[0].PosW.xyz);
		float3 right = normalize(cross(float3(0, 1, 0), look));
		float3 up = cross(look, right);

		//
		// Compute triangle strip vertices (quad) in world space.
		//
		float halfWidth = 0.5f * gin[0].SizeW.x;
		float halfHeight = 0.5f * gin[0].SizeW.y;

		float4 v[4];
		v[0] = float4(gin[0].PosW.xyz + halfWidth * right - halfHeight * up, 1.0f);
		v[1] = float4(gin[0].PosW.xyz + halfWidth * right + halfHeight * up, 1.0f);
		v[2] = float4(gin[0].PosW.xyz - halfWidth * right - halfHeight * up, 1.0f);
		v[3] = float4(gin[0].PosW.xyz - halfWidth * right + halfHeight * up, 1.0f);

		//
		// Transform quad vertices to world space and output 
		// them as a triangle strip.
		//
		GeoOut gout;
		[unroll]
		for (int i = 0; i < 4; ++i)
		{
			gout.PosH = mul(v[i], VP);
			gout.Tex = gQuadTexC[i];
			gout.Color = gin[0].Color;
			triStream.Append(gout);
		}
	}
}

float4 DrawPS(GeoOut pin) : SV_TARGET
{
	float4 diffuse = DiffuseMap.Sample(LinearSampler, pin.Tex);
	float4 result = diffuse * pin.Color * Material.diffuse;
	return result;
}

technique11 DrawTech
{
	pass P0
	{
		SetVertexShader(CompileShader(vs_5_0, DrawVS()));
		SetGeometryShader(CompileShader(gs_5_0, DrawGS()));
		SetPixelShader(CompileShader(ps_5_0, DrawPS()));

		SetBlendState(AdditiveBlend, float4(0.0f, 0.0f, 0.0f, 0.0f), 0xffffffff);
		SetDepthStencilState(NoDepthWrites, 0);
	}
};
