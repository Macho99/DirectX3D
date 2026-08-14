#pragma once
#include "Renderer.h"
class InstancingRenderer : public Renderer
{
public:
    using Super = Renderer;
    InstancingRenderer(ComponentType componentType);

    virtual AssetId GetMeshId() const = 0;
    virtual void RenderInstancing(class InstancingBuffer& buffer, RenderTech renderTech) = 0;
    virtual void LateUpdate() override;
    void AddInstancingData(const Matrix& mat);
    bool HasInstancingData() const { return _originInstDatas.size() > 0; }
    const vector<InstancingData>& GetInstancingDatas() const { return _instDatas; }

    virtual bool OnGUI() override;
    virtual void OnInspectorFocusLost() override;

    template<typename Archive>
    void serialize(Archive& ar)
    {
        Super::serialize(ar);
        CEREAL_NVP(_originInstDatas);
    }

protected:
    virtual void OnMaterialChange(const Material* oldMaterial, const Material* newMaterial) override;
    void OnMeshChange(const AssetId& oldMeshId, const AssetId& newMeshId);

private:
    vector<InstancingData> _originInstDatas;
    vector<InstancingData> _instDatas;
    Matrix _lastWorldMatrix;
    int _selectedInstDataIndex = -1;
};

