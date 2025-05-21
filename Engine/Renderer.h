#pragma once
#include "Component.h"
class Renderer : public Component
{
	using Super = Component;
public:
	Renderer(ComponentType componentType);
	~Renderer();

	void SetPass(uint8 pass) { _pass = pass; }
	virtual void SetMaterial(shared_ptr<Material> material) { _material = material; }
	shared_ptr<Material> GetMaterial() { return _material; }

	bool Render(bool isShadowTech);

protected:
	virtual void InnerRender(bool isShadowTech);
	shared_ptr<Material> _material;
	uint8 _pass = 0;
};

