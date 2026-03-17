#include "pch.h"
#include "ResourceManager.h"
#include "Texture.h"
#include "Shader.h"
#include "Mesh.h"
#include <filesystem>
#include "MathUtils.h"
#include "SlotManager.h"
#include "MetaStore.h"

ResourceManager::ResourceManager()
	:assetDatabase()
{
}

ResourceManager::~ResourceManager()
{
	watcher.Stop();
}

void ResourceManager::Init()
{
	_root = L"..\\Assets";
	if (!watcher.Start(_root, true, [&](const FsEvent& e)
		{
			// 워처 스레드: 절대 여기서 무거운 일 하지 말고 Push만!
			eventThreadQueue.Push(e);
		}))
	{
		DBG->LogW(L"Watcher start failed");
	}
	else
	{
		DBG->LogW(L"Watching: " + _root.wstring());
	}
	assetDatabase.ReconcileAndBuildFromMeta(_root);
}

void ResourceManager::Start()
{
	CreateDefaultMesh();
	CreateRandomTexture();
}

void ResourceManager::Update()
{
	pendingEvents.clear();
	eventThreadQueue.PopAll(pendingEvents);

	// 1) 필터 + 디바운서 입력
	for (auto& e : pendingEvents)
	{
		if (MetaStore::IsMetaFile(e.absPath))
			continue;

		debouncer.Push(e);
	}

	// 2) 디바운스 완료분 배출 (예: 300ms)
	readyEvents.clear();
	debouncer.PopReady(300, readyEvents);

	// 3) 최종 처리(여기서부터 메인 스레드)
	for (auto& e : readyEvents)
	{
		assetDatabase.OnFileEvent(e);
        if (onFileEventCallback)
        {
            onFileEventCallback(e);
        }
	}
}

void ResourceManager::OnDestroy()
{
    //for (auto& keyObjMap : _resources)
    //{
    //    keyObjMap.clear();
    //}

	_assetSlot.OnDestroy();
	_editorResources.clear();
}

//shared_ptr<Texture> ResourceManager::GetOrAddTexture(const wstring& key, const wstring& path)
//{
//	shared_ptr<Texture> texture = Get<Texture>(key);
//
//	if (filesystem::exists(filesystem::path(path)) == false)
//		return nullptr;
//
//	texture = Load<Texture>(key, path);
//
//	if (texture == nullptr)
//	{
//		texture = make_shared<Texture>();
//		texture->Load(path);
//		Add(key, texture);
//	}
//
//	return texture;
//}

void ResourceManager::CreateDefaultMesh()
{
	{
		_quad = AllocateTempResource(make_unique<Mesh>());
		_quad.Resolve()->CreateQuad();
	}
	{
		_cube = AllocateTempResource(make_unique<Mesh>());
		_cube.Resolve()->CreateCube();
	}
	{
		_sphere = AllocateTempResource(make_unique<Mesh>());
		_sphere.Resolve()->CreateSphere();
	}
}

void ResourceManager::CreateRandomTexture()
{
	unique_ptr<Texture> texture = make_unique<Texture>();
	// 
	// Create the random data.
	//
	vector<Vec4> randomValues(1024);

	for (int32 i = 0; i < 1024; ++i)
	{
		randomValues[i].x = MathUtils::Random(-1.0f, 1.0f);
		randomValues[i].y = MathUtils::Random(-1.0f, 1.0f);
		randomValues[i].z = MathUtils::Random(-1.0f, 1.0f);
		randomValues[i].w = MathUtils::Random(-1.0f, 1.0f);
	}

	D3D11_SUBRESOURCE_DATA initData;
	initData.pSysMem = randomValues.data();
	initData.SysMemPitch = 1024 * sizeof(XMFLOAT4);
	initData.SysMemSlicePitch = 0;

	//
	// Create the texture.
	//
	D3D11_TEXTURE1D_DESC texDesc;
	texDesc.Width = 1024;
	texDesc.MipLevels = 1;
	texDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	texDesc.Usage = D3D11_USAGE_IMMUTABLE;
	texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	texDesc.CPUAccessFlags = 0;
	texDesc.MiscFlags = 0;
	texDesc.ArraySize = 1;

	ComPtr<ID3D11Texture1D> randomTex;
	CHECK(DEVICE->CreateTexture1D(&texDesc, &initData, randomTex.GetAddressOf()));

	//
	// Create the resource view.
	//
	D3D11_SHADER_RESOURCE_VIEW_DESC viewDesc;
	viewDesc.Format = texDesc.Format;
	viewDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE1D;
	viewDesc.Texture1D.MipLevels = texDesc.MipLevels;
	viewDesc.Texture1D.MostDetailedMip = 0;

	ComPtr<ID3D11ShaderResourceView> randomTexSRV;
	DX_CREATE_SRV(randomTex.Get(), &viewDesc, randomTexSRV);
	texture->SetSRV(randomTexSRV);
    _randomTexture = AllocateTempResource(std::move(texture));
}

void ResourceManager::SaveAsset(const AssetId& assetId)
{
    fs::path assetPath;
	if (assetDatabase.TryGetPathByAssetId(assetId, OUT assetPath) == false)
	{
        ASSERT(false, "ResourceManager::SaveAsset: assetId not found in database: " + assetId.ToString());
		return;
	}

	_assetSlot.SaveAsset(assetId, assetPath);
}

Texture* ResourceManager::GetEditorTexture(string key, const fs::path& loadPath)
{
	auto it = _editorResources.find(key);

	if (it == _editorResources.end())
	{
		unique_ptr<Texture> texture = make_unique<Texture>();
		texture->Load(loadPath);

		{
			D3D11_SHADER_RESOURCE_VIEW_DESC desc = {};
			texture->GetComPtr()->GetDesc(&desc);

			if (desc.ViewDimension == D3D11_SRV_DIMENSION_TEXTURECUBE
				|| desc.ViewDimension == D3D11_SRV_DIMENSION_TEXTURECUBEARRAY)
			{
				texture->Load(L"..\\EditorResource\\ImageIcon.png");
			}
		}

		Texture* returnValue = texture.get();
		_editorResources[key] = std::move(texture);
		return returnValue;
	}
	else
	{
		return static_cast<Texture*>(it->second.get());
	}
}

Texture* ResourceManager::GetRandomTexture()
{
	return _randomTexture.Resolve();
}

wstring ResourceManager::ToStr(FsAction fsAction)
{
	switch (fsAction)
	{
		case FsAction::Added: return L"Added";
		case FsAction::Removed: return L"Removed";
		case FsAction::Modified: return L"Modified";
		case FsAction::Renamed: return L"Renamed";
		default: return L"?";
	}
}