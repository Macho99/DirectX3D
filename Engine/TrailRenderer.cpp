#include "pch.h"
#include "TrailRenderer.h"
#include "Material.h"

TrailRenderer::TrailRenderer()
    : Super(StaticType)
{
}

TrailRenderer::~TrailRenderer()
{
}

void TrailRenderer::SetPoints(const vector<TrailPoint>& points)
{
    _points = points;
    _dirty = true;
}

void TrailRenderer::AddPoint(const Vec3& base, const Vec3& tip)
{
    _points.push_back({ base, tip });
    _dirty = true;
}

bool TrailRenderer::SetPoint(uint32 index, const Vec3& base, const Vec3& tip)
{
    if (index >= _points.size())
        return false;

    _points[index] = { base, tip };
    _dirty = true;
    return true;
}

bool TrailRenderer::RemovePoint(uint32 index)
{
    if (index >= _points.size())
        return false;

    _points.erase(_points.begin() + index);
    _dirty = true;
    return true;
}

void TrailRenderer::ClearPoints()
{
    _points.clear();
    _dirty = true;
}

bool TrailRenderer::OnGUI()
{
    return Super::OnGUI();
}

void TrailRenderer::InnerRender(RenderTech renderTech)
{
    if (_dirty)
        RebuildBuffers();

    if (_vertexBuffer == nullptr || _indexBuffer == nullptr || _indices.empty())
        return;

    Super::InnerRender(renderTech);

    Material* material = _material.Resolve();
    if (material == nullptr)
        return;

    Shader* shader = material->GetShader();
    if (shader == nullptr)
        return;

    _vertexBuffer->PushData();
    _indexBuffer->PushData();
    shader->DrawIndexed(renderTech, _pass, static_cast<uint32>(_indices.size()));
}

void TrailRenderer::RebuildBuffers()
{
    _dirty = false;
    _vertices.clear();
    _indices.clear();

    if (_points.size() < 2)
    {
        _vertexBuffer = nullptr;
        _indexBuffer = nullptr;
        return;
    }

    vector<float> distances(_points.size(), 0.f);
    for (uint32 i = 1; i < _points.size(); ++i)
    {
        const Vec3 previousCenter = (_points[i - 1].base + _points[i - 1].tip) * 0.5f;
        const Vec3 currentCenter = (_points[i].base + _points[i].tip) * 0.5f;
        distances[i] = distances[i - 1] + Vec3::Distance(previousCenter, currentCenter);
    }

    const float totalDistance = distances.back();
    _vertices.reserve(_points.size() * 2);
    _indices.reserve((_points.size() - 1) * 6);

    for (uint32 i = 0; i < _points.size(); ++i)
    {
        const float trailRatio = totalDistance > 0.0001f
            ? distances[i] / totalDistance
            : static_cast<float>(i) / static_cast<float>(_points.size() - 1);

        VertexTextureNormalTangentData vertexBase;
        vertexBase.position = _points[i].base;
        vertexBase.uv = Vec2(0.f, trailRatio);
        _vertices.push_back(vertexBase);

        VertexTextureNormalTangentData vertexTip;
        vertexTip.position = _points[i].tip;
        vertexTip.uv = Vec2(1.f, trailRatio);
        _vertices.push_back(vertexTip);
    }

    for (uint32 i = 0; i < _points.size() - 1; ++i)
    {
        const uint32 currentBase = i;
        const uint32 currentTip = currentBase + 1;
        const uint32 nextBase = currentBase + 2;
        const uint32 nextTip = currentBase + 3;

        _indices.push_back(currentBase);
        _indices.push_back(currentTip);
        _indices.push_back(nextBase);

        _indices.push_back(currentTip);
        _indices.push_back(nextTip);
        _indices.push_back(nextBase);
    }

    _vertexBuffer = make_shared<VertexBuffer>();
    _vertexBuffer->Create(_vertices, "TrailRendererVB");
    _indexBuffer = make_shared<IndexBuffer>();
    _indexBuffer->Create(_indices, -1);
}
