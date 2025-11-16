#pragma once
#include "Renderer.h"

struct GrassData
{
    Vec3 position = { 0, 0, 0 };
};

struct GrassConstant
{
    Matrix VP;
    Vec3 camPos;
    float time;
    UINT totalGrassCount; // 초기 풀 버퍼의 전체 개수
    Vec3 padding;
};

struct DrawInstancedIndirectArgs
{
    UINT VertexCountPerInstance; // 예: 4 (쿼드) 또는 8 (입체 풀잎)
    UINT InstanceCount;          // CS가 이 값을 채워 넣습니다.
    UINT StartVertexLocation;    // 0
    UINT StartInstanceLocation;  // 0
};

// --- DX11 리소스 전역 변수 ---
const UINT MAX_GRASS_COUNT = 1000000; // 최대 100만 개의 풀잎 (예시)
const UINT THREAD_GROUP_SIZE = 256;   // CSMain의 [numthreads(256, 1, 1)]와 일치

template<typename T>
class ConstantBuffer;

class GrassRenderer : public Renderer
{
    using Super = Renderer;

protected:
    void InnerRender(RenderTech renderTech) override;

private:
    void CreateResources();
    void UpdateGrass();

private:
    shared_ptr<Shader> _grassComputeShader; // 미리 로드된 셰이더
    ComPtr<ID3D11Buffer> _initGrassBuffer; // (SRV) CPU -> GPU, 모든 풀 위치
    ComPtr<ID3D11ShaderResourceView> _initGrassSRV;
    ComPtr<ID3DX11EffectShaderResourceVariable> _initGrassEffectBuffer;

    ComPtr<ID3D11Buffer> _finalGrassBuffer;   // (UAV + SRV) CS -> VS, 컬링된 풀
    ComPtr<ID3D11UnorderedAccessView> _finalGrassUAV;
    ComPtr<ID3D11ShaderResourceView> _finalGrassSRV;
    ComPtr<ID3DX11EffectUnorderedAccessViewVariable> _finalGrassEffectBuffer;

    ComPtr<ID3D11Buffer> _indirectDrawBuffer; // 간접 드로우 인자 버퍼

    GrassConstant _grassConstantsData;
    shared_ptr<ConstantBuffer<GrassConstant>> _grassConstantBuffer;     // 상수 버퍼
    ComPtr<ID3DX11EffectConstantBuffer> _grassEffectBuffer;
};

