#pragma once
#include "pch.h"
#include "Component.h"

struct VertexSnow
{
	Vec3 position;
	Vec2 uv;
	Vec2 scale;
	Vec2 random;
};

#define MAX_BILLBOARD_COUNT 10000

class SnowBillboard : public Component
{
	using Super = Component;

public:
	SnowBillboard(Vec3 extent, int32 drawCount = 100);
	~SnowBillboard();

	void Update() override;

	void SetMaterial(shared_ptr<Material> material) { _material = material; }
	void SetPass(uint8 pass) { _pass = pass; }

private:
	vector<VertexSnow> _vertices;
	vector<uint32> _indices;
	shared_ptr<VertexBuffer> _vertexBuffer;
	shared_ptr<IndexBuffer> _indexBuffer;

	int32 _drawCount = 0;
	shared_ptr<Material> _material;

	uint8 _pass = 0;

	SnowBillboardDesc _desc;
	float _elapsedTime = 0.f;
};

