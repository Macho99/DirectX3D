#include "pch.h"
#include "GrassRenderer.h"
#include "MathUtils.h"
#include "Material.h"
#include "TessTerrain.h"
#include "Camera.h"

GrassRenderer::GrassRenderer(shared_ptr<Shader> grassComputeShader, TessTerrain* terrain)
    : _grassComputeShader(grassComputeShader), Renderer(ComponentType::GrassRenderer)
{
    CreateResources(terrain);
}

GrassRenderer::~GrassRenderer()
{
}

void GrassRenderer::CreateResources(TessTerrain* terrain)
{
    // --- 1. 초기 풀 데이터 CPU에서 생성 ---
    vector<GrassData> grassData(MAX_GRASS_COUNT);
    for (UINT i = 0; i < MAX_GRASS_COUNT; ++i)
    {
        float x = MathUtils::Random(-500.f, 500.f);
        float z = MathUtils::Random(-500.f, 500.f);
        float y = terrain->GetHeight(x, z);
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

    HR(DEVICE->CreateBuffer(&finalDesc, nullptr, _nearbyGrassBuffer.GetAddressOf()));
    HR(DEVICE->CreateBuffer(&finalDesc, nullptr, _distantGrassBuffer.GetAddressOf()));

    // UAV (Append Buffer로 생성)
    D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
    uavDesc.Format = DXGI_FORMAT_UNKNOWN;
    uavDesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
    uavDesc.Buffer.FirstElement = 0;
    uavDesc.Buffer.NumElements = MAX_GRASS_COUNT;
    uavDesc.Buffer.Flags = D3D11_BUFFER_UAV_FLAG_APPEND; // Append Buffer로 설정

    HR(DEVICE->CreateUnorderedAccessView(_nearbyGrassBuffer.Get(), &uavDesc, _nearbyGrassUAV.GetAddressOf()));
    HR(DEVICE->CreateUnorderedAccessView(_distantGrassBuffer.Get(), &uavDesc, _distantGrassUAV.GetAddressOf()));

    // SRV (나중에 렌더링 파이프라인(VS)에서 읽기 위함)
    srvDesc.Buffer.NumElements = MAX_GRASS_COUNT;
    HR(DEVICE->CreateShaderResourceView(_nearbyGrassBuffer.Get(), &srvDesc, _nearbyGrassSRV.GetAddressOf()));
    HR(DEVICE->CreateShaderResourceView(_distantGrassBuffer.Get(), &srvDesc, _distantGrassSRV.GetAddressOf()));

    // --- 4. IndirectDrawBuffer 생성 ---
    D3D11_BUFFER_DESC indirectDesc = {};
    indirectDesc.ByteWidth = sizeof(DrawInstancedIndirectArgs);
    indirectDesc.Usage = D3D11_USAGE_DEFAULT;
    indirectDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    indirectDesc.MiscFlags = D3D11_RESOURCE_MISC_DRAWINDIRECT_ARGS;

    // 초기값 설정 (InstanceCount는 0, VertexCountPerInstance는 풀잎 정점 수)
    DrawInstancedIndirectArgs args = {};
    args.VertexCountPerInstance = 4;
    args.InstanceCount = 0;          // CS가 매 프레임 이 값을 덮어씁니다.
    args.StartVertexLocation = 0;
    args.StartInstanceLocation = 0;

    D3D11_SUBRESOURCE_DATA argData = {};
    argData.pSysMem = &args;

    HR(DEVICE->CreateBuffer(&indirectDesc, &argData, _nearbyDrawBuffer.GetAddressOf()));
    args.VertexCountPerInstance = 1;
    HR(DEVICE->CreateBuffer(&indirectDesc, &argData, _distantDrawBuffer.GetAddressOf()));

    _grassConstantBuffer = make_shared<ConstantBuffer<GrassConstant>>();
    _grassConstantBuffer->Create();
    _grassEffectBuffer = _grassComputeShader->GetConstantBuffer("GrassConstant");
    _randomEffectBuffer = _grassComputeShader->GetSRV("RandomMap");
    _randomTex = RESOURCES->Get<Texture>(L"RandomTex");

    _initGrassEffectBuffer = _grassComputeShader->GetSRV("Input");
    _nearbyGrassEffectBuffer = _grassComputeShader->GetUAV("NearbyOutput");
    _distantGrassEffectBuffer = _grassComputeShader->GetUAV("DistantOutput");
}

void GrassRenderer::UpdateGrass()
{
    {
        _grassConstantData.totalGrassCount = MAX_GRASS_COUNT;

        Matrix viewProj = Camera::S_MatView * Camera::S_MatProjection;
        MathUtils::ExtractFrustumPlanes(_grassConstantData.worldFrustumPlanes, viewProj);

        _grassConstantBuffer->CopyData(_grassConstantData);
        _grassEffectBuffer->SetConstantBuffer(_grassConstantBuffer->GetComPtr().Get());
    }
    _randomEffectBuffer->SetResource(_randomTex->GetComPtr().Get());
    _grassComputeShader->PushGlobalData(Camera::S_MatView, Camera::S_MatProjection);
    _initGrassEffectBuffer->SetResource(_initGrassSRV.Get());

    {
        UINT initCounts[2] = { 0, 0 };
        ID3D11UnorderedAccessView* uavs[] = {
            _nearbyGrassUAV.Get(),   // u0
            _distantGrassUAV.Get()   // u1
        };
        // 한 번에 모두 설정
        DC->CSSetUnorderedAccessViews(0, 2, uavs, initCounts);
    }

    _nearbyGrassEffectBuffer->SetUnorderedAccessView(_nearbyGrassUAV.Get());
    _distantGrassEffectBuffer->SetUnorderedAccessView(_distantGrassUAV.Get());

    UINT numThreadGroups = (MAX_GRASS_COUNT + THREAD_GROUP_SIZE - 1) / THREAD_GROUP_SIZE;
    _grassComputeShader->Dispatch(0, 0, numThreadGroups, 1, 1);

    {
        UINT initCounts[2] = { 0, 0 };
        ID3D11UnorderedAccessView* uavs[] = {
            0,
            0
        };
        // 한 번에 모두 설정
        DC->CSSetUnorderedAccessViews(0, 2, uavs, initCounts);
    }

    DC->CopyStructureCount(_distantDrawBuffer.Get(), offsetof(DrawInstancedIndirectArgs, InstanceCount), _distantGrassUAV.Get());
    DC->CopyStructureCount(_nearbyDrawBuffer.Get(), offsetof(DrawInstancedIndirectArgs, InstanceCount), _nearbyGrassUAV.Get());
}

void GrassRenderer::InnerRender(RenderTech renderTech)
{
    Super::InnerRender(renderTech);

    if (prevFrameCount != TIME->GetTotalFrameCount())
    {
        UpdateGrass();
        prevFrameCount = TIME->GetTotalFrameCount();
    }
    auto shader = _material->GetShader();

    UINT techNum = shader->GetTechNum(renderTech);
    {
        DC->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
        shader->GetSRV("NearbyGrassBuffer")->SetResource(_nearbyGrassSRV.Get());
        shader->BeginDraw(techNum, 0);
        DC->DrawInstancedIndirect(_nearbyDrawBuffer.Get(), 0);
        shader->EndDraw(techNum, 0);
    }

    if(renderTech == RenderTech::Draw)
    {
        DC->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);
        shader->GetSRV("DistantGrassBuffer")->SetResource(_distantGrassSRV.Get());
        shader->BeginDraw(techNum, 1);
        DC->DrawInstancedIndirect(_distantDrawBuffer.Get(), 0);
        shader->EndDraw(techNum, 1);
    }
}
