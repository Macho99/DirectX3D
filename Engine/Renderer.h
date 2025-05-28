#pragma once
#include "Component.h"
enum class RenderTech;

class Renderer : public Component
{
	using Super = Component;
public:
	Renderer(ComponentType componentType);
	~Renderer();

	void SetPass(uint8 pass) { _pass = pass; }
	virtual void SetMaterial(shared_ptr<Material> material) { _material = material; }
	shared_ptr<Material> GetMaterial() { return _material; }

	bool Render(RenderTech renderTech);

protected:
	virtual void InnerRender(RenderTech renderTech);
	shared_ptr<Material> _material;
	uint8 _pass = 0;
};

