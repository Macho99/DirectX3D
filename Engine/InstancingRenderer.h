#pragma once
#include "Renderer.h"
class InstancingRenderer : public Renderer
{
public:
    using Super = Renderer;
    InstancingRenderer(ComponentType componentType, bool isStatic);

    virtual AssetId GetMeshId() const = 0;
    virtual void RenderInstancing(class InstancingBuffer& buffer, RenderTech renderTech) = 0;
    virtual void LateUpdate() override;
    virtual void OnEnable() override;
    bool IsInFrustum(const Vec4 frustumPlanes[6]);
    void AddInstancingData(const Matrix& mat);
    bool HasInstancingData() const { return _originInstDatas.size() > 0; }
    int GetInstancingCount() const { return _originInstDatas.size(); }
    const vector<InstancingData>& GetInstancingDatas() const { return _instDatas; }
    virtual int GetVersion() const override { return Super::GetVersion() + 1; }

    virtual bool OnGUI() override;
    virtual void OnInspectorFocusLost() override;

    template<typename Archive>
    void serialize(Archive& ar)
    {
        Super::serialize(ar);

        if(_version >= 1)
            ar(CEREAL_NVP(_originInstDatas));
    }

protected:
    virtual void OnMaterialChange(const Material* oldMaterial, const Material* newMaterial) override;
    void OnMeshChange(const AssetId& oldMeshId, const AssetId& newMeshId);
    void InvalidateBounds() { _boundsInitialized = false; }
    virtual bool TryCalculateLocalBounds(OUT BoundingBox& localBounds) = 0;

private:
    void UpdateBounds();

    vector<InstancingData> _originInstDatas;
    vector<InstancingData> _instDatas;
    Matrix _lastWorldMatrix;
    int _selectedInstDataIndex = -1;

    BoundingBox _worldBounds;
    bool _hasWorldBounds = false;
    bool _boundsInitialized = false;
    bool _isStatic = false;
};

