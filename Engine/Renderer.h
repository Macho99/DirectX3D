#pragma once
#include "Component.h"
#include "../NavBuild/NavTypes.h"

enum class RenderTech;

class Renderer : public Component
{
	using Super = Component;
public:
	Renderer(ComponentType componentType);
	~Renderer();

	void SetPass(uint8 pass) { _pass = pass; }
	virtual void SetMaterial(ResourceRef<Material> material);
	const ResourceRef<Material>& GetMaterial() const { return _material; }

	bool Render(RenderTech renderTech);
    bool CanRender(RenderTech renderTech);
    void SetBeforeRender(function<void(Material*)> func) { _beforeRender = func; }
    virtual bool OnGUI() override;
	virtual bool TryInitialize() { return true; }
    virtual void SubmitTriangles(const Bounds& explicitBounds, vector<InputTri>& tris) {}
    bool IsInstRenderer() const;

    template<typename Archive>
    void serialize(Archive& ar)
    {
        Super::serialize(ar);
        ar(CEREAL_NVP(_material), CEREAL_NVP(_pass));
    }

protected:
	virtual void InnerRender(RenderTech renderTech);
	virtual void OnMaterialChange(const Material* oldMaterial, const Material* newMaterial);

protected:
	uint8 _pass = 0;
    function<void(Material*)> _beforeRender = nullptr;

private:
	ResourceRef<Material> _material;
};

