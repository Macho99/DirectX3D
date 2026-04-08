#pragma once
#include "Renderer.h"
class LineRenderer : public Renderer
{
    using Super = Renderer;
    DECLARE_COMPONENT(LineRenderer)

public:
    LineRenderer();
    virtual ~LineRenderer();

    void SetPoints(const vector<Vec3>& points);
    const vector<Vec3>& GetPoints() const { return _points; }
    void AddPoint(const Vec3& point);
    void ClearPoints();
    void SetColor(const Color& color) { _color = color; }
    void SetVisualizeDirection(bool visualizeDirection);
    bool IsVisualizeDirection() const { return _visualizeDirection; }

    void SetLoop(bool loop);
    bool IsLoop() const { return _loop; }

    virtual void Awake() override;
    virtual bool OnGUI() override;
    virtual void InnerRender(RenderTech renderTech) override;

    template<typename Archive>
    void serialize(Archive& ar)
    {
        Super::serialize(ar);
        ar(CEREAL_NVP(_points));
        ar(CEREAL_NVP(_loop));
        ar(CEREAL_NVP(_visualizeDirection));
        ar(CEREAL_NVP(_arrowSizeRatio));
        if (Archive::is_loading::value)
            _dirty = true;
    }

private:
    void RebuildBuffers();

private:
    vector<Vec3> _points;
    vector<VertexColorData> _vertices;
    vector<uint32> _indices;

    shared_ptr<VertexBuffer> _vertexBuffer;
    shared_ptr<IndexBuffer> _indexBuffer;

    bool _loop = false;
    bool _dirty = true;
    bool _visualizeDirection = true;
    float _arrowSizeRatio = 0.25f;
    Color _color = { 1, 0, 0, 1 };
};