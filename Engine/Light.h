#pragma once
#include "Component.h"

class Camera;

class Light : public Component
{
	using Super = Component;

public:
	Light();
	virtual ~Light();

	virtual void Update();

public:
	LightDesc& GetLightDesc() { return _desc; }

	void SetLightDesc(LightDesc& desc) { _desc = desc; }
	void SetAmbient(const Color& color) { _desc.ambient = color; }
	void SetDiffuse(const Color& color) { _desc.diffuse = color; }
	void SetSpecular(const Color& color) { _desc.specular = color; }
	void SetEmissive(const Color& color) { _desc.emissive = color; }
	void SetLightDirection(Vec3 direction) { _desc.direction = direction; }

public:
	void SetVPMatrix(Matrix matView, Matrix matProjection, int cascadeIdx);
	static Matrix S_MatView;
	static Matrix S_MatProjection;
	static ShadowDesc S_ShadowData;

private:
	LightDesc _desc;
};

