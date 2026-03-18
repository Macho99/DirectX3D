#pragma once
#include "DirectoryWatcherWin.h"
class EditorWindow;

enum class EditorIcon
{
	Lock,
	Unlock,

	Max,
};

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

	void HandleRemove(const AssetRef& assetRef);
    void HandleRemove(const TransformRef& transformRef);

	bool TryGetHierarchyTransform(OUT TransformRef& transformRef) const;
	void FocusHierarchyTransform(const TransformRef& transformRef);

    bool TryGetHierarchyFocusMoveTransform(OUT TransformRef& transformRef) const;
    void SetFocusMoveHierarchyTransform(const TransformRef& transformRef);

	bool TryGetContentBrowserAsset(OUT AssetRef& assetRef, OUT int& subAssetIndex) const;
	void FocusContentBrowserAsset(const AssetRef& assetRef);

    Texture * GetEditorIconTexture(EditorIcon icon);

    bool IsInspectorLocked() const { return _inspectorLock; }
    void SetInspectorLock(bool lock) { _inspectorLock = lock; }

private:
	TransformRef _inspectorTransform;
    AssetRef _inspectorAsset;
    int _inspectorSubAsset = -1;

	TransformRef _hierarchyTransform;
	TransformRef _hierarchyFocusMoveTransform;

    AssetRef _contentBrowserAsset;
	int _contentBrowserSubAsset = -1;

	bool _inspectorLock = false;

	vector<unique_ptr<EditorWindow>> _editorWindows; 
	char _saveSceneName[256] = "NewScene";
};