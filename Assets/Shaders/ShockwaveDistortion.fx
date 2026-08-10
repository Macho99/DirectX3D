#include "00. Global.fx"
#include "00. Light.fx"
#include "00. Render.fx"

// 충격파 링이 구의 중심에서 외곽까지 퍼져 나가는 속도
static const float ShockwaveSpeed = 0.8f;
// 안쪽 경계는 선명하게, 바깥쪽 경계는 부드럽게 감쇠시키기 위한 링 두께
static const float RingInnerWidth = 0.08f;
static const float RingOuterWidth = 0.12f;

float4 PS_ShockwaveDistortion(MeshOutput input) : SV_TARGET
{
    // VS_Mesh가 넘겨준 뷰 공간 데이터를 사용한다.
    float3 viewNormal = normalize(input.normalV);
    float3 viewDirection = normalize(-input.positionV);

    // 픽셀별 시선 방향과 법선의 각도로 구의 방사형 위치를 구한다.
    // 화면 위치와 무관하게 정면은 0, 실루엣은 1에 가까워진다.
    float facing = saturate(dot(viewNormal, viewDirection));
    float radialPosition = sqrt(saturate(1.f - facing * facing));

    // 시간에 따라 구의 정면에서 실루엣으로 반복해서 확장되는 링을 만든다.
    float wavePosition = frac(Time * ShockwaveSpeed);
    float ringDistance = abs(radialPosition - wavePosition);
    float ring = 1.f - smoothstep(RingInnerWidth, RingOuterWidth, ringDistance);

    // 충격파 링이 지나간 뒤에도 실루엣에 약한 디스토션이 남도록 프레넬을 더한다.
    // Material.diffuse.a로 최종 디스토션 마스크의 투명도를 조절한다.
    float fresnel = pow(1.f - facing, 4.f);
    float mask = saturate(max(ring, fresnel * 0.35f) * Material.diffuse.a);
    if (mask < 0.001f)
        discard;

    // 구의 법선을 화면 바깥쪽으로 뻗는 디스토션 방향으로 변환한다.
    // 화면 비율에 따라 X축을 보정하여 디스토션 모양이 원형으로 유지되게 한다.
    float aspectRatio = P._22 / max(P._11, 0.00001f);
    float2 pixelDirection = float2(
        viewNormal.x,
        -viewNormal.y);
    float directionLength = length(pixelDirection);
    if (directionLength < 0.00001f)
        discard;

    pixelDirection /= directionLength;
    float2 uvDirection = float2(pixelDirection.x / aspectRatio, pixelDirection.y);

    // Material.diffuse.x를 0~1 범위의 디스토션 강도로 사용한다.
    // R16G16_FLOAT 디스토션 타깃은 (0.5, 0.5)를 오프셋이 없는 중립값으로 사용하므로,
    // 부호가 있는 UV 오프셋을 중립값을 기준으로 인코딩한다.
    float strength = saturate(Material.diffuse.x) * MaxDistortionOffset;
    float2 offset = uvDirection * strength;
    float2 encodedOffset = saturate(offset / (MaxDistortionOffset * 2.f) + 0.5f);

    return float4(encodedOffset, 0.f, mask);
}

technique11 Distortion
{
    pass P0
    {
        // 0.5 중립 배경을 유지하면서 링과 프레넬 마스크가 적용되도록 알파 블렌딩한다.
        SetBlendState(AlphaBlend, float4(0.f, 0.f, 0.f, 0.f), 0xFF);
        SetDepthStencilState(NoDepthWrites, 0);
        SetVertexShader(CompileShader(vs_5_0, VS_Mesh()));
        SetPixelShader(CompileShader(ps_5_0, PS_ShockwaveDistortion()));
    }
};
