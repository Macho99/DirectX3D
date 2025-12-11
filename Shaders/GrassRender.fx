#include "00. GrassCommon.fx"
#include "00. Global.fx"
#include "00. Render.fx"
#include "00. Light.fx"

struct VS_TO_GS
{
    float3 worldPos : POSITION;
};

StructuredBuffer<GrassBlade> CulledGrassBuffer;

VS_TO_GS VS(uint instanceID : SV_InstanceID)
{
    // 1. CS가 생성한 버퍼에서 현재 인스턴스 ID에 해당하는 풀잎 데이터를 읽어옵니다.
    GrassBlade blade = CulledGrassBuffer[instanceID];
    
    VS_TO_GS output;
    
    // 2. 월드 좌표계 위치를 GS로 그대로 전달합니다.
    output.worldPos = blade.position;
    
    return output;
}

[maxvertexcount(4)]
void GS(point VS_TO_GS input[1], inout TriangleStream<MeshOutput> stream)
{
    float3 bladeBasePos = input[0].worldPos; // 풀잎이 자라날 바닥 위치
    
    // --- 1. 빌보딩(Billboarding) 계산 ---
    // 카메라를 항상 바라보도록 쿼드의 방향을 설정합니다.
    
    // 카메라에서 풀잎으로 향하는 벡터 (Z축 정렬)
    float3 vecToCam = normalize(CamPos - bladeBasePos);
    
    // 월드 'Up' 벡터 (Y축)
    float3 worldUp = float3(0, 1, 0);
    
    // 빌보드의 'Right' 벡터 (X축)
    float3 bladeRight = normalize(cross(worldUp, vecToCam));
    
    // 빌보드의 'Up' 벡터 (Y축)
    float3 bladeUp = worldUp * 1;
    

    // --- 2. (선택) 바람 애니메이션 ---
    // CS와 동일한 로직으로 바람을 계산하되, 풀잎의 위쪽 정점에만 적용합니다.
    //float windStrength = sin(g_Time * 1.5 + bladeBasePos.x * 0.1) * 0.5;
    float3 windOffset = float3(0, 0, 0); // X축으로만 간단히 흔들림
    
    
    // --- 3. 4개의 정점 생성 (쿼드) ---
    MeshOutput v[4];
    
    // 0: Bottom-Left
    v[0].worldPosition = bladeBasePos - bladeRight;
    v[0].uv = float2(0, 1);
    
    // 1: Bottom-Right
    v[1].worldPosition = bladeBasePos + bladeRight;
    v[1].uv = float2(1, 1);
    
    // 2: Top-Left (바람 적용)
    v[2].worldPosition = bladeBasePos - bladeRight + bladeUp + windOffset;
    v[2].uv = float2(0, 0);

    // 3: Top-Right (바람 적용)
    v[3].worldPosition = bladeBasePos + bladeRight + bladeUp + windOffset;
    v[3].uv = float2(1, 0);

    // 빌보드의 법선 (조명용, 카메라를 향함)
    float3 billboardNormal = worldUp;

    // --- 4. 정점 스트림에 추가 ---
    // 4개의 정점을 뷰-프로젝션 변환하여 스트림에 추가합니다.
    
    [unroll]
    for (int i = 0; i < 4; ++i)
    {
        float4 worldPos = float4(v[i].worldPosition, 1.0f);
        
        v[i].position = mul(worldPos, VP);
        v[i].normal = billboardNormal;
        v[i].normalV = mul(v[i].normal, (float3x3) V);
        v[i].positionV = mul(worldPos, V);
        v[i].shadowPosH = mul(worldPos, ShadowTransform);
        v[i].ssaoPosH = mul(worldPos, VPT);
        
        stream.Append(v[i]);
    }
    
    // 삼각형 스트립(Triangle Strip)을 완성합니다.
    stream.RestartStrip();
}

float4 AlphaClipShadowPS(MeshOutput input) : SV_TARGET
{
    float4 litColor = DiffuseMap.Sample(LinearSampler, input.uv);
	
    if (litColor.a < 0.1f)
        discard;
	
    return litColor;
}

float4 AlphaClipPS(MeshOutput input) : SV_TARGET
{
    float4 litColor = DiffuseMap.Sample(LinearSampler, input.uv);
    if (litColor.a < 0.1f)
        discard;
    
    float shadow = CalcShadowFactor(ShadowMap, input.shadowPosH);
    float4 color = ComputeLight(input.normal, litColor, input.worldPosition, input.ssaoPosH, shadow);
	
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
    pass P0
    {
        SetRasterizerState(NoCull);
        SetVertexShader(CompileShader(vs_5_0, VS()));
        SetGeometryShader(CompileShader(gs_5_0, GS()));
        SetPixelShader(CompileShader(ps_5_0, AlphaClipPS()));
    }
};

technique11 Shadow
{
    pass P0
    {
        SetRasterizerState(Depth);
        SetVertexShader(CompileShader(vs_5_0, VS()));
        SetGeometryShader(CompileShader(gs_5_0, GS()));
        SetPixelShader(CompileShader(ps_5_0, AlphaClipShadowPS()));
    }
};

technique11 NormalDepth
{
    pass P0
    {
        SetRasterizerState(NoCull);
        SetVertexShader(CompileShader(vs_5_0, VS()));
        SetGeometryShader(CompileShader(gs_5_0, GS()));
        SetPixelShader(CompileShader(ps_5_0, AlphaClipNormalDepthPS()));
    }
};