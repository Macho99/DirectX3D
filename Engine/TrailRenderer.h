#pragma once
#include "Renderer.h"

struct TrailPoint
{
    Vec3 base = Vec3::Zero;
    Vec3 tip = Vec3::Zero;
};

class TrailRenderer : public Renderer
{
    using Super = Renderer;
    DECLARE_COMPONENT(TrailRenderer)

public:
    TrailRenderer();
    virtual ~TrailRenderer();

    void SetPoints(const vector<TrailPoint>& points);
    const vector<TrailPoint>& GetPoints() const { return _points; }

    void AddPoint(const Vec3& base, const Vec3& tip);
    bool SetPoint(uint32 index, const Vec3& base, const Vec3& tip);
    bool RemovePoint(uint32 index);
    void ClearPoints();

    virtual bool OnGUI() override;
    virtual void InnerRender(RenderTech renderTech) override;

    template<typename Archive>
    void serialize(Archive& ar)
    {
        Super::serialize(ar);
    }

private:
    void RebuildBuffers();

private:
    vector<TrailPoint> _points;
    vector<VertexTextureNormalTangentData> _vertices;
    vector<uint32> _indices;

    shared_ptr<VertexBuffer> _vertexBuffer;
    shared_ptr<IndexBuffer> _indexBuffer;

    bool _dirty = true;
};
