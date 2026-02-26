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
	virtual void SetMaterial(ResourceRef<Material> material) { _material = material; }
	ResourceRef<Material> GetMaterial() { return _material; }

	virtual bool Render(RenderTech renderTech);
    void SetBeforeRender(function<void(Material*)> func) { _beforeRender = func; }

protected:
	virtual void InnerRender(RenderTech renderTech);
	ResourceRef<Material> _material;
	uint8 _pass = 0;
    function<void(Material*)> _beforeRender = nullptr;
};

