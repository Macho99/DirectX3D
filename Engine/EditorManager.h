#pragma once
#include "DirectoryWatcherWin.h"
class EditorWindow;

class EditorManager
{
public:
    EditorManager();
    ~EditorManager();
	static EditorManager* GetInstance()
	{
		static EditorManager s_instance;
		return &s_instance;
	}

public:
	void Init();
	void Update();
	void DrawDockSpace();
	void Render();
    void OnDestroy();

public:
    void GetInspectorRef(OUT TransformRef& transformRef, OUT AssetRef& assetRef, OUT int& subAssetIndex) const;
    void ClickTransform(const TransformRef& transformRef);
    void ClickAsset(const AssetRef& assetRef);
    void UnselectAsset() { ClickAsset(AssetRef()); }

	bool TryGetHierarchyTransform(OUT TransformRef& transformRef) const;
	void FocusHierarchyTransform(const TransformRef& transformRef);

	bool TryGetContentBrowserAsset(OUT AssetRef& assetRef, OUT int& subAssetIndex) const;
	void FocusContentBrowserAsset(const AssetRef& assetRef);

private:
	TransformRef _inspectorTransform;
    AssetRef _inspectorAsset;
    int _inspectorSubAsset = -1;

	TransformRef _hierarchyTransform;

    AssetRef _contentBrowserAsset;
	int _contentBrowserSubAsset = -1;

	vector<unique_ptr<EditorWindow>> _editorWindows;
};