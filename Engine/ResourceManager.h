#pragma once

#include "ResourceBase.h"
#include "DirectoryWatcherWin.h"
#include "ThreadSafeQueue.h"
#include "FsEventDebouncer.h"
#include "AssetDatabase.h"

class Shader;
class Texture;
class Mesh;
class Material;
template<class T>
class SlotManager;

struct BrowserItem
{
	fs::path absPath;
	Guid guid;       // 파일이면 guid, 폴더면 invalid
	bool isFolder = false;
};

class ResourceManager
{
	DECLARE_SINGLE_WITH_CONSTRUCTOR(ResourceManager);
    ~ResourceManager();
public:
	void Init();
	void Update();
	void OnDestroy();

	template<typename T>
	shared_ptr<T> Load(const wstring& key, const wstring& path);

	template<typename T>
	bool Add(const wstring& key, shared_ptr<T> object);

	template<typename T>
	shared_ptr<T> Get(const wstring& key);

	shared_ptr<Texture> GetOrAddTexture(const wstring& key, const wstring& path);

	template<typename T>
	ResourceType GetResourceType();

private:
	void CreateDefaultMesh();
	void CreateRandomTexture();

private:
	using KeyObjMap = map<wstring/*key*/, shared_ptr<ResourceBase>>;
	array<KeyObjMap, RESOURCE_TYPE_COUNT> _resources;
    unique_ptr<SlotManager<ResourceBase>> _slotManager;

public:
    fs::path GetRootPath() const { return _root; }
	bool TryGetGuidByPath(const fs::path& absPath, OUT Guid& guid);
    bool TryGetPathByGuid(const Guid& guid, OUT fs::path& path);
    void SetOnFileEventCallback(function<void(const FsEvent&)> cb) { onFileEventCallback = cb; }

private:
	static wstring ToStr(FsAction fsAction);
	bool IsInterestingFile(const fs::path& path);

private:
	fs::path _root;

	DirectoryWatcherWin watcher;
	FsEventDebouncer debouncer;
	AssetDatabase assetDatabase;

	ThreadSafeQueue<FsEvent> eventThreadQueue;
	vector<FsEvent> pendingEvents;
	vector<FsEvent> readyEvents;

	function<void(const FsEvent&)> onFileEventCallback;
};

template<typename T>
shared_ptr<T>
ResourceManager::Load(const wstring& key, const wstring& path)
{
	auto objectType = GetResourceType<T>();
	KeyObjMap& keyObjMap = _resources[static_cast<uint8>(objectType)];

	auto findIt = keyObjMap.find(key);
	if (findIt != keyObjMap.end())
		return static_pointer_cast<T>(findIt->second);

	shared_ptr<T> object = make_shared<T>();
	object->Load(path);
	keyObjMap[key] = object;

	return object;
}

template<typename T>
bool ResourceManager::Add(const wstring& key, shared_ptr<T> object)
{
	ResourceType resourceType = GetResourceType<T>();
	KeyObjMap& keyObjMap = _resources[static_cast<uint8>(resourceType)];

	auto findIt = keyObjMap.find(key);
	if (findIt != keyObjMap.end())
		return false;

	keyObjMap[key] = object;
	return true;
}

template<typename T>
shared_ptr<T> ResourceManager::Get(const wstring& key)
{
	ResourceType resourceType = GetResourceType<T>();
	KeyObjMap& keyObjMap = _resources[static_cast<uint8>(resourceType)];

	auto findIt = keyObjMap.find(key);
	if (findIt != keyObjMap.end())
		return static_pointer_cast<T>(findIt->second);

	return nullptr;
}

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

