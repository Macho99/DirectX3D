#pragma once
#include "Component.h"
class Renderer : public Component
{
	using Super = Component;
public:
	Renderer(ComponentType componentType);
	~Renderer();

	void SetPass(uint8 pass) { _pass = pass; }
	void SetMaterial(shared_ptr<Material> material) { _material = material; }

	virtual void Render();

protected:
	shared_ptr<Material> _material;
	uint8 _pass = 0;
};

