#include "00. Global.fx"
#include "00. Light.fx"
#include "00. Render.fx"

float ShockwaveSpeed <
    bool MaterialProperty = true;
    string UIName = "충격파 속도";
    float UIMin = 0.f;
    float UIMax = 3.f;
> = 0.8f;

bool RepeatShockwave <
    bool MaterialProperty = true;
    string UIName = "충격파 반복";
> = true;

float ShockwaveStartTime <
    bool MaterialProperty = true;
    string UIName = "1회 재생 시작 시간";
> = 0.f;

float RingInnerWidth <
    bool MaterialProperty = true;
    string UIName = "링 안쪽 두께";
    float UIMin = 0.001f;
    float UIMax = 0.5f;
> = 0.08f;

float RingOuterWidth <
    bool MaterialProperty = true;
    string UIName = "링 바깥쪽 두께";
    float UIMin = 0.001f;
    float UIMax = 0.5f;
> = 0.12f;

int RingCount <
    bool MaterialProperty = true;
    string UIName = "링 개수";
    int UIMin = 1;
    int UIMax = 8;
> = 1;

float FresnelStrength <
    bool MaterialProperty = true;
    string UIName = "프레넬 강도";
    float UIMin = 0.f;
    float UIMax = 1.f;
> = 0.35f;

float FresnelPower <
    bool MaterialProperty = true;
    string UIName = "프레넬 지수";
    float UIMin = 1.f;
    float UIMax = 10.f;
> = 4.f;

bool EnableFresnel <
    bool MaterialProperty = true;
    string UIName = "프레넬 사용";
> = true;

Texture2D ShockwaveNoiseMap <
    bool MaterialProperty = true;
    string UIName = "충격파 노이즈 텍스처";
>;

float NoiseStrength <
    bool MaterialProperty = true;
    string UIName = "노이즈 강도";
    float UIMin = 0.f;
    float UIMax = 0.2f;
> = 0.f;

float NoiseTiling <
    bool MaterialProperty = true;
    string UIName = "노이즈 타일링";
    float UIMin = 0.1f;
    float UIMax = 10.f;
> = 2.f;

float NoiseSpeed <
    bool MaterialProperty = true;
    string UIName = "노이즈 이동 속도";
    float UIMin = 0.f;
    float UIMax = 2.f;
> = 0.2f;

float DistortionStrength <
    bool MaterialProperty = true;
    string UIName = "디스토션 강도";
    float UIMin = 0.f;
    float UIMax = 1.f;
> = 1.f;

float DistortionOpacity <
    bool MaterialProperty = true;
    string UIName = "디스토션 투명도";
    float UIMin = 0.f;
    float UIMax = 1.f;
> = 1.f;

float4 PS_ShockwaveDistortion(MeshOutput input) : SV_TARGET
{
    // VS_Mesh가 넘겨준 뷰 공간 데이터를 사용한다.
    float3 viewNormal = normalize(input.normalV);
    float3 viewDirection = normalize(-input.positionV);

    // 픽셀별 시선 방향과 법선의 각도로 구의 방사형 위치를 구한다.
    // 화면 위치와 무관하게 정면은 0, 실루엣은 1에 가까워진다.
    float facing = saturate(dot(viewNormal, viewDirection));
    float radialPosition = sqrt(saturate(1.f - facing * facing));

    // 구형 메시 UV를 따라 움직이는 노이즈로 링의 진행 위치를 흔든다.
    // NoiseStrength가 0이면 텍스처를 지정하지 않아도 기존 충격파 모양을 유지한다.
    float2 noiseUv = input.uv * NoiseTiling + float2(Time * NoiseSpeed, 0.f);
    float noise = ShockwaveNoiseMap.Sample(LinearSampler, noiseUv).r * 2.f - 1.f;
    radialPosition = saturate(radialPosition + noise * NoiseStrength);

    // 반복 모드는 기존처럼 전역 시간을 사용한다.
    // 1회 모드는 재생을 요청한 현재 시간을 ShockwaveStartTime에 넣으면
    // 그 시점부터 링을 한 묶음만 재생하고 마지막 링이 지나간 뒤 종료한다.
    int ringCount = max(RingCount, 1);
    float waveTime = RepeatShockwave
        ? Time * ShockwaveSpeed
        : (Time - ShockwaveStartTime) * ShockwaveSpeed;
    float lastRingEndTime = 1.f + (float)(ringCount - 1) / ringCount;
    bool isOneShotActive = waveTime >= 0.f && waveTime <= lastRingEndTime;

    float ring = 0.f;
    for (int i = 0; i < ringCount; i++)
    {
        float ringDelay = (float)i / ringCount;
        float ringPosition = RepeatShockwave
            ? frac(waveTime + ringDelay)
            : waveTime - ringDelay;
        float ringActive = (RepeatShockwave || (ringPosition >= 0.f && ringPosition <= 1.f))
            ? 1.f
            : 0.f;
        float ringDistance = abs(radialPosition - ringPosition);
        float ringMask = 1.f - smoothstep(RingInnerWidth, RingOuterWidth, ringDistance);
        ring = max(ring, ringMask * ringActive);
    }

    // 충격파 링이 지나간 뒤에도 실루엣에 약한 디스토션이 남도록 프레넬을 더한다.
    // DistortionOpacity로 최종 디스토션 마스크의 투명도를 조절한다.
    float fresnel = EnableFresnel ? pow(1.f - facing, FresnelPower) : 0.f;
    float mask = saturate(max(ring, fresnel * FresnelStrength) * DistortionOpacity);
    mask *= (RepeatShockwave || isOneShotActive) ? 1.f : 0.f;
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

    // R16G16_FLOAT 디스토션 타깃은 (0.5, 0.5)를 오프셋이 없는 중립값으로 사용하므로,
    // 부호가 있는 UV 오프셋을 중립값을 기준으로 인코딩한다.
    float strength = saturate(DistortionStrength) * MaxDistortionOffset;
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
