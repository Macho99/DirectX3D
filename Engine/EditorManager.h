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
    void UnselectAll();

	bool TryGetSelectedTransform(OUT TransformRef& transformRef) const;
	void SetSelectedTransform(const TransformRef& transformRef);

	bool TryGetSelectedAsset(OUT AssetRef& assetRef, OUT int& subAssetIndex) const;
	void SetSelectedAsset(const AssetRef& assetRef, int subAssetIndex = -1);

private:
	TransformRef _selectedTransform;
    AssetRef _selectedAsset;
	int _selectedSubAsset = -1;
	vector<unique_ptr<EditorWindow>> _editorWindows;
};