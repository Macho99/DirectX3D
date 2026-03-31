#pragma once

#include "cereal/cereal.hpp"
#include "AssetId.h"

enum
{
	RESOURCE_TYPE_COUNT = static_cast<uint8>(ResourceType::End)
};

class ResourceBase
{
public:
    ResourceBase();
	ResourceBase(ResourceType type);
	virtual ~ResourceBase();

	ResourceType GetType() { return _type; }
    virtual int GetVersion() const { return 0; }

	void SetName(const wstring& name) { _name = name; }
	const wstring& GetName() { return _name; }
	AssetId GetID() { return _assetId; }
	void SetId(const AssetId& assetId) { _assetId = assetId; }
	virtual bool OnGUI(bool isReadOnly);
	virtual void OnMenu(bool isReadOnly);

    template<class Archive>
    void serialize(Archive& ar)
    {
		if (Archive::is_saving::value)
			_version = GetVersion();

		ar(CEREAL_NVP(_version));
    }

protected:
	virtual void Load(const wstring& path) { }
	virtual void Save(const wstring& path) { }

protected:
	int _version = 0;
	ResourceType _type = ResourceType::None;
	wstring _name;
	wstring _path;
	AssetId _assetId;
};

