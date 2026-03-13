#pragma once
#include "pch.h"
#include "Renderer.h"

struct VertexSnow
{
	Vec3 position;
	Vec2 uv;
	Vec2 scale;
	Vec2 random;
};

#define MAX_BILLBOARD_COUNT 10000

class SnowBillboard : public Renderer
{
	using Super = Renderer;

public:
	static constexpr ComponentType StaticType = ComponentType::SnowBillboard;
	SnowBillboard();
	~SnowBillboard();

    void SetDrawCount(int32 count) { _drawCount = count; }
    void SetExtent(Vec3 extent) { _desc.extent = extent; }

	void InnerRender(RenderTech renderTech) override;

	//void SetMaterial(shared_ptr<Material> material) { _material = material; }
	void SetPass(uint8 pass) { _pass = pass; }

	void SetMaterial(ResourceRef<Material> material) override;

    virtual bool OnGUI() override;

protected:
    virtual bool TryInitialize() override;

private:
	vector<VertexSnow> _vertices;
	vector<uint32> _indices;
	shared_ptr<VertexBuffer> _vertexBuffer;
	shared_ptr<IndexBuffer> _indexBuffer;

	int32 _drawCount = 0;
	//shared_ptr<Material> _material;

	uint8 _pass = 0;

	SnowBillboardDesc _desc;
	float _elapsedTime = 0.f;

    bool _initialized = false;
};

