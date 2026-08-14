#pragma once

#include "EditorWindow.h"
#include "ComponentRef.h"

class ModelRenderer;

class ModelBatchingTool : public EditorWindow
{
    using Super = EditorWindow;

public:
    ModelBatchingTool();

protected:
    void OnGUI() override;

private:
    bool Build();
    size_t GetCurrentMaterialCount() const;
    void SetStatus(bool succeeded, const string& message);

private:
    vector<ComponentRef<ModelRenderer>> _modelRenderers = { ComponentRef<ModelRenderer>() };
    int _maxAtlasSize = 4096;
    int _maxTextureSize = 256;
    bool _lastBuildSucceeded = false;
    string _status;
};
