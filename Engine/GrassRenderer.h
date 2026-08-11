#pragma once
#include "Renderer.h"
#include "Event.h"

// --- DX11 리소스 전역 변수 ---
const UINT MAX_GRASS_COUNT = 2000000;
const UINT THREAD_GROUP_SIZE = 256;   // CSMain의 [numthreads(256, 1, 1)]와 일치
const UINT MAX_UV_COUNT = 30;

struct GrassData
{
    Vec3 position = { 0, 0, 0 };
    float padding; // 16바이트 정렬을 위해 패딩 추가
};

struct NearbyGrassData
{
    Matrix worldMatrix;
    Vec4 uvMinMax;
    Vec4 terrainColor;
};

struct DistantGrassData
{
    Vec3 position;
    Vec2 scale;
    Vec4 uvMinMax;
    Vec4 terrainColor;
};

struct GrassConstant
{
    UINT totalGrassCount; // 초기 풀 버퍼의 전체 개수
    float terrainWidth;
    float terrainDepth;
    UINT uvCount;
    Vec4 uvs[MAX_UV_COUNT];
    Vec4 worldFrustumPlanes[6];
};

struct DrawInstancedIndirectArgs
{
    UINT VertexCountPerInstance; // 예: 4 (쿼드) 또는 8 (입체 풀잎)
    UINT InstanceCount;          // CS가 이 값을 채워 넣습니다.
    UINT StartVertexLocation;    // 0
    UINT StartInstanceLocation;  // 0
};

template<typename T>
class ConstantBuffer;
class TessTerrain;

class GrassRenderer : public Renderer
{
    using Super = Renderer;
    DECLARE_COMPONENT(GrassRenderer)
public:
    GrassRenderer();
    GrassRenderer(ResourceRef<Shader> grassComputeShader, ComponentRef<TessTerrain> terrain, const AssetRef& uvAsset);
    ~GrassRenderer();

    void CreateInitGrassBufferFromTerrain(bool needRefresh = true);

    virtual void OnEnable() override;

    virtual bool OnGUI() override;
    virtual bool TryInitialize() override;


    template<typename Archive>
    void serialize(Archive& ar)
    {
        Super::serialize(ar);
        ar(
            CEREAL_NVP(_grassComputeShader),
            CEREAL_NVP(_terrain),
            CEREAL_NVP(_uvAsset)
        );
    }

protected:
    void InnerRender(RenderTech renderTech) override;

private:
    void UpdateGrass();

private:
    ResourceRef<Shader> _grassComputeShader; // 미리 로드된 셰이더
    ComPtr<ID3D11Buffer> _initGrassBuffer; // (SRV) CPU -> GPU, 모든 풀 위치
    ComPtr<ID3D11ShaderResourceView> _initGrassSRV;
    ComPtr<ID3DX11EffectShaderResourceVariable> _initGrassEffectBuffer;
    vector<GrassData> _initGrassData;

    ComPtr<ID3D11Buffer> _nearbyGrassBuffer;
    ComPtr<ID3D11UnorderedAccessView> _nearbyGrassUAV;
    ComPtr<ID3D11ShaderResourceView> _nearbyGrassSRV;
    ComPtr<ID3DX11EffectUnorderedAccessViewVariable> _nearbyGrassEffectBuffer;

    ComPtr<ID3D11Buffer> _distantGrassBuffer;
    ComPtr<ID3D11UnorderedAccessView> _distantGrassUAV;
    ComPtr<ID3D11ShaderResourceView> _distantGrassSRV;
    ComPtr<ID3DX11EffectUnorderedAccessViewVariable> _distantGrassEffectBuffer;

    ComPtr<ID3D11Buffer> _nearbyDrawBuffer; // 간접 드로우 인자 버퍼
    ComPtr<ID3D11Buffer> _distantDrawBuffer; // 간접 드로우 인자 버퍼

    AssetRef _uvAsset;
    GrassConstant _grassConstantData;
    shared_ptr<ConstantBuffer<GrassConstant>> _grassConstantBuffer;     // 상수 버퍼
    ComPtr<ID3DX11EffectConstantBuffer> _grassEffectBuffer;
    ComPtr<ID3DX11EffectShaderResourceVariable> _randomEffectBuffer;
    //ResourceRef<Texture> _randomTex;

    ComponentRef<TessTerrain> _terrain;
    ComPtr<ID3DX11EffectShaderResourceVariable> _layerMapArrayEffectBuffer;
    ComPtr<ID3DX11EffectShaderResourceVariable> _blendMapEffectBuffer;

    int prevFrameCount = -1;
    bool _initialized = false;

    Event<>::ScopedListener _onHeightmapChangedListener;
    bool _heightmapChanged = false;
};

