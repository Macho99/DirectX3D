#include "pch.h"
#include "GrassRenderer.h"
#include "MathUtils.h"
#include "Material.h"

GrassRenderer::GrassRenderer(shared_ptr<Shader> grassComputeShader)
    : _grassComputeShader(grassComputeShader), Renderer(ComponentType::GrassRenderer)
{
    CreateResources();
}

GrassRenderer::~GrassRenderer()
{
}

void GrassRenderer::InnerRender(RenderTech renderTech)
{
    Super::InnerRender(renderTech);

    if (prevFrameCount != TIME->GetTotalFrameCount())
    {
        UpdateGrass();
        prevFrameCount = TIME->GetTotalFrameCount();
    }
    DC->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);
    auto shader = _material->GetShader();
    shader->GetSRV("CulledGrassBuffer")->SetResource(_finalGrassSRV.Get());

    //shader->DrawIndexedInstanced(renderTech, 0, 1, 100);
    UINT techNum = shader->GetTechNum(renderTech);
    shader->BeginDraw(techNum, 0);
    DC->DrawInstancedIndirect(_indirectDrawBuffer.Get(), 0);
    shader->EndDraw(techNum, 0);
}

void GrassRenderer::CreateResources()
{
    // --- 1. 초기 풀 데이터 CPU에서 생성 ---
    vector<GrassData> grassData(MAX_GRASS_COUNT);
    for (UINT i = 0; i < MAX_GRASS_COUNT; ++i)
    {
        float x = MathUtils::Random(-100.f, 100.f);
        float z = MathUtils::Random(-100.f, 100.f);
        float y = 0.f; // 지형 높이에 맞게 조정 가능
        grassData[i].position = Vec3(x, y, z);
    }

    // --- 2. InitialGrassBuffer 생성 (SRV 전용) ---
    D3D11_BUFFER_DESC initialDesc = {};
    initialDesc.ByteWidth = sizeof(GrassData) * MAX_GRASS_COUNT;
    initialDesc.Usage = D3D11_USAGE_IMMUTABLE; // 한 번 쓰고 바뀌지 않음
    initialDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    initialDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
    initialDesc.StructureByteStride = sizeof(GrassData);

    D3D11_SUBRESOURCE_DATA initData = {};
    initData.pSysMem = grassData.data();
    HR(DEVICE->CreateBuffer(&initialDesc, &initData, _initGrassBuffer.GetAddressOf()));

    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = DXGI_FORMAT_UNKNOWN;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
    srvDesc.Buffer.FirstElement = 0;
    srvDesc.Buffer.NumElements = MAX_GRASS_COUNT;

    HR(DEVICE->CreateShaderResourceView(_initGrassBuffer.Get(), &srvDesc, _initGrassSRV.GetAddressOf()));

    // --- 3. FinalGrassBuffer 생성 (Append Buffer: UAV + SRV) ---
    D3D11_BUFFER_DESC finalDesc = {};
    finalDesc.ByteWidth = sizeof(GrassData) * MAX_GRASS_COUNT; // 최대 크기
    finalDesc.Usage = D3D11_USAGE_DEFAULT;
    finalDesc.BindFlags = D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE;
    finalDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
    finalDesc.StructureByteStride = sizeof(GrassData);

    HR(DEVICE->CreateBuffer(&finalDesc, nullptr, _finalGrassBuffer.GetAddressOf()));

    // UAV (Append Buffer로 생성)
    D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
    uavDesc.Format = DXGI_FORMAT_UNKNOWN;
    uavDesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
    uavDesc.Buffer.FirstElement = 0;
    uavDesc.Buffer.NumElements = MAX_GRASS_COUNT;
    uavDesc.Buffer.Flags = D3D11_BUFFER_UAV_FLAG_APPEND; // Append Buffer로 설정

    HR(DEVICE->CreateUnorderedAccessView(_finalGrassBuffer.Get(), &uavDesc, _finalGrassUAV.GetAddressOf()));

    // SRV (나중에 렌더링 파이프라인(VS)에서 읽기 위함)
    srvDesc.Buffer.NumElements = MAX_GRASS_COUNT;
    HR(DEVICE->CreateShaderResourceView(_finalGrassBuffer.Get(), &srvDesc, _finalGrassSRV.GetAddressOf()));

    // --- 4. IndirectDrawBuffer 생성 ---
    D3D11_BUFFER_DESC indirectDesc = {};
    indirectDesc.ByteWidth = sizeof(DrawInstancedIndirectArgs);
    indirectDesc.Usage = D3D11_USAGE_DEFAULT;
    indirectDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    indirectDesc.MiscFlags = D3D11_RESOURCE_MISC_DRAWINDIRECT_ARGS;

    // 초기값 설정 (InstanceCount는 0, VertexCountPerInstance는 풀잎 정점 수)
    DrawInstancedIndirectArgs args = {};
    args.VertexCountPerInstance = 1; // 예: 쿼드(4개 정점)
    args.InstanceCount = 0;          // CS가 매 프레임 이 값을 덮어씁니다.
    args.StartVertexLocation = 0;
    args.StartInstanceLocation = 0;

    D3D11_SUBRESOURCE_DATA argData = {};
    argData.pSysMem = &args;

    HR(DEVICE->CreateBuffer(&indirectDesc, &argData, _indirectDrawBuffer.GetAddressOf()));

    _grassConstantBuffer = make_shared<ConstantBuffer<GrassConstant>>();
    _grassConstantBuffer->Create();
    _grassEffectBuffer = _grassComputeShader->GetConstantBuffer("GrassConstant");

    _initGrassEffectBuffer = _grassComputeShader->GetSRV("Input");
    _finalGrassEffectBuffer = _grassComputeShader->GetUAV("Output");
}

void GrassRenderer::UpdateGrass()
{
    {
        _grassConstantData.totalGrassCount = MAX_GRASS_COUNT;
        _grassConstantBuffer->CopyData(_grassConstantData);
        _grassEffectBuffer->SetConstantBuffer(_grassConstantBuffer->GetComPtr().Get());
    }

    ID3D11UnorderedAccessView* nullUav[1] = { 0 };
    _initGrassEffectBuffer->SetResource(_initGrassSRV.Get());
    _finalGrassEffectBuffer->SetUnorderedAccessView(_finalGrassUAV.Get());

    UINT numThreadGroups = (MAX_GRASS_COUNT + THREAD_GROUP_SIZE - 1) / THREAD_GROUP_SIZE;
    _grassComputeShader->Dispatch(0, 0, numThreadGroups, 1, 1);

    DC->CopyStructureCount(_indirectDrawBuffer.Get(), offsetof(DrawInstancedIndirectArgs, InstanceCount), _finalGrassUAV.Get());
}
