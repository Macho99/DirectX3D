#pragma once

#include "ResourceBase.h"
#include "DirectoryWatcherWin.h"
#include "ThreadSafeQueue.h"
#include "FsEventDebouncer.h"
#include "AssetDatabase.h"
#include "ResourceRef.h"
#include "AssetSlot.h"
#include "SubAssetMetaFile.h"

class Shader;
class Texture;
class Mesh;
class Material;
template<class T>
class SlotManager;

//struct BrowserItem
//{
//	fs::path absPath;
//	AssetId assetId;       // 파일이면 guid, 폴더면 invalid
//	bool isFolder = false;
//};

class ResourceManager
{
	DECLARE_SINGLE_WITH_CONSTRUCTOR(ResourceManager);
	~ResourceManager();
public:
	void Init();
	void Start();
	void Update();
	void OnDestroy();

private:
	void CreateDefaultMesh();
	void CreateRandomTexture();

public:
	fs::path GetRootPath() const { return _root; }
	bool TryGetAssetIdByPath(const fs::path& assetsPath, OUT AssetId& assetId) const { return assetDatabase.TryGetAssetIdByPath(L"..\\Assets\\" / assetsPath, assetId); }
	bool TryGetPathByAssetId(const AssetId& assetId, OUT fs::path& path)	const { return assetDatabase.TryGetPathByAssetId(assetId, path); }
	bool TryGetMetaByAssetId(const AssetId& assetId, OUT MetaFile*& out)	const { return assetDatabase.TryGetMetaByAssetId(assetId, out); }
	bool TryGetMetaByPath(const fs::path& absPath, OUT MetaFile*& out)		const { return assetDatabase.TryGetMetaByPath(absPath, out); }
	bool SearchAssetIdByPath(const fs::path& searchFolder, const wstring& fileName, OUT AssetId& assetId) const
	{
		return assetDatabase.SearchAssetIdByPath(searchFolder, fileName, OUT assetId);
	}

    void SaveAsset(const AssetId & assetId);

	AssetDatabase& GetAssetDatabase() { return assetDatabase; }
	AssetSlot& GetAssetSlot() { return _assetSlot; }

	template<typename T>
	bool TryGetResourceRefByAssetId(const AssetId& assetId, OUT ResourceRef<T>& resourceRef) const;

	template<typename T>
	ResourceRef<T> GetResourceRefByPath(const fs::path& assetsPath) const;

	template<typename T>
	ResourceRef<T> AllocateTempResource(unique_ptr<T> resource)
	{
		AssetRef assetRef = _assetSlot.Register(std::move(resource));
		return ResourceRef<T>(assetRef);
	}

	template<typename T>
	ResourceRef<T> AllocateTempResource()
	{
		AssetRef assetRef = _assetSlot.Register(make_unique<T>());
		return ResourceRef<T>(assetRef);
	}

	void SetOnFileEventCallback(function<void(const FsEvent&)> cb) { onFileEventCallback = cb; }

	unordered_map<string, unique_ptr<ResourceBase>>& GetEditorResources() { return _editorResources; }

	ResourceRef<Mesh> GetQuadMesh() const { return _quad; }
	ResourceRef<Mesh> GetCubeMesh() const { return _cube; }
	ResourceRef<Mesh> GetSphereMesh() const { return _sphere; }
	ResourceRef<Texture> GetRandomTexture() const { return _randomTexture; }

private:
	static wstring ToStr(FsAction fsAction);

private:
	unordered_map<string, unique_ptr<ResourceBase>> _editorResources; // 에디터 전용 리소스(예: 아이콘)

	fs::path _root;

	DirectoryWatcherWin watcher;
	FsEventDebouncer debouncer;
	AssetDatabase assetDatabase;

	ThreadSafeQueue<FsEvent> eventThreadQueue;
	vector<FsEvent> pendingEvents;
	vector<FsEvent> readyEvents;

	function<void(const FsEvent&)> onFileEventCallback;

	AssetSlot _assetSlot;

	ResourceRef<Mesh> _quad;
	ResourceRef<Mesh> _cube;
	ResourceRef<Mesh> _sphere;
	ResourceRef<Texture> _randomTexture;
};

/*
template<typename T>
ResourceType ResourceManager::GetResourceType()
{
	if (std::is_same_v<T, Texture>)
		return ResourceType::Texture;
	if (std::is_same_v<T, Mesh>)
		return ResourceType::Mesh;
	if (std::is_same_v<T, Material>)
		return ResourceType::Material;

	assert(false);
	return ResourceType::None;
}
*/

template<typename T>
bool ResourceManager::TryGetResourceRefByAssetId(const AssetId& assetId, OUT ResourceRef<T>& resourceRef) const
{
	MetaFile* metaFile;
	if (assetDatabase.TryGetMetaByAssetId(assetId, OUT metaFile) == false)
	{
		return false;
	}

	SubAssetMetaFile* subAssetMeta = dynamic_cast<SubAssetMetaFile*>(metaFile);
	if (subAssetMeta != nullptr)
	{
		AssetId subAssetId;
		if (subAssetMeta->TryGetSubAssetByType(ResourceTypeTrait<T>::value, OUT subAssetId))
		{
			resourceRef = ResourceRef<T>(subAssetId);
			return true;
		}
	}

	if (metaFile->GetResourceType() != ResourceTypeTrait<T>::value)
	{
		return false;
	}

	resourceRef = ResourceRef<T>(assetId);
	return true;
}


template<typename T>
ResourceRef<T> ResourceManager::GetResourceRefByPath(const fs::path& assetsPath) const
{
	AssetId assetId;
	if (assetDatabase.TryGetAssetIdByPath(L"..\\Assets\\" / assetsPath, OUT assetId) == false)
	{
		ASSERT(false, "absPath is not valid");
		return ResourceRef<T>();
	}

	ResourceRef<T> resourceRef;
	if (TryGetResourceRefByAssetId<T>(assetId, OUT resourceRef) == false)
	{
		ASSERT(false, "Failed to get ResourceRef by assetId");
		return ResourceRef<T>();
	}
	return resourceRef;
}