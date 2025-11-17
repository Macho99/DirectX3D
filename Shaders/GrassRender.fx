#include "00. GrassCommon.fx"
#include "00. Global.fx"
#include "00. Render.fx"
#include "00. Light.fx"

struct VS_TO_GS
{
    float3 worldPos : POSITION;
};

StructuredBuffer<GrassBlade> g_CulledGrassBuffer : register(t0);

VS_TO_GS VSMain(uint instanceID : SV_InstanceID)
{
    // 1. CS가 생성한 버퍼에서 현재 인스턴스 ID에 해당하는 풀잎 데이터를 읽어옵니다.
    GrassBlade blade = g_CulledGrassBuffer[instanceID];
    
    VS_TO_GS output;
    
    // 2. 월드 좌표계 위치를 GS로 그대로 전달합니다.
    output.worldPos = blade.position;
    
    return output;
}

[maxvertexcount(4)]
void GSMain(point VS_TO_GS input[1], inout TriangleStream<MeshOutput> stream)
{
    float3 bladeBasePos = input[0].worldPos; // 풀잎이 자라날 바닥 위치
    
    // --- 1. 빌보딩(Billboarding) 계산 ---
    // 카메라를 항상 바라보도록 쿼드의 방향을 설정합니다.
    
    // 카메라에서 풀잎으로 향하는 벡터 (Z축 정렬)
    float3 vecToCam = normalize(g_CameraPosition.xyz - bladeBasePos);
    
    // 월드 'Up' 벡터 (Y축)
    float3 worldUp = float3(0, 1, 0);
    
    // 빌보드의 'Right' 벡터 (X축)
    float3 bladeRight = normalize(cross(worldUp, vecToCam)) * (g_GrassWidth * 0.5f);
    
    // 빌보드의 'Up' 벡터 (Y축)
    float3 bladeUp = worldUp * g_GrassHeight;
    

    // --- 2. (선택) 바람 애니메이션 ---
    // CS와 동일한 로직으로 바람을 계산하되, 풀잎의 위쪽 정점에만 적용합니다.
    float windStrength = sin(g_Time * 1.5 + bladeBasePos.x * 0.1) * 0.5;
    float3 windOffset = float3(windStrength, 0, 0); // X축으로만 간단히 흔들림
    
    
    // --- 3. 4개의 정점 생성 (쿼드) ---
    MeshOutput v[4];
    
    // 0: Bottom-Left
    v[0].worldPos = bladeBasePos - bladeRight;
    v[0].texCoord = float2(0, 1);
    
    // 1: Bottom-Right
    v[1].worldPos = bladeBasePos + bladeRight;
    v[1].texCoord = float2(1, 1);
    
    // 2: Top-Left (바람 적용)
    v[2].worldPos = bladeBasePos - bladeRight + bladeUp + windOffset;
    v[2].texCoord = float2(0, 0);

    // 3: Top-Right (바람 적용)
    v[3].worldPos = bladeBasePos + bladeRight + bladeUp + windOffset;
    v[3].texCoord = float2(1, 0);

    // 빌보드의 법선 (조명용, 카메라를 향함)
    float3 billboardNormal = -vecToCam;

    // --- 4. 정점 스트림에 추가 ---
    // 4개의 정점을 뷰-프로젝션 변환하여 스트림에 추가합니다.
    
    [unroll]
    for (int i = 0; i < 4; ++i)
    {
        v[i].clipPos = mul(float4(v[i].worldPos, 1.0f), g_ViewProjection);
        v[i].normal = billboardNormal;
        stream.Append(v[i]);
    }
    
    // 삼각형 스트립(Triangle Strip)을 완성합니다.
    stream.RestartStrip();
}

float4 PSMain(MeshOutput input) : SV_TARGET
{
    // 1. 텍스처 샘플링
    float4 texColor = g_GrassTexture.Sample(g_Sampler, input.texCoord);
    
    // --- 2. 알파 클리핑 (Alpha Clipping) ---
    // ★핵심★: 텍스처의 알파(투명도) 값을 기준으로 픽셀을 버립니다.
    // 알파 값이 0.1보다 낮으면 (투명한 배경이면) 픽셀을 그리지 않습니다.
    clip(texColor.a - 0.1);
    
    // --- 3. 간단한 조명 ---
    // 태양 방향과 빌보드 법선을 내적(dot)하여 간단한 음영을 계산합니다.
    float light = saturate(dot(input.normal, -g_SunDirection)) * 0.7 + 0.3; // (Ambient 0.3)
    
    float3 finalColor = texColor.rgb * light;
    
    return float4(finalColor, 1.0f);
}