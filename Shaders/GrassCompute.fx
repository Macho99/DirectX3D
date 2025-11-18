#include "00. GrassCommon.fx"
#include "00. Global.fx"

cbuffer GrassConstant
{
    uint totalGrassCount; // 초기 풀 버퍼의 전체 개수
    float3 padding2;
};

// SRV: 읽기 전용 버퍼 (초기 풀 위치 데이터)
StructuredBuffer<GrassBlade> g_InitialGrassBuffer;
// UAV: 쓰기 가능한 버퍼 (컬링된 풀잎을 여기에 모읍니다.)
// AppendStructuredBuffer는 자동으로 카운터를 증가시킵니다.
AppendStructuredBuffer<GrassBlade> g_FinalGrassBuffer;


// 스레드 그룹 크기 정의 (예: 256)
[numthreads(256, 1, 1)]
void CS(uint3 DTid : SV_DispatchThreadID)
{
    // 현재 처리할 풀잎의 인덱스
    uint index = DTid.x;

    //if (index >= totalGrassCount)
    //    return;

    GrassBlade initialBlade = g_InitialGrassBuffer[index];
    //initialBlade.position = float3(index, 0, 0); // 임시 위치
    
    // 1. LoD 및 Frustum Culling (컬링)
    // 카메라와의 거리 계산
    /*float distSq = distance(initialBlade.position, CamPos);
    
    // 특정 거리(예: MaxRenderDistance) 밖에 있으면 컬링
    if (distSq > 100 * 100)
    {
        return; // 컬링: 렌더링하지 않음
    }
    */
    // 3. 최종 풀 버퍼에 추가 (컬링을 통과한 풀잎만)
    g_FinalGrassBuffer.Append(initialBlade);
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