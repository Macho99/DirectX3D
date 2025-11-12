#include "00. Global.fx"
#include "00. Light.fx"
#include "00. Render.fx"

cbuffer FoliageBuffer
{
    float time; // 현재 시간 (애니메이션 구동용)
    float3 windDirection; // 바람 방향 (정규화된 벡터)
    float windStrength; // 바람 강도 (흔들림 정도 조절)
    float waveFrequency; // 흔들림 파동의 빈도
    float bendFactor; // 수풀 하단 고정/상단 흔들림 정도 조절
    float stiffness;
};

MeshOutput VS(VertexMesh input)
{
    MeshOutput output;
    
    float4 worldPos = mul(input.position, input.world);
        
    // a. 흔들림의 세기 결정: 시간과 월드 위치를 조합하여 파동 생성
    // sin 함수를 사용하여 부드러운 왕복 운동 구현
    float wave = sin(worldPos.x * waveFrequency * 0.5 +
                     worldPos.z * waveFrequency * 0.7 +
                     time);
    
    // b. 수풀 높이에 따른 흔들림 강도 조절 (하단 고정, 상단 최대)
    // input.position.y (로컬 Y축, 일반적으로 수풀의 높이)를 사용
    // BendFactor는 흔들림이 시작되는 높이를 조절하거나, 전체 강도를 조정
    float heightInfluence = saturate((input.position.y - bendFactor) * (1 - stiffness)); // 0.0 ~ 1.0
    
    // c. 최종 이동량 계산
    // WindDirection(바람 방향) * Wave(시간 파동) * WindStrength(강도) * HeightInfluence(높이 영향)
    float3 windOffset = windDirection.xyz * wave * windStrength * heightInfluence;
    worldPos.xyz += windOffset;
    output.position = worldPos; // W
    output.worldPosition = output.position;
    output.position = mul(output.position, VP);
    output.uv = input.uv;
	
    output.normal = mul(input.normal, (float3x3) W);
    output.normalV = mul(output.normal, (float3x3) V);
    output.positionV = mul(worldPos, V);
    output.shadowPosH = mul(worldPos, ShadowTransform);
    output.ssaoPosH = mul(worldPos, VPT);
	
    return output;
}

float4 AlphaClipPS(MeshOutput input) : SV_TARGET
{
    float shadow = CalcShadowFactor(ShadowMap, input.shadowPosH);
    float4 color = ComputeLight(input.normal, input.uv, input.worldPosition, input.ssaoPosH, shadow);
	
	if(color.a < 0.1f)
        discard;
	
    return color;
}

float4 AlphaClipNormalDepthPS(MeshOutput input) : SV_TARGET
{
    input.normalV = normalize(input.normalV);
	
    float4 color = DiffuseMap.Sample(LinearSampler, input.uv);
    if (color.a < 0.1f)
        discard;
	
    return float4(input.normalV, input.positionV.z);
}

technique11 Draw
{
	PASS_RS_VP(P0, NoCull, VS, AlphaClipPS)
};

technique11 Shadow
{
	PASS_RS_VP(P0, Depth, VS, AlphaClipPS)
};

technique11 NormalDepth
{
	PASS_RS_VP(P0, NoCull, VS, AlphaClipNormalDepthPS)
};