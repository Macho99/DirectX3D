/*

    Copyright 2021 Etay Meiri

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef SKINNED_MESH_H
#define SKINNED_MESH_H

#include <map>
#include <vector>

#include <assimp/Importer.hpp>      // C++ importer interface
#include <assimp/scene.h>       // Output data structure
#include <assimp/postprocess.h> // Post processing flags

class SkinnedMesh
{
public:
    struct BoneInfo
    {
        Matrix OffsetMatrix;
        Matrix GlobalTransformation;
        Matrix GlobalTransformationInverse;
        Matrix FinalTransformation; // with OffsetMatrix

        Matrix BoneLocalTransform;
        int ParentIndex;
        vector<int> ChildrenIndices;
    };

    SkinnedMesh() {};

    ~SkinnedMesh();

    bool LoadMesh(const std::string& Filename);

    uint32 NumBones() const
    {
        return (uint32)m_BoneNameToIndexMap.size();
    }

    void LoadBoneInfos(float AnimationTimeSec, uint32 AnimationIndex = 0);
    vector<BoneInfo>& GetBoneInfos() { return m_BoneInfo; }
    string GetBoneName(uint32 boneIndex) const;
    static Matrix ConvertMatrix(const aiMatrix4x4& from);

    bool HasAnimations() const
    {
        return pScene && pScene->mNumAnimations > 0;
    }

    uint32 GetAnimationCount() const
    {
        return HasAnimations() ? pScene->mNumAnimations : 0;
    }

    string GetAnimationName(uint32 animationIndex) const
    {
        if (!HasAnimations() || animationIndex >= pScene->mNumAnimations)
            return "";

        const string name = pScene->mAnimations[animationIndex]->mName.C_Str();
        return name.empty() ? "Animation " + to_string(animationIndex) : name;
    }

    uint32 GetAnimationFrameCount(uint32 animationIndex) const
    {
        if (!HasAnimations() || animationIndex >= pScene->mNumAnimations)
            return 0;

        const double duration = pScene->mAnimations[animationIndex]->mDuration;
        return max(1u, (uint32)ceil(max(0.0, duration)) + 1u);
    }

    float GetAnimationTicksPerSecond(uint32 animationIndex) const
    {
        if (!HasAnimations() || animationIndex >= pScene->mNumAnimations)
            return 0.0f;

        const double ticksPerSecond = pScene->mAnimations[animationIndex]->mTicksPerSecond;
        return (float)(ticksPerSecond != 0.0 ? ticksPerSecond : 25.0);
    }

    float GetAnimationDuration(uint32 animationIndex) const
    {
        if (!HasAnimations() || animationIndex >= pScene->mNumAnimations)
            return 0.0f;

        return (float)pScene->mAnimations[animationIndex]->mDuration;
    }

    void RecalcBoneTransforms(uint32 BoneIndex);

private:
    #define MAX_NUM_BONES_PER_VERTEX 4

    void Clear();

    bool InitFromScene(const aiScene* pScene, const std::string& Filename);

    void CountVerticesAndIndices(const aiScene* pScene, unsigned int& NumVertices, unsigned int& NumIndices);

    void ReserveSpace(unsigned int NumVertices, unsigned int NumIndices);

    void InitAllMeshes(const aiScene* pScene);

    void InitSingleMesh(uint32 MeshIndex, const aiMesh* paiMesh);

    void LoadMeshBones(uint32 MeshIndex, const aiMesh* paiMesh);
    void LoadSingleBone(uint32 MeshIndex, const aiBone* pBone);
    int GetBoneId(const aiBone* pBone);
    void CalcInterpolatedScaling(aiVector3D& Out, float AnimationTime, const aiNodeAnim* pNodeAnim);
    void CalcInterpolatedRotation(aiQuaternion& Out, float AnimationTime, const aiNodeAnim* pNodeAnim, const string& NodeName);
    void CalcInterpolatedPosition(aiVector3D& Out, float AnimationTime, const aiNodeAnim* pNodeAnim);
    uint32 FindScaling(float AnimationTime, const aiNodeAnim* pNodeAnim);
    uint32 FindRotation(float AnimationTime, const aiNodeAnim* pNodeAnim);
    uint32 FindPosition(float AnimationTime, const aiNodeAnim* pNodeAnim);
    const aiNodeAnim* FindNodeAnim(const aiAnimation* pAnimation, const string& NodeName);
    void ReadNodeHierarchy(float AnimationTime, const aiAnimation* pAnimation,
        const aiNode* pNode, const Matrix& ParentTransform, int parentBoneIndex,
        const Matrix& PendingPreTransform);

#define INVALID_MATERIAL 0xFFFFFFFF

    enum BUFFER_TYPE {
        INDEX_BUFFER = 0,
        POS_VB       = 1,
        TEXCOORD_VB  = 2,
        NORMAL_VB    = 3,
        BONE_VB      = 4,
        NUM_BUFFERS  = 5
    };

    struct BasicMeshEntry {
        BasicMeshEntry()
        {
            NumIndices = 0;
            BaseVertex = 0;
            BaseIndex = 0;
            MaterialIndex = INVALID_MATERIAL;
        }

        unsigned int NumIndices;
        unsigned int BaseVertex;
        unsigned int BaseIndex;
        unsigned int MaterialIndex;
    };
    struct VertexBoneData
    {
        uint32 BoneIDs[MAX_NUM_BONES_PER_VERTEX] = { 0 };
        float Weights[MAX_NUM_BONES_PER_VERTEX] = { 0.0f };

        VertexBoneData()
        {
        }

        void AddBoneData(uint32 BoneID, float Weight)
        {
            for (uint32 i = 0; i < MAX_NUM_BONES_PER_VERTEX; i++)
            {
                if (Weights[i] == 0.0)
                {
                    BoneIDs[i] = BoneID;
                    Weights[i] = Weight;
                    //printf("Adding bone %d weight %f at index %i\n", BoneID, Weight, i);
                    return;
                }
            }

            // should never get here - more bones than we have space for
            assert(0);
        }
    };
    Assimp::Importer Importer;
    const aiScene* pScene = NULL;
    std::vector<BasicMeshEntry> m_Meshes;
    std::vector<Material> m_Materials;

    vector<Vec3> m_Positions;
    vector<Vec3> m_Normals;
    vector<Vec2> m_TexCoords;
    vector<unsigned int> m_Indices;
    vector<VertexBoneData> m_Bones;

    map<string,uint32> m_BoneNameToIndexMap;

    vector<BoneInfo> m_BoneInfo;
    Matrix m_GlobalInverseTransform;
};

#endif  /* SKINNED_MESH_H */
