#include "00. GrassCommon.fx"
#include "00. Global.fx"

cbuffer GrassConstant
{
    uint totalGrassCount; // 초기 풀 버퍼의 전체 개수
    float3 padding2;
    float4 worldFrustumPlanes[6];
};

// SRV: 읽기 전용 버퍼 (초기 풀 위치 데이터)
StructuredBuffer<GrassBlade> Input;
// UAV: 쓰기 가능한 버퍼 (컬링된 풀잎을 여기에 모읍니다.)
// AppendStructuredBuffer는 자동으로 카운터를 증가시킵니다.
AppendStructuredBuffer<GrassBlade> DistantOutput : register(u0);


// 스레드 그룹 크기 정의 (예: 256)
[numthreads(256, 1, 1)]
void CS(uint3 DTid : SV_DispatchThreadID)
{
    // 현재 처리할 풀잎의 인덱스
    uint index = DTid.x;

    if (index >= totalGrassCount)
        return;

    GrassBlade initialBlade = Input[index];
    //initialBlade.position = float3(index, 0, 0); // 임시 위치
    
    // 1. LoD 및 Frustum Culling (컬링)
    // 카메라와의 거리 계산
    float distSq = distance(initialBlade.position, CamPos);
    
    // 특정 거리(예: MaxRenderDistance) 밖에 있으면 컬링
    if (distSq < 100 || distSq > 500)
    {
        return; // 컬링: 렌더링하지 않음
    }
    
    
    if (AabbOutsideFrustumTest(initialBlade.position, float3(1, 1, 1), worldFrustumPlanes))
    {
        return;
    }
    
    // 3. 최종 풀 버퍼에 추가 (컬링을 통과한 풀잎만)
    DistantOutput.Append(initialBlade);
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