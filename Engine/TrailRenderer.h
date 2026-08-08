#pragma once
#include "Renderer.h"

struct TrailPoint
{
    Vec3 base = Vec3::Zero;
    Vec3 tip = Vec3::Zero;
    float spawnTime = 0.f;
};

struct TrailVertexData
{
    Vec3 position = Vec3::Zero;
    Vec2 uv = Vec2::Zero;
    float spawnTime = 0.f;
    float lifetime = 0.f;
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

    void SetLifetime(float lifetime);
    float GetLifetime() const { return _lifetime; }

    virtual void Update() override;
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
    vector<TrailVertexData> _vertices;
    vector<uint32> _indices;

    shared_ptr<VertexBuffer> _vertexBuffer;
    shared_ptr<IndexBuffer> _indexBuffer;

    bool _dirty = true;
    float _lifetime = 0.35f;
};
