#pragma once
#include "pch.h"
#include "Renderer.h"

struct VertexBillboard
{
	Vec3 position;
	Vec2 uv;
	Vec2 scale;
};

#define MAX_BILLBOARD_COUNT 10000

class Billboard : public Renderer
{
	using Super = Renderer;

public:
	Billboard();
	~Billboard();

	void Render() override;
	void Add(Vec3 position, Vec2 scale);

	//void SetMaterial(shared_ptr<Material> material) { _material = material; }
	void SetPass(uint8 pass) { _pass = pass; }

private:
	vector<VertexBillboard> _vertices;
	vector<uint32> _indices;
	shared_ptr<VertexBuffer> _vertexBuffer;
	shared_ptr<IndexBuffer> _indexBuffer;

	int32 _drawCount = 0;
	int32 _prevCount = 0;
	//shared_ptr<Material> _material;

	uint8 _pass = 0;
};

