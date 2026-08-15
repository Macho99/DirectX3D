#pragma once

#include "EditorWindow.h"

class CubemapBuilderTool : public EditorWindow
{
    using Super = EditorWindow;

public:
    CubemapBuilderTool();

    static bool BuildCubemap(
        const array<fs::path, 6>& facePaths,
        const fs::path& outputPath,
        OUT string& error);

protected:
    void OnGUI() override;

private:
    void SelectFace(size_t faceIndex);
    void SelectOutput();
    void Build();
    void SetStatus(bool succeeded, const string& message);

private:
    array<fs::path, 6> _facePaths;
    fs::path _outputPath;
    bool _lastBuildSucceeded = false;
    string _status;
};
