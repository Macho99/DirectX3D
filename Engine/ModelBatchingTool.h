#pragma once

#include "EditorWindow.h"
#include "ComponentRef.h"

class ModelRenderer;

struct ModelRendererSlot
{
    ComponentRef<ModelRenderer> rendererRef;
    uint64 sourceMeshBytes = 0;
    uint64 estimatedOutputBytes = 0;
    size_t instanceCount = 0;
    size_t materialCount = 0;
    bool estimateInitialized = false;
    bool sizeAvailable = false;
    bool materialCountAvailable = false;
    bool overflowed = false;
};

class ModelBatchingTool : public EditorWindow
{
    using Super = EditorWindow;

public:
    ModelBatchingTool();

protected:
    void OnGUI() override;

private:
    size_t ExtractModelRenderersFromChildren();
    bool ExtractBatchInfo();
    bool Build();
    size_t GetCurrentMaterialCount() const;
    void SetStatus(bool succeeded, const string& message);

private:
    TransformRef _autoExtractRoot;
    vector<ModelRendererSlot> _modelRenderers = { ModelRendererSlot() };
    int _maxAtlasSize = 4096;
    int _maxTextureSize = 256;
    bool _lastBuildSucceeded = false;
    string _status;
};
