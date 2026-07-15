/*

        Copyright 2011 Etay Meiri

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
#include "pch.h"
#include "skinned_mesh.h"
#include "MathUtils.h"
#include <fstream>

#define POSITION_LOCATION    0
#define TEX_COORD_LOCATION   1
#define NORMAL_LOCATION      2
#define BONE_ID_LOCATION     3
#define BONE_WEIGHT_LOCATION 4


SkinnedMesh::~SkinnedMesh()
{
    Clear();
}


void SkinnedMesh::RecalcBoneTransforms(uint32 BoneIndex)
{
    BoneInfo& boneInfo = m_BoneInfo[BoneIndex];
    Matrix parentTransform = Matrix::Identity;
    if (boneInfo.ParentIndex >= 0)
    {
        parentTransform = m_BoneInfo[boneInfo.ParentIndex].GlobalTransformation;
    }
    Matrix GlobalTransformation = boneInfo.BoneLocalTransform * parentTransform;
    boneInfo.FinalTransformation = m_BoneInfo[BoneIndex].OffsetMatrix * GlobalTransformation * m_GlobalInverseTransform;
    boneInfo.GlobalTransformation = GlobalTransformation;
    boneInfo.GlobalTransformationInverse = GlobalTransformation.Invert();

    for(int child : boneInfo.ChildrenIndices)
    {
        RecalcBoneTransforms(child);
    }
}

void SkinnedMesh::Clear()
{
}


bool SkinnedMesh::LoadMesh(const string& Filename)
{
    // Release the previously loaded mesh (if it exists)
    Clear();

    bool Ret = false;

    pScene = Importer.ReadFile(Filename.c_str(),
        aiProcess_ConvertToLeftHanded |
        aiProcess_Triangulate |
        aiProcess_GenUVCoords |
        aiProcess_GenNormals |
        aiProcess_CalcTangentSpace );

    if (pScene) {
        m_GlobalInverseTransform = ConvertMatrix(pScene->mRootNode->mTransformation);
        m_GlobalInverseTransform = m_GlobalInverseTransform.Invert();
        Ret = InitFromScene(pScene, Filename);
    }
    else {
        printf("Error parsing '%s': '%s'\n", Filename.c_str(), Importer.GetErrorString());
    }

    return Ret;
}

bool SkinnedMesh::InitFromScene(const aiScene* pScene, const string& Filename)
{
    m_Meshes.resize(pScene->mNumMeshes);
    m_Materials.resize(pScene->mNumMaterials);

    unsigned int NumVertices = 0;
    unsigned int NumIndices = 0;

    CountVerticesAndIndices(pScene, NumVertices, NumIndices);

    ReserveSpace(NumVertices, NumIndices);

    InitAllMeshes(pScene);
    return true;
}


void SkinnedMesh::CountVerticesAndIndices(const aiScene* pScene, unsigned int& NumVertices, unsigned int& NumIndices)
{
    for (unsigned int i = 0 ; i < m_Meshes.size() ; i++) {
        m_Meshes[i].MaterialIndex = pScene->mMeshes[i]->mMaterialIndex;
        m_Meshes[i].NumIndices = pScene->mMeshes[i]->mNumFaces * 3;
        m_Meshes[i].BaseVertex = NumVertices;
        m_Meshes[i].BaseIndex = NumIndices;

        NumVertices += pScene->mMeshes[i]->mNumVertices;
        NumIndices  += m_Meshes[i].NumIndices;
    }
}

void SkinnedMesh::ReserveSpace(unsigned int NumVertices, unsigned int NumIndices)
{
    m_Positions.reserve(NumVertices);
    m_Normals.reserve(NumVertices);
    m_TexCoords.reserve(NumVertices);
    m_Indices.reserve(NumIndices);
    m_Bones.resize(NumVertices);
}

void SkinnedMesh::InitAllMeshes(const aiScene* pScene)
{
    for (unsigned int i = 0 ; i < m_Meshes.size() ; i++) {
        const aiMesh* paiMesh = pScene->mMeshes[i];
        InitSingleMesh(i, paiMesh);
    }
}

void SkinnedMesh::InitSingleMesh(uint32 MeshIndex, const aiMesh* paiMesh)
{
    const aiVector3D Zero3D(0.0f, 0.0f, 0.0f);

    // Populate the vertex attribute vectors
    for (unsigned int i = 0; i < paiMesh->mNumVertices; i++)
    {
        const aiVector3D& pPos = paiMesh->mVertices[i];
        m_Positions.push_back(Vec3(pPos.x, pPos.y, pPos.z));

        if (paiMesh->mNormals)
        {
            const aiVector3D& pNormal = paiMesh->mNormals[i];
            m_Normals.push_back(Vec3(pNormal.x, pNormal.y, pNormal.z));
        }
        else
        {
            aiVector3D Normal(0.0f, 1.0f, 0.0f);
            m_Normals.push_back(Vec3(Normal.x, Normal.y, Normal.z));
        }

        const aiVector3D& pTexCoord = paiMesh->HasTextureCoords(0) ? paiMesh->mTextureCoords[0][i] : Zero3D;
        m_TexCoords.push_back(Vec2(pTexCoord.x, pTexCoord.y));
    }

    LoadMeshBones(MeshIndex, paiMesh);

    // Populate the index buffer
    for (unsigned int i = 0; i < paiMesh->mNumFaces; i++)
    {
        const aiFace& Face = paiMesh->mFaces[i];
        //        printf("num indices %d\n", Face.mNumIndices);
        //        assert(Face.mNumIndices == 3);
        m_Indices.push_back(Face.mIndices[0]);
        m_Indices.push_back(Face.mIndices[1]);
        m_Indices.push_back(Face.mIndices[2]);
    }
}

void SkinnedMesh::LoadMeshBones(uint32 MeshIndex, const aiMesh* pMesh)
{
    for (uint32 i = 0 ; i < pMesh->mNumBones ; i++) {
        LoadSingleBone(MeshIndex, pMesh->mBones[i]);
    }
}

void SkinnedMesh::LoadSingleBone(uint32 MeshIndex, const aiBone* pBone)
{
    int BoneId = GetBoneId(pBone);

    if (BoneId == m_BoneInfo.size()) {
        BoneInfo bi;
        bi.OffsetMatrix = ConvertMatrix(pBone->mOffsetMatrix);
        m_BoneInfo.push_back(bi);
    }

    for (uint32 i = 0; i < pBone->mNumWeights; i++)
    {
        const aiVertexWeight& vw = pBone->mWeights[i];
        uint32 GlobalVertexID = m_Meshes[MeshIndex].BaseVertex + pBone->mWeights[i].mVertexId;
        m_Bones[GlobalVertexID].AddBoneData(BoneId, vw.mWeight);
    }
}

int SkinnedMesh::GetBoneId(const aiBone* pBone)
{
    int BoneIndex = 0;
    string BoneName(pBone->mName.C_Str());

    if (m_BoneNameToIndexMap.find(BoneName) == m_BoneNameToIndexMap.end()) {
        // Allocate an index for a new bone
        BoneIndex = (int)m_BoneNameToIndexMap.size();
        m_BoneNameToIndexMap[BoneName] = BoneIndex;
    }
    else {
        BoneIndex = m_BoneNameToIndexMap[BoneName];
    }

    return BoneIndex;
}


uint32 SkinnedMesh::FindPosition(float AnimationTimeTicks, const aiNodeAnim* pNodeAnim)
{
    for (uint32 i = 0 ; i < pNodeAnim->mNumPositionKeys - 1 ; i++) {
        float t = (float)pNodeAnim->mPositionKeys[i + 1].mTime;
        if (AnimationTimeTicks < t) {
            return i;
        }
    }

    return 0;
}


void SkinnedMesh::CalcInterpolatedPosition(aiVector3D& Out, float AnimationTimeTicks, const aiNodeAnim* pNodeAnim)
{
    // we need at least two values to interpolate...
    if (pNodeAnim->mNumPositionKeys == 1) {
        Out = pNodeAnim->mPositionKeys[0].mValue;
        return;
    }

    uint32 PositionIndex = FindPosition(AnimationTimeTicks, pNodeAnim);
    uint32 NextPositionIndex = PositionIndex + 1;
    assert(NextPositionIndex < pNodeAnim->mNumPositionKeys);
    float t1 = (float)pNodeAnim->mPositionKeys[PositionIndex].mTime;
    float t2 = (float)pNodeAnim->mPositionKeys[NextPositionIndex].mTime;
    float DeltaTime = t2 - t1 + ai_epsilon;
    float Factor = (AnimationTimeTicks - t1) / DeltaTime;
    Factor = clamp(Factor, 0.f, 1.f);
    const aiVector3D& Start = pNodeAnim->mPositionKeys[PositionIndex].mValue;
    const aiVector3D& End = pNodeAnim->mPositionKeys[NextPositionIndex].mValue;
    aiVector3D Delta = End - Start;
    Out = Start + Factor * Delta;
}


uint32 SkinnedMesh::FindRotation(float AnimationTimeTicks, const aiNodeAnim* pNodeAnim)
{
    assert(pNodeAnim->mNumRotationKeys > 0);

    uint32 foundIndex = 0;
    for (uint32 i = 0 ; i < pNodeAnim->mNumRotationKeys - 1 ; i++) {
        float t = (float)pNodeAnim->mRotationKeys[i + 1].mTime;
        if (t < 0.1)
        {
            continue;
        }
        if (AnimationTimeTicks < t) 
        {
            return foundIndex;
        }
        foundIndex = i + 1;
    }

    return 0;
}

namespace
{
    void DumpHipsRotationQuaternions(const float time,
        const Vec3 rotation)
    {
#if defined(_DEBUG)

        fs::create_directories("../DebugTextures");

        ofstream file("../DebugTextures/HipsRotationQuaternions.txt", ios::app);
        if (!file.is_open())
            return;

        file << "Time=" << time << '\n';
        file << "Rotation: " << rotation.x << ", " << rotation.y << ", " << rotation.z << "\n\n";
#endif
    }
}

void SkinnedMesh::CalcInterpolatedRotation(aiQuaternion& Out, float AnimationTimeTicks, const aiNodeAnim* pNodeAnim)
{
    // we need at least two values to interpolate...
    if (pNodeAnim->mNumRotationKeys == 1) {
        Out = pNodeAnim->mRotationKeys[0].mValue;
        return;
    }

    uint32 RotationIndex = FindRotation(AnimationTimeTicks, pNodeAnim);
    uint32 NextRotationIndex = FindRotation(AnimationTimeTicks + 1, pNodeAnim);
    assert(NextRotationIndex < pNodeAnim->mNumRotationKeys);
    float t1 = (float)pNodeAnim->mRotationKeys[RotationIndex].mTime;
    float t2 = (float)pNodeAnim->mRotationKeys[NextRotationIndex].mTime;
    float DeltaTime = t2 - t1 + ai_epsilon;
    float Factor = (AnimationTimeTicks - t1) / DeltaTime;
    Factor = clamp(Factor, 0.f, 1.f);
    const aiQuaternion& StartRotationQ = pNodeAnim->mRotationKeys[RotationIndex].mValue;
    const aiQuaternion& EndRotationQ   = pNodeAnim->mRotationKeys[NextRotationIndex].mValue;
    aiQuaternion::Interpolate(Out, StartRotationQ, EndRotationQ, Factor);
    Out.Normalize();

    //if (NodeName.find("RightUpLeg") != string::npos)
    //{
    //    for (int i = 0; i < pNodeAnim->mNumRotationKeys; i++)
    //    {
    //        const aiQuaternion& rotationQ = pNodeAnim->mRotationKeys[i].mValue;
    //        Quaternion quat(rotationQ.x, rotationQ.y, rotationQ.z, rotationQ.w);
    //        Vec3 eulerRotation = Transform::ToEulerAngles(quat);
    //        eulerRotation = MathUtils::RadToDeg(eulerRotation);
    //        DumpHipsRotationQuaternions((float)pNodeAnim->mRotationKeys[i].mTime, eulerRotation);
    //    }
    //}
}


uint32 SkinnedMesh::FindScaling(float AnimationTimeTicks, const aiNodeAnim* pNodeAnim)
{
    assert(pNodeAnim->mNumScalingKeys > 0);

    for (uint32 i = 0 ; i < pNodeAnim->mNumScalingKeys - 1 ; i++) {
        float t = (float)pNodeAnim->mScalingKeys[i + 1].mTime;
        if (AnimationTimeTicks < t) {
            return i;
        }
    }

    return 0;
}


void SkinnedMesh::CalcInterpolatedScaling(aiVector3D& Out, float AnimationTimeTicks, const aiNodeAnim* pNodeAnim)
{
    // we need at least two values to interpolate...
    if (pNodeAnim->mNumScalingKeys == 1) {
        Out = pNodeAnim->mScalingKeys[0].mValue;
        return;
    }

    uint32 ScalingIndex = FindScaling(AnimationTimeTicks, pNodeAnim);
    uint32 NextScalingIndex = ScalingIndex + 1;
    assert(NextScalingIndex < pNodeAnim->mNumScalingKeys);
    float t1 = (float)pNodeAnim->mScalingKeys[ScalingIndex].mTime;
    float t2 = (float)pNodeAnim->mScalingKeys[NextScalingIndex].mTime;
    float DeltaTime = t2 - t1 + ai_epsilon;
    float Factor = (AnimationTimeTicks - (float)t1) / DeltaTime;
    Factor = clamp(Factor, 0.f, 1.f);
    const aiVector3D& Start = pNodeAnim->mScalingKeys[ScalingIndex].mValue;
    const aiVector3D& End   = pNodeAnim->mScalingKeys[NextScalingIndex].mValue;
    aiVector3D Delta = End - Start;
    Out = Start + Factor * Delta;
}


namespace
{
    bool IsFbxPreTransformNode(const string& NodeName)
    {
        return NodeName.find("$AssimpFbx$_PreRotation") != string::npos ||
            NodeName.find("$AssimpFbx$_Rotation") != string::npos ||
            NodeName.find("$AssimpFbx$_PreTranslation") != string::npos ||
            NodeName.find("$AssimpFbx$_PreTranslate") != string::npos ||
            NodeName.find("$AssimpFbx$_Translation") != string::npos;
    }
}

void SkinnedMesh::ReadNodeHierarchy(float AnimationTimeTicks, const aiAnimation* pAnimation,
    const aiNode* pNode, const Matrix& ParentTransform, int ParentBoneIndex,
    const Matrix& PendingPreTransform)
{
    string NodeName(pNode->mName.data);

    Matrix NodeTransformation(ConvertMatrix(pNode->mTransformation));

    const aiNodeAnim* pNodeAnim = FindNodeAnim(pAnimation, NodeName);
    Matrix ParentTransformForNode = ParentTransform;

    if (pNodeAnim) {
        // Interpolate scaling and generate scaling transformation matrix
        aiVector3D Scaling;
        CalcInterpolatedScaling(Scaling, AnimationTimeTicks, pNodeAnim);
        Matrix ScalingM = Matrix::CreateScale(Scaling.x, Scaling.y, Scaling.z);

        // Interpolate rotation and generate rotation transformation matrix
        aiQuaternion RotationQ;
        CalcInterpolatedRotation(RotationQ, AnimationTimeTicks, pNodeAnim);
        Quaternion quat(RotationQ.x, RotationQ.y, RotationQ.z, RotationQ.w);
        Matrix RotationM = Matrix::CreateFromQuaternion(quat);

        // Interpolate translation and generate translation trbansformation matrix
        aiVector3D Translation;
        CalcInterpolatedPosition(Translation, AnimationTimeTicks, pNodeAnim);
        Matrix TranslationM = Matrix::CreateTranslation(Translation.x, Translation.y, Translation.z);

        // Combine the above transformations
        NodeTransformation =  ScalingM * RotationM * TranslationM;
        ParentTransformForNode = PendingPreTransform.Invert() * ParentTransform;
    }

    const Matrix GlobalTransformation = NodeTransformation * ParentTransformForNode;

    if (m_BoneNameToIndexMap.find(NodeName) != m_BoneNameToIndexMap.end())
    {
        uint32 BoneIndex = m_BoneNameToIndexMap[NodeName];
        BoneInfo& boneInfo = m_BoneInfo[BoneIndex];
        boneInfo.FinalTransformation = m_BoneInfo[BoneIndex].OffsetMatrix * GlobalTransformation * m_GlobalInverseTransform;
        boneInfo.GlobalTransformation = GlobalTransformation;
        boneInfo.GlobalTransformationInverse = GlobalTransformation.Invert();
        boneInfo.ParentIndex = ParentBoneIndex;        
        Matrix parentBoneTransformInv = Matrix::Identity;
        if (ParentBoneIndex >= 0)
        {
            parentBoneTransformInv = m_BoneInfo[ParentBoneIndex].GlobalTransformationInverse;
        }
        boneInfo.BoneLocalTransform = GlobalTransformation * parentBoneTransformInv;
        ParentBoneIndex = BoneIndex;
    }

    Matrix ChildPendingPreTransform = Matrix::Identity;
    if (IsFbxPreTransformNode(NodeName))
    {
        ChildPendingPreTransform = NodeTransformation * PendingPreTransform;
    }

    for (uint32 i = 0 ; i < pNode->mNumChildren ; i++) {
        ReadNodeHierarchy(AnimationTimeTicks, pAnimation,
            pNode->mChildren[i], GlobalTransformation, ParentBoneIndex, ChildPendingPreTransform);
    }
}

Matrix SkinnedMesh::ConvertMatrix(const aiMatrix4x4& from)
{
    Matrix to;
    to._11 = from.a1; to._12 = from.b1; to._13 = from.c1; to._14 = from.d1;
    to._21 = from.a2; to._22 = from.b2; to._23 = from.c2; to._24 = from.d2;
    to._31 = from.a3; to._32 = from.b3; to._33 = from.c3; to._34 = from.d3;
    to._41 = from.a4; to._42 = from.b4; to._43 = from.c4; to._44 = from.d4;
    return to;
}

void SkinnedMesh::LoadBoneInfos(float TimeInSeconds, uint32 AnimationIndex)
{
    if (!HasAnimations() || AnimationIndex >= pScene->mNumAnimations)
        return;

    const aiAnimation* animation = pScene->mAnimations[AnimationIndex];
    float TicksPerSecond = (float)(animation->mTicksPerSecond != 0 ? animation->mTicksPerSecond : 25.0f);
    float TimeInTicks = TimeInSeconds * TicksPerSecond;
    float AnimationTimeTicks = animation->mDuration > 0.0
        ? fmod(TimeInTicks, (float)animation->mDuration)
        : 0.0f;

    ReadNodeHierarchy(AnimationTimeTicks, animation, pScene->mRootNode, Matrix::Identity, -1, Matrix::Identity);

    for (uint32 i = 0 ; i < m_BoneInfo.size(); i++)
    {
        m_BoneInfo[i].ChildrenIndices.clear();
    }

    for (uint32 i = 0; i < m_BoneInfo.size(); i++)
    {
        int parentIndex = m_BoneInfo[i].ParentIndex;
        if (parentIndex >= 0)
        {
            m_BoneInfo[parentIndex].ChildrenIndices.push_back(i);
        }
    }
}

string SkinnedMesh::GetBoneName(uint32 BoneIndex) const
{
    for (const auto& [name, index] : m_BoneNameToIndexMap)
    {
        if (index == BoneIndex)
            return name;
    }

    return "Bone " + to_string(BoneIndex);
}


const aiNodeAnim* SkinnedMesh::FindNodeAnim(const aiAnimation* pAnimation, const string& NodeName)
{
    for (uint32 i = 0 ; i < pAnimation->mNumChannels ; i++) {
        const aiNodeAnim* pNodeAnim = pAnimation->mChannels[i];

        if (string(pNodeAnim->mNodeName.data) == NodeName) {
            return pNodeAnim;
        }
    }

    return NULL;
}
