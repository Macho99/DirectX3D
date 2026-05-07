#include "00. Global.fx"
#include "00. Light.fx"
#include "00. Render.fx"

//const float ReflectionStrength = ;
//const float Roughness = 0.5f;
const float DepthThickness = 100.f;
const float MaxRayDistance = 300.f;
const int MaxSteps = 64;

#define SceneMap DiffuseMap
#define NormalDepthMap NormalMap

float3 ComputeSSR(float3 viewPos, float3 viewNormal, float3 viewDir)
{
    float3 reflDir = normalize(reflect(viewDir, viewNormal));

    float stepDistance = MaxRayDistance / max((float) MaxSteps, 1.0f);
    float3 rayPos = viewPos;

    [loop]
    for (int i = 0; i < MaxSteps; ++i)
    {
        rayPos += reflDir * stepDistance;

        float4 clip = mul(float4(rayPos, 1.0f), P);
        float2 uv = clip.xy / max(clip.w, 1e-5f);
        uv = uv * 0.5f + 0.5f;
        uv.y = 1.0f - uv.y;

        if (uv.x < 0.0f || uv.x > 1.0f || uv.y < 0.0f || uv.y > 1.0f)
            break;

        float sceneDepth = NormalDepthMap.SampleLevel(PointSampler, uv, 0).a;
        float rayDepth = clip.z / max(clip.w, 1e-5f);
        
        if (rayDepth > sceneDepth)
        {
            return SceneMap.Sample(LinearSampler, uv).rgb;
        }
    }

    return float3(0, 0, 0);
}

float4 PS(MeshOutput input) : SV_TARGET
{
    float3 viewDir = normalize(input.positionV);
    float3 viewNormal = normalize(input.normalV);

    float3 reflected = ComputeSSR(input.positionV, viewNormal, viewDir);
    //float fresnel = pow(1.0f - saturate(dot(-viewDir, viewNormal)), 5.0f);
    //float reflectivity = 0.5f; //= saturate(ReflectionStrength * (1.0f - Roughness));
    //
    //float shadow = CalcCascadeShadowFactor(input.worldPosition, input.viewZ);
    //float4 baseColor = ComputeLight(input.normal, Material.diffuse, input.worldPosition, input.ssaoPosH, shadow);
    float3 color = reflected; //lerp(baseColor.rgb, reflected, saturate(reflectivity + fresnel));
    
    return float4(color, 1.0f);
}

technique11 Draw
{
	PASS_BS_VP(P0, AlphaBlend, VS_Mesh_NotInst, PS)
};