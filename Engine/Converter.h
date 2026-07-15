#pragma once

#include "AsTypes.h"
#include "SubAssetMetaFile.h"

namespace tinyxml2
{
	class XMLDocument;
	class XMLElement;
}

class Converter
{
public:
	Converter();
	~Converter();

public:
	void ReadAssetFile(wstring file);
	//void ExportMaterialData(wstring savePath);
	void ExportAnimationData(wstring savePath, uint32 index = 0);
	void TryExportAll(wstring assetPath, wstring artifactPath, const vector<SubAssetInfo>& prev, OUT vector<SubAssetInfo>& exported);

private:
	//void ReadModelData(aiNode* node, int32 index, int32 parent);
	//void ReadMeshData(aiNode* node, int32 bone);

    void ReadAllMeshes(const aiScene* scene);
	void ReadSingleMesh(uint32 meshIndex, const aiMesh* paiMesh);
    void ReadMeshBones(uint32 meshIndex, const aiMesh* paiMesh);
    void ReadSingleBone(uint32 meshIndex, const aiBone* pBone);
	void ReadVertexBlendData();
	void ReadNodeHierarchy(const unordered_map<string, int32>& boneNameToIndexMap, const aiNode* pNode, int32 parentBoneIndex, const Matrix& hierarchyTransform);
	void WriteModelFile(wstring finalPath);

private:
	void ReadMaterialData();
	void WriteMaterialData(const fs::path& assetPath, const fs::path& artifactPath, const vector<SubAssetInfo>& prev, OUT vector<SubAssetInfo>& exported);
	ResourceRef<Texture> WriteTexture(string file, const fs::path& assetPath, const fs::path& artifactFolder, const vector<SubAssetInfo>& prev, OUT vector<SubAssetInfo>& exported);

private:
	shared_ptr<asAnimation> ReadAnimationData_New(aiAnimation* srcAnimation);
    void DumpAnimationData(aiAnimation* srcAnimation, const fs::path& path);
	shared_ptr<asAnimation> ReadAnimationData(aiAnimation* srcAnimation);
	shared_ptr<asAnimationNode> ParseAnimationNode(shared_ptr<asAnimation> animation, aiNodeAnim* srcNode);
	void ReadKeyframeData(shared_ptr<asAnimation> animation, aiNode* srcNode, map<string, shared_ptr<asAnimationNode>>& cache);
	void WriteAnimationData(shared_ptr<asAnimation> animation, wstring finalPath);

	AssetId AddExported(const vector<SubAssetInfo>& prev, OUT vector<SubAssetInfo>& exported, wstring fileName, ResourceType resourceType);

private:
	int GetBoneIndex(const string& name);
    Matrix ConvertMatrix(const aiMatrix4x4& from);

private:
	shared_ptr<Assimp::Importer> _importer;
	const aiScene* _scene;

private:
	vector<shared_ptr<asBone>> _bones;
	vector<shared_ptr<asMesh>> _meshes;
	vector<shared_ptr<asMaterial>> _materials;
};

