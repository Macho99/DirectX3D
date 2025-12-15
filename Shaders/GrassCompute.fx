#include "00. GrassCommon.fx"
#include "00. Global.fx"

cbuffer GrassConstant
{
    uint totalGrassCount; // 초기 풀 버퍼의 전체 개수
    float3 padding2;
    float4 worldFrustumPlanes[6];
};

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
    
    
    if (AabbOutsideFrustumTest(position, float3(1, 1, 1), worldFrustumPlanes))
    {
        return;
    }
    
    float randomKey = position.x + position.y + position.z;
    int seed = 0;
    if (distSq < 120)
    {
        [unroll]
        for (int i = 0; i < 3; i++)
        {
            NearbyGrassData output;
            float3 rotation = SampleRandom(randomKey, seed++);
            rotation.x *= 0.25f * PI;
            rotation.y *= 2.f * PI;
            rotation.z = 0;
            float3 scale = SampleRandom(randomKey, seed++);
            scale *= 0.5f;
            scale += 1.f;
            float3 positionOffset = SampleRandom(randomKey, seed++);
            positionOffset *= 0.3f;
            positionOffset.y = 0;
            
            float4x4 S = ScaleMatrix(scale);
            float4x4 R = RotationMatrixXYZ(rotation);
            float4x4 T = TranslationMatrix(position + positionOffset);
        
            output.worldMatrix = mul(T, mul(R, S));
            NearbyOutput.Append(output);
        }
    }
    else
    {
        DistantGrassData output;
        output.position = position;
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