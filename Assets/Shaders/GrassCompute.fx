#include "00. GrassCommon.fx"
#include "00. Global.fx"

#define MAX_UV_COUNT 30

cbuffer GrassConstant
{
    uint totalGrassCount; // 초기 풀 버퍼의 전체 개수
    float terrainWidth;
    float terrainDepth;
    uint uvCount;
    float4 uvs[MAX_UV_COUNT];
    float4 worldFrustumPlanes[6];
};

Texture2DArray LayerMapArray;
Texture2D BlendMap;

// SRV: 읽기 전용 버퍼 (초기 풀 위치 데이터)
StructuredBuffer<GrassPosition> Input;
// UAV: 쓰기 가능한 버퍼 (컬링된 풀잎을 여기에 모읍니다.)
// AppendStructuredBuffer는 자동으로 카운터를 증가시킵니다.
AppendStructuredBuffer<NearbyGrassData> NearbyOutput : register(u0);
AppendStructuredBuffer<DistantGrassData> DistantOutput : register(u1);

float4x4 ScaleMatrix(float3 s)
{
    return float4x4(
        s.x, 0, 0, 0,
        0, s.y, 0, 0,
        0, 0, s.z, 0,
        0, 0, 0, 1
    );
}

float4x4 RotationMatrixXYZ(float3 r)
{
    float cx = cos(r.x);
    float sx = sin(r.x);
    float cy = cos(r.y);
    float sy = sin(r.y);
    float cz = cos(r.z);
    float sz = sin(r.z);

    float4x4 Rx = float4x4(
        1, 0, 0, 0,
        0, cx, -sx, 0,
        0, sx, cx, 0,
        0, 0, 0, 1
    );

    float4x4 Ry = float4x4(
         cy, 0, sy, 0,
         0, 1, 0, 0,
        -sy, 0, cy, 0,
         0, 0, 0, 1
    );

    float4x4 Rz = float4x4(
        cz, -sz, 0, 0,
        sz, cz, 0, 0,
        0, 0, 1, 0,
        0, 0, 0, 1
    );

    // XYZ 회전
    return mul(Rz, mul(Ry, Rx));
}

float4x4 TranslationMatrix(float3 t)
{
    return float4x4(
        1, 0, 0, t.x,
        0, 1, 0, t.y,
        0, 0, 1, t.z,
        0, 0, 0, 1
    );
}

float3 SampleRandom(float v, float seed)
{
    float key = frac(sin(v * 12.9898 + seed * 78.233) * 43758.5453);
    return RandomMap.SampleLevel(LinearSampler, key, 0).xyz;
}

float3 RGBtoHSV(float3 c)
{
    float4 K = float4(0.0, -1.0 / 3.0, 2.0 / 3.0, -1.0);
    float4 p = lerp(float4(c.bg, K.wz), float4(c.gb, K.xy), step(c.b, c.g));
    float4 q = lerp(float4(p.xyw, c.r), float4(c.r, p.yzx), step(p.x, c.r));

    float d = q.x - min(q.w, q.y);
    float e = 1e-10;
    return float3(abs(q.z + (q.w - q.y) / (6.0 * d + e)), d / (q.x + e), q.x);
}

float GreenSimilarity(float3 rgb)
{
    float3 hsv = RGBtoHSV(rgb);

    // 초록색 Hue (0~1 기준)
    const float greenHue = 1.0 / 3.0;

    // Hue는 원형이므로 wrap 처리
    float dh = abs(hsv.x - greenHue);
    dh = min(dh, 1.0 - dh);

    // 허용 범위 (약 ±60도)
    const float maxDist = 1.0 / 6.0;

    return saturate(1.0 - dh / maxDist);
}


// 스레드 그룹 크기 정의 (예: 256)
[numthreads(256, 1, 1)]
void CS(uint3 DTid : SV_DispatchThreadID)
{
    // 현재 처리할 풀잎의 인덱스
    uint index = DTid.x;

    if (index >= totalGrassCount)
        return;

    GrassPosition grassPosition = Input[index];
    float3 position = grassPosition.position;
    //initialBlade.position = float3(index, 0, 0); // 임시 위치
    
    // 1. LoD 및 Frustum Culling (컬링)
    // 카메라와의 거리 계산
    float distSq = distance(position, CamPos);
    
    // 특정 거리(예: MaxRenderDistance) 밖에 있으면 컬링
    if (distSq > 500)
    {
        return; // 컬링: 렌더링하지 않음
    }
    
    if (AabbOutsideFrustumTest(position, float3(5, 5, 5), worldFrustumPlanes))
    {
        return;
    }
    
    float2 halfWidthDepth = float2(terrainWidth, terrainDepth) * 0.5f;
    float2 pinTex = (position.xz + halfWidthDepth) / float2(terrainWidth, terrainDepth);
    pinTex.y = 1 - pinTex.y;
    
    // Sample layers in texture array.
    float4 c0 = LayerMapArray.SampleLevel(LinearSampler, float3(pinTex, 0), 0);
    float4 c1 = LayerMapArray.SampleLevel(LinearSampler, float3(pinTex, 1), 0);
    float4 c2 = LayerMapArray.SampleLevel(LinearSampler, float3(pinTex, 2), 0);
    float4 c3 = LayerMapArray.SampleLevel(LinearSampler, float3(pinTex, 3), 0);
    float4 c4 = LayerMapArray.SampleLevel(LinearSampler, float3(pinTex, 4), 0);
    
	// Sample the blend map.
    float4 t = BlendMap.SampleLevel(LinearSampler, pinTex, 0);

	// Blend the layers on top of each other.
    float4 texColor = c0;
    texColor = lerp(texColor, c1, t.r);
    texColor = lerp(texColor, c2, t.g);
    texColor = lerp(texColor, c3, t.b);
    texColor = lerp(texColor, c4, t.a);
    
    float randomKey = position.x + position.y + position.z;
    int seed = 0;
    
    //if (GreenSimilarity(texColor.rgb) < SampleRandom(randomKey, seed++).x)
    //{
    //    texColor = float4(1, 0, 0, 1);
    //}
    
    int uvIndex = (SampleRandom(randomKey, seed++).x + 1) * 0.5 * uvCount;
    float4 uvMinMax = uvs[uvIndex];
    float3 scaleBase = float3(uvMinMax.z - uvMinMax.x, uvMinMax.y - uvMinMax.w, 1);
    scaleBase *= 8.f;
    if (distSq < 120)
    {
        [unroll]
        for (int i = 0; i < 3; i++)
        {
            NearbyGrassData output;
            float3 rotation = SampleRandom(randomKey, seed++);
            rotation.x *= 0.25f * PI;
            rotation.y *= 2.f * PI;
            rotation.z = 0.f;
            float3 scaleOffset = SampleRandom(randomKey, seed++);
            scaleOffset *= 0.2f;
            float3 scale = scaleBase + scaleOffset;
            scale.z = 1.f;
            
            float3 positionOffset = SampleRandom(randomKey, seed++);
            positionOffset *= 0.1f;
            positionOffset.y = 0;
            
            float4x4 S = ScaleMatrix(scaleBase);
            float4x4 R = RotationMatrixXYZ(rotation);
            float4x4 T = TranslationMatrix(position + positionOffset);
        
            output.worldMatrix = mul(T, mul(R, S));
            output.uvMinMax = uvMinMax;
            output.terrainColor = texColor;
            NearbyOutput.Append(output);
        }
    }
    else
    {
        DistantGrassData output;
        output.position = position;
        output.uvMinMax = uvMinMax;
        output.scale = scaleBase.xy;
        output.terrainColor = texColor;
        DistantOutput.Append(output);
    }
}

technique11 T0
{
    pass P0
    {
        SetVertexShader(NULL);
        SetPixelShader(NULL);
        SetComputeShader(CompileShader(cs_5_0, CS()));
    }
};