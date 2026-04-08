#include "pch.h"
#include "LineRenderer.h"
#include "Material.h"
#include "OnGUIUtils.h"

LineRenderer::LineRenderer()
    : Super(StaticType)
{
}

LineRenderer::~LineRenderer()
{
}

void LineRenderer::SetPoints(const vector<Vec3>& points)
{
    _points = points;
    _dirty = true;
}

void LineRenderer::AddPoint(const Vec3& point)
{
    _points.push_back(point);
    _dirty = true;
}

void LineRenderer::ClearPoints()
{
    _points.clear();
    _dirty = true;
}

void LineRenderer::SetVisualizeDirection(bool visualizeDirection)
{
    if (_visualizeDirection == visualizeDirection)
        return;

    _visualizeDirection = visualizeDirection;
    _dirty = true;
}

void LineRenderer::SetLoop(bool loop)
{
    if (_loop == loop)
        return;

    _loop = loop;
    _dirty = true;
}

void LineRenderer::Awake()
{
    Super::Awake();

    if (_material.IsValid() == false)
        SetMaterial(RESOURCES->GetResourceRefByPath<Material>("Materials\\LineMat.mat"));
}

bool LineRenderer::OnGUI()
{
    bool changed = false;
    changed |= Super::OnGUI();

    ImGui::Separator();

    changed |= OnGUIUtils::DrawColor("Color", &_color);
    bool loop = _loop;
    changed |= OnGUIUtils::DrawBool("Loop", &loop);
    if (loop != _loop)
        SetLoop(loop);

    bool visualizeDirection = _visualizeDirection;
    changed |= OnGUIUtils::DrawBool("Visualize Direction", &visualizeDirection);
    if (visualizeDirection != _visualizeDirection)
        SetVisualizeDirection(visualizeDirection);
    changed |= OnGUIUtils::DrawFloat("Arrow Size Ratio", &_arrowSizeRatio, 0.01f);

    ImGui::Separator();
    uint32 pointCount = _points.size();
    bool pointCountChanged = OnGUIUtils::DrawUInt32("Point Count", &pointCount, 1.f);
    if (pointCountChanged && pointCount >= 0)
    {
        if (pointCount > _points.size())
        {
            for (int i = _points.size(); i < pointCount; i++)
            {
                _points.push_back(Vec3::Zero);
            }
        }
        else
        {
            _points.resize(pointCount);
        }
        changed = true;
    }
    for (int i = 0; i < _points.size(); i++)
    {
        string label = "Point " + to_string(i);
        changed |= OnGUIUtils::DrawVec3(label.c_str(), &_points[i]);
    }
    ImGui::Separator();

    if (ImGui::Button("Clear Points"))
    {
        ClearPoints();
        changed = true;
    }

    if (changed)
        _dirty = true;
    return changed;
}

void LineRenderer::InnerRender(RenderTech renderTech)
{
    if (_points.size() < 2)
        return;

    if (_dirty)
        RebuildBuffers();

    //if (_vertexBuffer == nullptr || _indexBuffer == nullptr || _indices.empty())
    //    return;

    Super::InnerRender(renderTech);

    Material* material = _material.Resolve();
    if (material == nullptr)
        return;

    Shader* shader = material->GetShader();
    if (shader == nullptr)
        return;

    DC->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);

    _vertexBuffer->PushData();
    _indexBuffer->PushData();
    shader->DrawIndexed(renderTech, _pass, static_cast<uint32>(_indices.size()));
}

void LineRenderer::RebuildBuffers()
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

    for (const Vec3& point : _points)
    {
        VertexColorData vertex;
        vertex.position = point;
        vertex.color = _color;
        _vertices.push_back(vertex);
    }

    for (uint32 i = 0; i + 1 < _points.size(); ++i)
    {
        _indices.push_back(i);
        _indices.push_back(i + 1);
    }

    if (_loop)
    {
        _indices.push_back(static_cast<uint32>(_points.size() - 1));
        _indices.push_back(0);
    }

    if (_visualizeDirection)
    {
        auto appendArrowHead = [&](uint32 fromIndex, uint32 toIndex)
            {
                const Vec3& from = _points[fromIndex];
                const Vec3& to = _points[toIndex];
                Vec3 direction = to - from;
                const float length = direction.Length();
                if (length < 0.0001f)
                    return;

                direction /= length;

                const float arrowLength = _arrowSizeRatio;
                Vec3 normal = direction.Cross(Vec3::Up);
                if (normal.LengthSquared() < 0.0001f)
                    normal = direction.Cross(Vec3::Right);
                normal.Normalize();

                Vec3 wingDirection = (direction * -1.0f) + (normal * 0.5f);
                wingDirection.Normalize();
                Vec3 leftWing = to + (wingDirection * arrowLength);

                wingDirection = (direction * -1.0f) - (normal * 0.5f);
                wingDirection.Normalize();
                Vec3 rightWing = to + (wingDirection * arrowLength);

                VertexColorData leftVertex;
                leftVertex.position = leftWing;
                leftVertex.color = _color;
                uint32 leftIndex = static_cast<uint32>(_vertices.size());
                _vertices.push_back(leftVertex);

                VertexColorData rightVertex;
                rightVertex.position = rightWing;
                rightVertex.color = _color;
                uint32 rightIndex = static_cast<uint32>(_vertices.size());
                _vertices.push_back(rightVertex);

                _indices.push_back(toIndex);
                _indices.push_back(leftIndex);
                _indices.push_back(toIndex);
                _indices.push_back(rightIndex);
            };

        for (uint32 i = 0; i + 1 < _points.size(); ++i)
            appendArrowHead(i, i + 1);

        if (_loop)
            appendArrowHead(static_cast<uint32>(_points.size() - 1), 0);
    }

    _vertexBuffer = make_shared<VertexBuffer>();
    _vertexBuffer->Create(_vertices, "LineRendererVB");
    _indexBuffer = make_shared<IndexBuffer>();
    _indexBuffer->Create(_indices, -1);
}