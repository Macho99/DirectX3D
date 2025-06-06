#include "pch.h"
#include "Billboard.h"
#include "Material.h"
#include "Renderer.h"
#include "Camera.h"

Billboard::Billboard()
	: Super(ComponentType::Billboard)
{
	int32 vertexCount = MAX_BILLBOARD_COUNT * 4;
	int32 indexCount = MAX_BILLBOARD_COUNT * 6;

	_vertices.resize(vertexCount);
	_vertexBuffer = make_shared<VertexBuffer>();
	_vertexBuffer->Create(_vertices, 0, true);

	_indices.resize(indexCount);
	
	for (int32 i = 0; i < MAX_BILLBOARD_COUNT; i++)
	{
		_indices[i * 6 + 0] = i * 4 + 0;
		_indices[i * 6 + 1] = i * 4 + 1;
		_indices[i * 6 + 2] = i * 4 + 2;
		_indices[i * 6 + 3] = i * 4 + 2;
		_indices[i * 6 + 4] = i * 4 + 1;
		_indices[i * 6 + 5] = i * 4 + 3;
	}

	_indexBuffer = make_shared<IndexBuffer>();
	_indexBuffer->Create(_indices);
}

Billboard::~Billboard()
{
}

void Billboard::InnerRender(RenderTech renderTech)
{
	Super::InnerRender(renderTech);

	if (_prevCount != _drawCount)
	{
		_prevCount = _drawCount;

		D3D11_MAPPED_SUBRESOURCE subResource;
		DC->Map(_vertexBuffer->GetComPtr().Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &subResource);
		{
			memcpy(subResource.pData, _vertices.data(), sizeof(VertexBillboard) * _vertices.size());
		}
		DC->Unmap(_vertexBuffer->GetComPtr().Get(), 0);
	}

	Shader* shader = _material->GetShader();
	
	// IA
	_vertexBuffer->PushData();
	_indexBuffer->PushData();

	shader->DrawIndexed(renderTech, _pass, _drawCount * 6);
}

void Billboard::Add(Vec3 position, Vec2 scale)
{
	_vertices[_drawCount * 4 + 0].position = position;
	_vertices[_drawCount * 4 + 1].position = position;
	_vertices[_drawCount * 4 + 2].position = position;
	_vertices[_drawCount * 4 + 3].position = position;

	_vertices[_drawCount * 4 + 0].uv = Vec2(0, 1);
	_vertices[_drawCount * 4 + 1].uv = Vec2(0, 0);
	_vertices[_drawCount * 4 + 2].uv = Vec2(1, 1);
	_vertices[_drawCount * 4 + 3].uv = Vec2(1, 0);

	_vertices[_drawCount * 4 + 0].scale = scale;
	_vertices[_drawCount * 4 + 1].scale = scale;
	_vertices[_drawCount * 4 + 2].scale = scale;
	_vertices[_drawCount * 4 + 3].scale = scale;

	_drawCount++;
}
