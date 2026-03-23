#include "00. Global.fx"
#include "00. Render.fx"
#include "00. Light.fx"
#include "00. GrassCommon.fx"

struct VS_TO_GS
{
    float3 worldPos : POSITION;
    float2 scale : SCALE;
    float4 uvMinMax : UV_MIN_MAX;
    float4 terrainColor : COLOR0;
};

struct GrassOutput
{
    float4 position : SV_POSITION;
    float3 worldPosition : POSITION1;
    float2 uv : TEXCOORD;
    float3 normal : NORMAL;
    float3 tangent : TANGENT;
    float viewZ : VIEW_Z;
    float4 ssaoPosH : TEXCOORD1;
    float3 normalV : NORMAL_V;
    float3 positionV : POSITION_V;
    float4 terrainColor : COLOR0;
};

StructuredBuffer<NearbyGrassData> NearbyGrassBuffer;
StructuredBuffer<DistantGrassData> DistantGrassBuffer;

VS_TO_GS DistantVS(uint instanceID : SV_InstanceID)
{
    // 1. CS가 생성한 버퍼에서 현재 인스턴스 ID에 해당하는 풀잎 데이터를 읽어옵니다.
    DistantGrassData blade = DistantGrassBuffer[instanceID];
    
    VS_TO_GS output;
    
    // 2. 월드 좌표계 위치를 GS로 그대로 전달합니다.
    output.worldPos = blade.position;
    output.scale = blade.scale;
    output.uvMinMax = blade.uvMinMax;
    output.terrainColor = blade.terrainColor;
    
    return output;
}

[maxvertexcount(4)]
void GS(point VS_TO_GS input[1], inout TriangleStream<GrassOutput> stream)
{
    float3 bladeBasePos = input[0].worldPos; // 풀잎이 자라날 바닥 위치
    float2 scale = input[0].scale; // 풀잎의 스케일 (너비, 높이)
    float4 uv = input[0].uvMinMax;
    
    // --- 1. 빌보딩(Billboarding) 계산 ---
    // 카메라를 항상 바라보도록 쿼드의 방향을 설정합니다.
    
    // 카메라에서 풀잎으로 향하는 벡터 (Z축 정렬)
    float3 vecToCam = normalize(CamPos - bladeBasePos);
    
    // 월드 'Up' 벡터 (Y축)
    float3 worldUp = float3(0, 1, 0);
    
    // 빌보드의 'Right' 벡터 (X축)
    float3 bladeRight = normalize(cross(worldUp, vecToCam));
    bladeRight *= scale.x * 0.5f;
    
    // 빌보드의 'Up' 벡터 (Y축)
    float3 bladeUp = worldUp * scale.y;
    
    GrassOutput v[4];
    
    // 0: Bottom-Left
    v[0].worldPosition = bladeBasePos - bladeRight;
    v[0].worldPosition += CalculateWindOffset(v[0].worldPosition, 0);
    v[0].uv = float2(uv.x, uv.y);
    
    // 1: Bottom-Right
    v[1].worldPosition = bladeBasePos + bladeRight;
    v[1].worldPosition += CalculateWindOffset(v[1].worldPosition, 0);
    v[1].uv = float2(uv.z, uv.y); // (1,1)
    
    // 2: Top-Left (바람 적용)
    v[2].worldPosition = bladeBasePos - bladeRight + bladeUp;
    v[2].worldPosition += CalculateWindOffset(v[2].worldPosition, bladeUp.y);
    v[2].uv = float2(uv.x, uv.w); // (0,0)

    // 3: Top-Right (바람 적용)
    v[3].worldPosition = bladeBasePos + bladeRight + bladeUp;
    v[3].worldPosition += CalculateWindOffset(v[3].worldPosition, bladeUp.y);
    v[3].uv = float2(uv.z, uv.w); // (1,0)

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
        v[i].viewZ = v[i].positionV.z;
        v[i].ssaoPosH = mul(worldPos, VPT);
        v[i].terrainColor = input[0].terrainColor;
        
        stream.Append(v[i]);
    }
    
    // 삼각형 스트립(Triangle Strip)을 완성합니다.
    stream.RestartStrip();
}

GrassOutput NearbyVS(uint vertexID : SV_VertexID, uint instanceID : SV_InstanceID)
{
    NearbyGrassData blade = NearbyGrassBuffer[instanceID];
    float4 uv = blade.uvMinMax;
    
    float3 quadOffset[4] =
    {
        float3(-0.5f, 0.0f, 0.0f),
        float3(0.5f, 0.0f, 0.0f),
        float3(-0.5f, 1.0f, 0.0f),
        float3(0.5f, 1.0f, 0.0f)
    };

    // 3. 텍스처 좌표 정의
    float2 texCoords[4] =
    {
        float2(uv.x, uv.y),
        float2(uv.z, uv.y),
        float2(uv.x, uv.w),
        float2(uv.z, uv.w) 
    };
        
    GrassOutput output;
    float3 localPosition = quadOffset[vertexID];
    
    output.worldPosition = mul(blade.worldMatrix, float4(localPosition, 1.0f)).xyz;
    output.worldPosition += CalculateWindOffset(output.worldPosition, localPosition.y);
    output.uv = texCoords[vertexID];
    
    float4 worldPos = float4(output.worldPosition, 1.0f);
        
    output.position = mul(worldPos, VP);
    output.normal = float3(0, 1, 0);
    output.normalV = mul(output.normal, (float3x3) V);
    output.positionV = mul(worldPos, V);
    output.viewZ = output.positionV.z;
    output.ssaoPosH = mul(worldPos, VPT);
    output.terrainColor = blade.terrainColor;
    
    return output;
}

void AlphaClipShadowPS(GrassOutput input)
{
    float4 litColor = DiffuseMap.Sample(LinearSampler, input.uv);
	
    if (litColor.a < 0.1f)
        discard;
}

float4 AlphaClipPS(GrassOutput input) : SV_TARGET
{
    float4 litColor = DiffuseMap.Sample(LinearSampler, input.uv);
    if (litColor.a < 0.1f)
        discard;
    
    float shadow = CalcCascadeShadowFactor(input.worldPosition, input.viewZ);
    float4 color = ComputeLight(input.normal, litColor, input.worldPosition, input.ssaoPosH, shadow);
    color = lerp(color, input.terrainColor, 0.4f);
    
    return color;
}

float4 AlphaClipNormalDepthPS(GrassOutput input) : SV_TARGET
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
        SetVertexShader(CompileShader(vs_5_0, NearbyVS()));
        SetPixelShader(CompileShader(ps_5_0, AlphaClipPS()));
    }
    pass P1
    {
        SetRasterizerState(NoCull);
        SetVertexShader(CompileShader(vs_5_0, DistantVS()));
        SetGeometryShader(CompileShader(gs_5_0, GS()));
        SetPixelShader(CompileShader(ps_5_0, AlphaClipPS()));
    }
};

technique11 Shadow
{
    pass P0
    {
        SetRasterizerState(DepthNoCull);
        SetVertexShader(CompileShader(vs_5_0, NearbyVS()));
        SetPixelShader(CompileShader(ps_5_0, AlphaClipShadowPS()));
    }
    pass P1
    {
        SetRasterizerState(DepthNoCull);
        SetVertexShader(CompileShader(vs_5_0, DistantVS()));
        SetGeometryShader(CompileShader(gs_5_0, GS()));
        SetPixelShader(CompileShader(ps_5_0, AlphaClipShadowPS()));
    }
};

technique11 NormalDepth
{
    pass P0
    {
        SetRasterizerState(NoCull);
        SetVertexShader(CompileShader(vs_5_0, NearbyVS()));
        SetPixelShader(CompileShader(ps_5_0, AlphaClipNormalDepthPS()));
    }
    pass P1
    {
        SetRasterizerState(NoCull);
        SetVertexShader(CompileShader(vs_5_0, DistantVS()));
        SetGeometryShader(CompileShader(gs_5_0, GS()));
        SetPixelShader(CompileShader(ps_5_0, AlphaClipNormalDepthPS()));
    }
};