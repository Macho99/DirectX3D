#ifndef _GRASS_FX_
#define _GRASS_FX_

struct GrassPosition
{
    float3 position;
    float padding;
};

struct NearbyGrassData
{
    float4x4 worldMatrix;
    float4 uvMinMax;
    float4 terrainColor;
};

struct DistantGrassData
{
    float3 position;
    float2 scale;
    float4 uvMinMax;
    float4 terrainColor;
};

cbuffer FoliageBuffer
{
    float time; // 현재 시간 (애니메이션 구동용)
    float3 windDirection; // 바람 방향 (정규화된 벡터)
    float windStrength; // 바람 강도 (흔들림 정도 조절)
    float waveFrequency; // 흔들림 파동의 빈도
    float bendFactor; // 수풀 하단 고정/상단 흔들림 정도 조절
    float stiffness;
};

float3 CalculateWindOffset(float3 worldPos, float localPosY)
{
    // a. 흔들림의 세기 결정: 시간과 월드 위치를 조합하여 파동 생성
    // sin 함수를 사용하여 부드러운 왕복 운동 구현
    float wave = sin(worldPos.x * waveFrequency * 0.5 +
                     worldPos.z * waveFrequency * 0.7 +
                     time);
    
    // b. 수풀 높이에 따른 흔들림 강도 조절 (하단 고정, 상단 최대)
    // BendFactor는 흔들림이 시작되는 높이를 조절하거나, 전체 강도를 조정
    float heightInfluence = saturate((localPosY - bendFactor) * (1 - stiffness)); // 0.0 ~ 1.0
    
    // c. 최종 이동량 계산
    // WindDirection(바람 방향) * Wave(시간 파동) * WindStrength(강도) * HeightInfluence(높이 영향)
    return windDirection.xyz * wave * windStrength * heightInfluence;
}

#endif