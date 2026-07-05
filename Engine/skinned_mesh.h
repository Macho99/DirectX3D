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
    SkinnedMesh() {};

    ~SkinnedMesh();

    bool LoadMesh(const std::string& Filename);

    uint32 NumBones() const
    {
        return (uint32)m_BoneNameToIndexMap.size();
    }

    void GetBoneTransforms(float AnimationTimeSec, vector<Matrix>& Transforms);
    static Matrix ConvertMatrix(const aiMatrix4x4& from);

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
    void CalcInterpolatedRotation(aiQuaternion& Out, float AnimationTime, const aiNodeAnim* pNodeAnim);
    void CalcInterpolatedPosition(aiVector3D& Out, float AnimationTime, const aiNodeAnim* pNodeAnim);
    uint32 FindScaling(float AnimationTime, const aiNodeAnim* pNodeAnim);
    uint32 FindRotation(float AnimationTime, const aiNodeAnim* pNodeAnim);
    uint32 FindPosition(float AnimationTime, const aiNodeAnim* pNodeAnim);
    const aiNodeAnim* FindNodeAnim(const aiAnimation* pAnimation, const string& NodeName);
    void ReadNodeHierarchy(float AnimationTime, const aiNode* pNode, const Matrix& ParentTransform);

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

    struct BoneInfo
    {
        Matrix OffsetMatrix;
        Matrix FinalTransformation;

        BoneInfo(const Matrix& Offset)
        {
            OffsetMatrix = Offset;
            FinalTransformation = Matrix::Identity;
        }
    };

    vector<BoneInfo> m_BoneInfo;
    Matrix m_GlobalInverseTransform;
};

#endif  /* SKINNED_MESH_H */
