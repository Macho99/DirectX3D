#pragma once
#include "Component.h"
class Terrain : public Component
{
	using Super = Component;
public:
	Terrain();
	~Terrain();

	void Create(int32 sizeX, int32 sizeZ, ResourceRef<Material> material);
	int32 GetSizeX() { return _sizeX; }
	int32 GetSizeY() { return _sizeZ; }

	bool Pick(int32 screenX, int32 screenY, OUT Vec3& pickPos, OUT float& distance);

private:
	ResourceRef<Mesh> _mesh;
	int32 _sizeX = 0;
	int32 _sizeZ = 0;
};

