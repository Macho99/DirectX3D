#include "pch.h"
#include "SnowBillboard.h"
#include "Material.h"
#include "Camera.h"
#include "MathUtils.h"
#include "OnGUIUtils.h"

SnowBillboard::SnowBillboard()
	: Super(StaticType)
{
}

SnowBillboard::~SnowBillboard()
{
}

bool SnowBillboard::TryInitialize()
{
    if (_initialized)
        return true;

    if (_drawCount <= 0 || _drawCount > MAX_BILLBOARD_COUNT)
        return false;

	if (_desc.extent == Vec3::Zero)
		return false;

	_desc.drawDistance = _desc.extent.z * 2.0f;

	const int32 vertexCount = _drawCount * 4;
	_vertices.resize(vertexCount);

	for (int32 i = 0; i < _drawCount * 4; i += 4)
	{
		Vec2 scale = MathUtils::RandomVec2(0.1f, 0.5f);

		Vec3 position;
		position.x = MathUtils::Random(-_desc.extent.x, _desc.extent.x);
		position.y = MathUtils::Random(-_desc.extent.y, _desc.extent.y);
		position.z = MathUtils::Random(-_desc.extent.z, _desc.extent.z);

		Vec2 random = MathUtils::RandomVec2(0.0f, 1.0f);

		_vertices[i + 0].position = position;
		_vertices[i + 1].position = position;
		_vertices[i + 2].position = position;
		_vertices[i + 3].position = position;

		_vertices[i + 0].uv = Vec2(0, 1);
		_vertices[i + 1].uv = Vec2(0, 0);
		_vertices[i + 2].uv = Vec2(1, 1);
		_vertices[i + 3].uv = Vec2(1, 0);

		_vertices[i + 0].scale = scale;
		_vertices[i + 1].scale = scale;
		_vertices[i + 2].scale = scale;
		_vertices[i + 3].scale = scale;

		_vertices[i + 0].random = random;
		_vertices[i + 1].random = random;
		_vertices[i + 2].random = random;
		_vertices[i + 3].random = random;
	}

	_vertexBuffer = make_shared<VertexBuffer>();
	_vertexBuffer->Create(_vertices, "SnowBillboardVB", 0);

	const int32 indexCount = _drawCount * 6;
	_indices.resize(indexCount);

	for (int32 i = 0; i < _drawCount; i++)
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

    _initialized = true;
	return true;
}

void SnowBillboard::InnerRender(RenderTech renderTech)
{
	// TODO: not Implemented
	assert(renderTech == RenderTech::Draw);

    if (!TryInitialize())
        return;

	Super::InnerRender(renderTech);

	_desc.origin = CUR_SCENE->GetMainCamera()->GetTransform()->GetPosition();
	_desc.time = _elapsedTime;
	_elapsedTime += DT;


	// Transform
	//auto world = GetTransform()->GetWorldMatrix();
	//shader->PushTransformData(TransformDesc{ world });

	// GlobalData
	//shader->PushGlobalData(Camera::S_MatView, Camera::S_MatProjection);

	auto shader = _material.Resolve()->GetShader();
	// SnowData
	shader->PushSnowData(_desc);

	// Light
	//_material->Update();

	// IA
	_vertexBuffer->PushData();
	_indexBuffer->PushData();

	shader->DrawIndexed(renderTech, _pass, _drawCount * 6);
}

void SnowBillboard::SetMaterial(ResourceRef<Material> material)
{
	Super::SetMaterial(material);
	material.Resolve()->SetCastShadow(false);
}

bool SnowBillboard::OnGUI()
{
    bool changed = false;
    changed |= Super::OnGUI();
    ImGui::Separator();
    changed |= OnGUIUtils::DrawColor("Color", &_desc.color);
    changed |= OnGUIUtils::DrawVec3("Velocity", &_desc.velocity, 0.1f);
    changed |= OnGUIUtils::DrawFloat("Draw Distance", &_desc.drawDistance, 0.1f, true);
    changed |= OnGUIUtils::DrawVec3("Origin", &_desc.origin, 0.1f);
    changed |= OnGUIUtils::DrawFloat("Turbulence", &_desc.turbulence, 0.1f);
    changed |= OnGUIUtils::DrawVec3("Extent", &_desc.extent, 0.1f, true);
    changed |= OnGUIUtils::DrawFloat("Time", &_desc.time, 0.1f);
	changed |= OnGUIUtils::DrawInt32("Draw Count", &_drawCount, 1.f, true);

    return changed;
}
