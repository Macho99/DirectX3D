#include "00. Global.fx"
#include "00. Light.fx"
#include "00. Render.fx"

const float ReflectionStrength = 0.5f;
const float Roughness = 0.5f;
const float MaxThickness = 8.f;
const float MaxRayDistance = 300.f;
const int MaxSteps = 64;

#define SceneMap DiffuseMap
#define NormalDepthMap NormalMap
TextureCube CubeMap;
SamplerState samTriLinearSam
{
    Filter = MIN_MAG_MIP_LINEAR;
    AddressU = Wrap;
    AddressV = Wrap;
};


float DepthToViewZ(float depth)
{
    return P._43 / (depth - P._33);
}

struct HitTestResult
{
    bool hit;
    float2 uv;
};

HitTestResult HitTest(float3 rayPos)
{
    HitTestResult result;
    
    float4 clip = mul(float4(rayPos, 1.0f), P);
    float2 uv = clip.xy / max(clip.w, 1e-5f);
    uv = uv * 0.5f + 0.5f;
    uv.y = 1.0f - uv.y;
    result.uv = uv;
    
    if (any(uv < 0.0f) || any(uv > 1.0f))
    {
        result.hit = false;
        return result;
    }

    float sceneDepth = NormalDepthMap.SampleLevel(PointSampler, uv, 0).a;
    float sceneViewZ = DepthToViewZ(sceneDepth);
    //float rayDepth = clip.z / max(clip.w, 1e-5f);
        
    float diff = rayPos.z - sceneViewZ;
    
    result.hit = diff > 0.f && diff < MaxThickness;
    
    return result;
}

float3 BinarySearch(float3 rayPos, float3 reflDir, float stepDistance)
{
    float3 lo = rayPos - reflDir * stepDistance;
    float3 hi = rayPos;
    float3 mid = rayPos;
    HitTestResult hitResult;
    
    [unroll]
    for (int j = 0; j < 8; ++j)
    {
        float3 mid = (lo + hi) * 0.5f;
        hitResult = HitTest(mid);
        
        if (hitResult.hit)
            hi = mid; // 너무 깊음 → 되돌아가기
        else
            lo = mid; // 아직 안 들어감 → 더 전진
    } 
    
    float2 edgeFade = smoothstep(0.0f, 0.1f, hitResult.uv)
                            * smoothstep(1.0f, 0.9f, hitResult.uv);
    float fade = edgeFade.x * edgeFade.y;
    
    float3 worldReflDir = mul(reflDir, (float3x3) VInv);
    float3 skyColor = CubeMap.Sample(samTriLinearSam, worldReflDir).rgb;
    float3 sceneColor = SceneMap.Sample(LinearSampler, hitResult.uv).rgb;
    return lerp(sceneColor, skyColor, 1.0f - fade);
}

float3 ComputeSSR(float3 viewPos, float3 viewNormal, float3 viewDir)
{
    float3 reflDir = normalize(reflect(viewDir, viewNormal));

    float stepDistance = MaxRayDistance / max((float) MaxSteps, 1.0f);
    float3 rayPos = viewPos;

    [loop]
    for (int i = 0; i < MaxSteps; ++i)
    {
        rayPos += reflDir * stepDistance;
        HitTestResult hitResult = HitTest(rayPos);
        if (hitResult.hit)
        {
            return BinarySearch(rayPos, reflDir, stepDistance);
        }
    }

    float3 worldReflDir = mul(reflDir, (float3x3) VInv);
    return CubeMap.Sample(samTriLinearSam, worldReflDir).rgb;
}

float4 PS(MeshOutput input) : SV_TARGET
{
    float3 viewDir = normalize(input.positionV);
    float3 viewNormal = normalize(input.normalV);

    float3 reflected = ComputeSSR(input.positionV, viewNormal, viewDir);
    float fresnel = pow(1.0f - saturate(dot(-viewDir, viewNormal)), 5.0f);
    float reflectivity = saturate(ReflectionStrength * (1.0f - Roughness));
    
    float shadow = CalcCascadeShadowFactor(input.worldPosition, input.viewZ);
    float4 baseColor = ComputeLight(input.normal, Material.diffuse, input.worldPosition, input.ssaoPosH, shadow);
    float3 color = lerp(baseColor.rgb, reflected, saturate(reflectivity + fresnel));
    
    return float4(color, 1.0f);
}

technique11 Draw
{
	PASS_BS_VP(P0, AlphaBlend, VS_Mesh_NotInst, PS)
};