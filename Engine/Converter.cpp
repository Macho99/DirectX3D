#include "pch.h"
#include "Converter.h"
#include <filesystem>
#include "Utils.h"
#include "FileUtils.h"
#include "Material.h"
#include "fstream"
#include "ModelMeshResource.h"
#include "skinned_mesh.h"
#include "MathUtils.h"
#include "TextureMeta.h"

Converter::Converter()
{
    _importer = make_shared<Assimp::Importer>();
}

Converter::~Converter()
{
}

void Converter::ReadAssetFile(wstring file, const ModelImportSettings& importSettings)
{
    auto p = std::filesystem::path(file);
    assert(std::filesystem::exists(p));

    _scene = _importer->ReadFile(Utils::ToString(file),
        aiProcess_ConvertToLeftHanded |
        aiProcess_Triangulate |
        aiProcess_GenUVCoords |
        aiProcess_GenNormals |
        aiProcess_CalcTangentSpace
    );

    assert(_scene != nullptr);
    ResolveImportTransform(importSettings);
}

//void Converter::ExportMaterialData(wstring savePath)
//{
//	wstring finalPath = savePath + L".mat";
//	ReadMaterialData();
//	WriteMaterialData(finalPath);
//}

void Converter::TryExportAll(wstring assetPath, wstring artifactPath, const vector<SubAssetInfo>& prev, OUT vector<SubAssetInfo>& exported)
{
    {
        ReadMaterialData();
        if (_materials.size() > 0)
        {
            WriteMaterialData(assetPath, artifactPath, prev, exported);
        }
    }

    {
        //ReadModelData(_scene->mRootNode, -1, -1);
        ReadAllMeshes(_scene);
        ReadVertexBlendData();
        if (_meshes.size() > 0)
        {
            unordered_map<string, int32> boneNameToIndexMap;
            for (uint32 i = 0; i < _bones.size(); ++i)
            {
                boneNameToIndexMap[_bones[i]->name] = _bones[i]->index;
            }
            const Matrix hierarchyTransform = _applyImportTransform ? _importTransform : Matrix::Identity;
            ReadNodeHierarchy(boneNameToIndexMap, _scene->mRootNode, -1, hierarchyTransform);

            SubAssetInfo info = SubAssetInfo();
            wstring assetName = Utils::ToWString(_meshes[0]->name) + ModelMeshResource::GetExtension();
            wstring finalPath = artifactPath + L"\\" + assetName;
            WriteModelFile(finalPath);
            AddExported(prev, exported, assetName, ResourceType::ModelMesh);
        }
    }

    {
        for (uint32 index = 0; index < _scene->mNumAnimations; ++index)
        {
            DumpAnimationData(_scene->mAnimations[index], artifactPath + L"\\" + L"AnimDump");
            wstring animName = Utils::ToWString(_scene->mAnimations[index]->mName.C_Str());
            if (animName.empty() || animName == L"mixamo.com")
            {
                fs::path filePath = assetPath;
                animName = filePath.stem().wstring();
            }

            shared_ptr<asAnimation> animation = ReadAnimationData(_scene->mAnimations[index], animName);

            SubAssetInfo info = SubAssetInfo();
            wstring assetName = animName + L".clip";
            while (assetName.find(L"|") != wstring::npos)
            {
                assetName.replace(assetName.find(L"|"), 1, L"_");
            }
            wstring finalPath = artifactPath + L"\\" + assetName;
            WriteAnimationData(animation, finalPath);
            AddExported(prev, exported, assetName, ResourceType::Animation);
        }
    }
}
//
//void Converter::ReadModelData(aiNode* node, int32 index, int32 parent)
//{
//    shared_ptr<asBone> bone = make_shared<asBone>();
//    bone->index = index;
//    bone->parent = parent;
//    bone->name = node->mName.C_Str();
//
//    // Relative Transform (Parent)
//    Matrix transform(node->mTransformation[0]);
//    bone->transform = transform.Transpose();
//
//    // Local Transform (Root)
//    Matrix matParent = Matrix::Identity;
//    if (parent >= 0)
//    {
//        matParent = _bones[parent]->transform;
//    }
//    bone->transform = bone->transform * matParent;
//
//    _bones.push_back(bone);
//
//    ReadMeshData(node, index);
//
//    for (uint32 i = 0; i < node->mNumChildren; ++i)
//    {
//        ReadModelData(node->mChildren[i], _bones.size(), index);
//    }
//}
//
//void Converter::ReadMeshData(aiNode* node, int32 bone)
//{
//    if (node->mNumMeshes < 1)
//        return;
//
//    for (uint32 i = 0; i < node->mNumMeshes; ++i)
//    {
//        shared_ptr<asMesh> mesh = make_shared<asMesh>();
//        mesh->name = node->mName.C_Str();
//        mesh->boneIndex = bone;
//
//        uint32 index = node->mMeshes[i];
//        const aiMesh* srcMesh = _scene->mMeshes[index];
//
//        const aiMaterial* srcMat = _scene->mMaterials[srcMesh->mMaterialIndex];
//        mesh->materialName = srcMat->GetName().C_Str();
//
//        const uint32 startVertex = mesh->vertices.size();
//
//        for (uint32 v = 0; v < srcMesh->mNumVertices; ++v)
//        {
//            VertexType vertex;
//            // Vertex
//            ::memcpy(&vertex.position, &srcMesh->mVertices[v], sizeof(Vec3));
//
//            // UV
//            if (srcMesh->HasTextureCoords(0))
//                ::memcpy(&vertex.uv, &srcMesh->mTextureCoords[0][v], sizeof(Vec2));
//
//            // Normal
//            if (srcMesh->HasNormals())
//                ::memcpy(&vertex.normal, &srcMesh->mNormals[v], sizeof(Vec3));
//
//            mesh->vertices.push_back(vertex);
//        }
//
//        // Index
//        for (uint32 f = 0; f < srcMesh->mNumFaces; ++f)
//        {
//            aiFace& face = srcMesh->mFaces[f];
//
//            for (uint32 k = 0; k < face.mNumIndices; ++k)
//            {
//                mesh->indices.push_back(face.mIndices[k] + startVertex);
//            }
//        }
//        _meshes.push_back(mesh);
//    }
//}

void Converter::ReadVertexBlendData()
{
    for (uint32 i = 0; i < _scene->mNumMeshes; i++)
    {
        aiMesh* srcMesh = _scene->mMeshes[i];
        if (srcMesh->HasBones() == false)
            continue;

        shared_ptr<asMesh> mesh = _meshes[i];

        vector<asBoneWeights> tempVertexBoneWeights;
        tempVertexBoneWeights.resize(mesh->vertices.size());

        // Bone을 순회하면서 연관된 VertexId, Weight를 구해서 기록
        for (uint32 b = 0; b < srcMesh->mNumBones; b++)
        {
            aiBone* srcMeshBone = srcMesh->mBones[b];
            uint32 boneIndex = GetBoneIndex(srcMeshBone->mName.C_Str());

            for (uint32 w = 0; w < srcMeshBone->mNumWeights; w++)
            {
                uint32 index = srcMeshBone->mWeights[w].mVertexId;
                float weight = srcMeshBone->mWeights[w].mWeight;

                tempVertexBoneWeights[index].AddWeights(boneIndex, weight);
            }
        }

        // 최종 결과 계산
        for (uint32 v = 0; v < tempVertexBoneWeights.size(); v++)
        {
            tempVertexBoneWeights[v].Normalize();
            asBlendWeight blendWeight = tempVertexBoneWeights[v].GetBlendWeights();
            mesh->vertices[v].blendIndices = blendWeight.indices;
            mesh->vertices[v].blendWeights = blendWeight.weights;
        }
    }
}

void Converter::ReadNodeHierarchy(const unordered_map<string, int32>& boneNameToIndexMap, const aiNode* pNode, int32 parentBoneIndex, const Matrix& hierarchyTransform)
{
    string nodeName(pNode->mName.data);
    Matrix nodeTransformation(ConvertMatrix(pNode->mTransformation));
    const Matrix globalTransformation = nodeTransformation * hierarchyTransform;

    auto it = boneNameToIndexMap.find(nodeName);
    if (it != boneNameToIndexMap.end())
    {
        int boneIndex = it->second;
        shared_ptr<asBone>& bone = _bones[boneIndex];
        bone->globalMatrix = globalTransformation;
        bone->parentIndex = parentBoneIndex;

        Matrix parentTransformation = Matrix::Identity;
        if (parentBoneIndex >= 0)
        {
            parentTransformation = _bones[parentBoneIndex]->globalMatrix;
        }
        bone->localMatrix = globalTransformation * parentTransformation.Invert();

        parentBoneIndex = boneIndex;
    }

    for (uint32 i = 0; i < pNode->mNumChildren; ++i)
    {
        ReadNodeHierarchy(boneNameToIndexMap, pNode->mChildren[i], parentBoneIndex, globalTransformation);
    }
}

void Converter::ReadAllMeshes(const aiScene* scene)
{
    for (unsigned int i = 0; i < scene->mNumMeshes; i++)
    {
        const aiMesh* paiMesh = scene->mMeshes[i];
        ReadSingleMesh(i, paiMesh);
    }
}

void Converter::ReadSingleMesh(uint32 meshIndex, const aiMesh* srcMesh)
{
    shared_ptr<asMesh> mesh = make_shared<asMesh>();
    mesh->name = srcMesh->mName.C_Str();
    mesh->boneIndex = 0;

    const aiMaterial* srcMat = _scene->mMaterials[srcMesh->mMaterialIndex];
    mesh->materialName = srcMat->GetName().C_Str();

    const uint32 startVertex = mesh->vertices.size();

    for (uint32 v = 0; v < srcMesh->mNumVertices; ++v)
    {
        VertexType vertex;
        // Vertex
        ::memcpy(&vertex.position, &srcMesh->mVertices[v], sizeof(Vec3));

        // UV
        if (srcMesh->HasTextureCoords(0))
            ::memcpy(&vertex.uv, &srcMesh->mTextureCoords[0][v], sizeof(Vec2));

        // Normal
        if (srcMesh->HasNormals())
            ::memcpy(&vertex.normal, &srcMesh->mNormals[v], sizeof(Vec3));

        if (srcMesh->HasTangentsAndBitangents())
        {
            ::memcpy(&vertex.tangent, &srcMesh->mTangents[v], sizeof(Vec3));
            //::memcpy(&vertex.bitangent, &srcMesh->mBitangents[v], sizeof(Vec3));
        }

        if (_applyImportTransform && srcMesh->HasBones() == false)
        {
            vertex.position = Vec3::Transform(vertex.position, _importTransform);
            vertex.normal = Vec3::TransformNormal(vertex.normal, _importTransform);
            vertex.normal.Normalize();
            if (srcMesh->HasTangentsAndBitangents())
            {
                vertex.tangent = Vec3::TransformNormal(vertex.tangent, _importTransform);
                vertex.tangent.Normalize();
            }
        }

        mesh->vertices.push_back(vertex);
    }

    // Index
    for (uint32 f = 0; f < srcMesh->mNumFaces; ++f)
    {
        aiFace& face = srcMesh->mFaces[f];

        for (uint32 k = 0; k < face.mNumIndices; ++k)
        {
            mesh->indices.push_back(face.mIndices[k] + startVertex);
        }
    }
    _meshes.push_back(mesh);

    ReadMeshBones(meshIndex, srcMesh);
}

void Converter::ReadMeshBones(uint32 meshIndex, const aiMesh* paiMesh)
{
    for (uint32 i = 0; i < paiMesh->mNumBones; i++)
    {
        ReadSingleBone(meshIndex, paiMesh->mBones[i]);
    }
}

void Converter::ReadSingleBone(uint32 meshIndex, const aiBone* pBone)
{
    string boneName = pBone->mName.C_Str();
    int boneIndex = GetBoneIndex(boneName);
    if (boneIndex < 0)
    {
        Matrix offsetMatrix = SkinnedMesh::ConvertMatrix(pBone->mOffsetMatrix);
        shared_ptr<asBone> bone = make_shared<asBone>();
        bone->index = _bones.size();
        bone->name = boneName;
        bone->offsetMatrix = offsetMatrix;
        _bones.push_back(bone);
    }
}

void Converter::WriteModelFile(wstring finalPath)
{
    auto path = filesystem::path(finalPath);
    filesystem::create_directory(path.parent_path());

    shared_ptr<FileUtils> fileUtils = make_shared<FileUtils>();
    fileUtils->Open(finalPath, FileMode::Write);

    // Bone Data
    fileUtils->Write<uint32>(_bones.size());
    for (shared_ptr<asBone> bone : _bones)
    {
        fileUtils->Write<int32>(bone->index);
        fileUtils->Write<string>(bone->name);
        fileUtils->Write<Matrix>(bone->offsetMatrix);
        fileUtils->Write<Matrix>(bone->localMatrix);
        fileUtils->Write<Matrix>(bone->globalMatrix);
        fileUtils->Write<int32>(bone->parentIndex);
    }

    // Mesh Data
    fileUtils->Write<uint32>(_meshes.size());
    for (shared_ptr<asMesh> meshData : _meshes)
    {
        fileUtils->Write<string>(meshData->name);
        fileUtils->Write<int32>(meshData->boneIndex);
        fileUtils->Write<string>(meshData->materialName);

        // Vertex Data
        fileUtils->Write<uint32>(meshData->vertices.size());
        fileUtils->Write(&meshData->vertices[0], sizeof(VertexType) * meshData->vertices.size());

        // Index Data
        fileUtils->Write<uint32>(meshData->indices.size());
        fileUtils->Write(&meshData->indices[0], sizeof(uint32) * meshData->indices.size());
    }
}

void Converter::ReadMaterialData()
{
    for (uint32 i = 0; i < _scene->mNumMaterials; i++)
    {
        aiMaterial* srcMaterial = _scene->mMaterials[i];

        shared_ptr<asMaterial> material = make_shared<asMaterial>();
        material->name = srcMaterial->GetName().C_Str();

        aiColor3D color;
        // Ambient
        srcMaterial->Get(AI_MATKEY_COLOR_AMBIENT, color);
        material->ambient = Color(color.r, color.g, color.b, 1.f);

        // Diffuse
        srcMaterial->Get(AI_MATKEY_COLOR_DIFFUSE, color);
        material->diffuse = Color(color.r, color.g, color.b, 1.f);

        // Specular
        srcMaterial->Get(AI_MATKEY_COLOR_SPECULAR, color);
        material->specular = Color(color.r, color.g, color.b, 1.f);
        srcMaterial->Get(AI_MATKEY_SHININESS, material->specular.w);

        // Emissive
        srcMaterial->Get(AI_MATKEY_COLOR_EMISSIVE, color);
        material->emissive = Color(color.r, color.g, color.b, 1.f);

        aiString file;
        // Diffuse Texture
        srcMaterial->GetTexture(aiTextureType_DIFFUSE, 0, &file);
        material->diffuseFile = file.C_Str();

        // Specular Texture
        srcMaterial->GetTexture(aiTextureType_SPECULAR, 0, &file);
        material->specularFile = file.C_Str();

        // Normal Texture
        srcMaterial->GetTexture(aiTextureType_NORMALS, 0, &file);
        material->normalFile = file.C_Str();

        _materials.push_back(material);
    }
}

void Converter::WriteMaterialData(const fs::path& assetPath, const fs::path& artifactPath, const vector<SubAssetInfo>& prev, OUT vector<SubAssetInfo>& exported)
{
    // 폴더가 없으면 만든다
    filesystem::create_directory(artifactPath);

    for (int i = 0; i < _materials.size(); i++)
    {
        shared_ptr<asMaterial>& asMaterial = _materials[i];
        unique_ptr<Material> material = make_unique<Material>();

        material->SetDiffuseMap(WriteTexture(asMaterial->diffuseFile, assetPath, artifactPath, prev, exported));
        material->SetSpecularMap(WriteTexture(asMaterial->specularFile, assetPath, artifactPath, prev, exported));
        material->SetNormalMap(WriteTexture(asMaterial->normalFile, assetPath, artifactPath, prev, exported));

        MaterialDesc& desc = material->GetMaterialDesc();
        desc.ambient = asMaterial->ambient;
        if (desc.ambient.x <= 0.001f && desc.ambient.y <= 0.001f && desc.ambient.z <= 0.001f)
            desc.ambient = Color(1.f, 1.f, 1.f, 1.f);
        desc.diffuse = asMaterial->diffuse;
        desc.specular = asMaterial->specular;
        desc.emissive = asMaterial->emissive;

        wstring assetName = Utils::ToWString(asMaterial->name) + L".mat";
        fs::path materialPath = artifactPath / assetName;

        unique_ptr<ResourceBase> resource = std::move(material);
        FileUtils::SaveResourceToJson(materialPath, resource);

        AddExported(prev, exported, assetName, ResourceType::Material);
    }
}

ResourceRef<Texture> Converter::WriteTexture(string file, const fs::path& assetPath, const fs::path& artifactFolder, const vector<SubAssetInfo>& prev, OUT vector<SubAssetInfo>& exported)
{
    if (file.empty())
        return ResourceRef<Texture>();
    string fileName = filesystem::path(file).filename().string();
    const aiTexture* srcTexture = _scene->GetEmbeddedTexture(fileName.c_str());
    // fbx 파일에 텍스쳐가 포함되어 있을 경우
    if (srcTexture)
    {
        const fs::path sourceName = fs::path(fileName);
        const wstring artifactName = sourceName.stem().wstring() + L".dds";
        const fs::path texturePath = artifactFolder / artifactName;
        DirectX::TexMetadata metadata = {};
        DirectX::ScratchImage image;
        HRESULT hr = E_FAIL;

        if (srcTexture->mHeight == 0)
        {
            const wstring ext = sourceName.extension().wstring();
            if (ext == L".dds" || ext == L".DDS")
            {
                hr = DirectX::LoadFromDDSMemory(
                    srcTexture->pcData, srcTexture->mWidth,
                    DirectX::DDS_FLAGS_NONE, &metadata, image);
            }
            else if (ext == L".tga" || ext == L".TGA")
            {
                hr = DirectX::LoadFromTGAMemory(
                    srcTexture->pcData, srcTexture->mWidth,
                    &metadata, image);
            }
            else
            {
                hr = DirectX::LoadFromWICMemory(
                    srcTexture->pcData, srcTexture->mWidth,
                    DirectX::WIC_FLAGS_NONE, &metadata, image);
            }
        }
        else
        {
            hr = image.Initialize2D(
                DXGI_FORMAT_B8G8R8A8_UNORM,
                srcTexture->mWidth, srcTexture->mHeight, 1, 1);
            if (SUCCEEDED(hr))
            {
                const DirectX::Image* destination = image.GetImage(0, 0, 0);
                const size_t sourceRowPitch = srcTexture->mWidth * sizeof(aiTexel);
                for (size_t y = 0; y < srcTexture->mHeight; ++y)
                {
                    memcpy(
                        destination->pixels + y * destination->rowPitch,
                        reinterpret_cast<const uint8_t*>(srcTexture->pcData) + y * sourceRowPitch,
                        sourceRowPitch);
                }
                metadata = image.GetMetadata();
            }
        }

        if (FAILED(hr))
        {
            DBG->LogErrorW(L"[Converter] Failed to load embedded texture: " + Utils::ToWString(fileName));
            return ResourceRef<Texture>();
        }

        const fs::path thumbnailPath = TextureMeta::GetThumbnailPathForArtifact(texturePath);
        HRESULT thumbnailHr = E_FAIL;
        hr = TextureMeta::SaveArtifacts(image, metadata, texturePath, thumbnailPath, false, &thumbnailHr);
        if (FAILED(hr))
        {
            DBG->LogErrorW(L"[Converter] Failed to build compressed embedded texture: " + texturePath.wstring());
            return ResourceRef<Texture>();
        }
        if (FAILED(thumbnailHr))
            DBG->LogErrorW(L"[Converter] Failed to build embedded texture thumbnail: " + thumbnailPath.wstring());

        AssetId assetId = AddExported(prev, exported, artifactName, ResourceType::Texture);
        return ResourceRef<Texture>(assetId);
    }
    else
    {
        AssetId assetId;
        fs::path parentPath = assetPath.parent_path();
        if (RESOURCES->SearchAssetIdByPath(parentPath, Utils::ToWString(fileName), OUT assetId))
        {
            // 에셋아이디 발견
        }
        else
        {
            assetId = AssetId::CreateAssetId();
            RESOURCES->GetAssetDatabase().ReserveAssetId(parentPath, Utils::ToWString(fileName), assetId);
            // 에셋아이디 등록
        }
        return ResourceRef<Texture>(assetId);
    }
}

shared_ptr<asAnimation> Converter::ReadAnimationData(aiAnimation* srcAnimation, wstring animationName)
{
    shared_ptr<asAnimation> animation = make_shared<asAnimation>();
    animation->name = Utils::ToString(animationName);
    animation->frameRate = (float)srcAnimation->mTicksPerSecond;

    uint32 duration = (uint32)srcAnimation->mDuration;
    animation->frameCount = duration + 1;

    for (uint32 i = 0; i < srcAnimation->mNumChannels; i++)
    {
        const aiNodeAnim* srcNode = srcAnimation->mChannels[i];

        shared_ptr<asKeyframe> keyframe = make_shared<asKeyframe>();
        keyframe->boneName = srcNode->mNodeName.C_Str();

        for (uint32 time = 0; time <= duration; time++)
        {
            asKeyframeData frameData;
            frameData.time = time;

            aiVector3D Scaling;
            SkinnedMesh::CalcInterpolatedScaling(Scaling, time, srcNode);
            frameData.scale = Vec3(Scaling.x, Scaling.y, Scaling.z);

            aiQuaternion RotationQ;
            SkinnedMesh::CalcInterpolatedRotation(RotationQ, time, srcNode);
            frameData.rotation = Quaternion(RotationQ.x, RotationQ.y, RotationQ.z, RotationQ.w);

            aiVector3D Translation;
            SkinnedMesh::CalcInterpolatedPosition(Translation, time, srcNode);
            frameData.translation = Vec3(Translation.x, Translation.y, Translation.z);

            keyframe->transforms.push_back(frameData);
        }
        animation->keyframes.push_back(keyframe);
    }

    return animation;
}

void Converter::DumpAnimationData(aiAnimation* srcAnimation, const fs::path& path)
{
    fs::create_directories(path);

    for (int channelIdx = 0; channelIdx < srcAnimation->mNumChannels; channelIdx++)
    {
        aiNodeAnim* channel = srcAnimation->mChannels[channelIdx];

        string boneName = channel->mNodeName.C_Str();
        auto it = boneName.find(":");
        if(it != string::npos)
            boneName.replace(boneName.find(":"), 1, "_");
        fs::path filePath = path / (boneName + string(".txt"));

        ofstream file(filePath);
        if (!file.is_open())
            continue;

        for (int positionIdx = 0; positionIdx < channel->mNumPositionKeys; positionIdx++)
        {
            auto positionKey = channel->mPositionKeys[positionIdx];

            file << "Time=" << positionKey.mTime << '\n';
            file << "Position: " << positionKey.mValue.x << ", " << positionKey.mValue.y << ", " << positionKey.mValue.z << "\n\n";
        }

        file << "============================" << "\n\n";

        for (int rotationIdx = 0; rotationIdx < channel->mNumRotationKeys; rotationIdx++)
        {
            auto rotationKey = channel->mRotationKeys[rotationIdx];
            Vec3 eulerRotation = Transform::ToEulerAngles(Quaternion(rotationKey.mValue.x, rotationKey.mValue.y, rotationKey.mValue.z, rotationKey.mValue.w));
            eulerRotation = MathUtils::RadToDeg(eulerRotation);
            file << "Time=" << rotationKey.mTime << '\n';
            file << "Rotation Quaternion: " << rotationKey.mValue.x << ", " << rotationKey.mValue.y << ", " << rotationKey.mValue.z << ", " << rotationKey.mValue.w << "\n";
            file << "Euler Rotation: " << eulerRotation.x << ", " << eulerRotation.y << ", " << eulerRotation.z << "\n\n";
        }

        for (int scaleIdx = 0; scaleIdx < channel->mNumScalingKeys; scaleIdx++)
        {
            auto scaleKey = channel->mScalingKeys[scaleIdx];
            file << "Time=" << scaleKey.mTime << '\n';
            file << "Scale: " << scaleKey.mValue.x << ", " << scaleKey.mValue.y << ", " << scaleKey.mValue.z << "\n\n";
        }

        file.close();
    }
}

/*
shared_ptr<asAnimation> Converter::ReadAnimationData(aiAnimation* srcAnimation)
{
    shared_ptr<asAnimation> animation = make_shared<asAnimation>();
    animation->name = srcAnimation->mName.C_Str();
    animation->frameRate = (float)srcAnimation->mTicksPerSecond;
    animation->frameCount = (uint32)srcAnimation->mDuration + 1;

    map<string, shared_ptr<asAnimationNode>> cacheAnimNodes;

    for (uint32 i = 0; i < srcAnimation->mNumChannels; i++)
    {
        aiNodeAnim* srcNode = srcAnimation->mChannels[i];

        // 애니메이션 노드 파싱
        shared_ptr<asAnimationNode> node = ParseAnimationNode(animation, srcNode);

        cacheAnimNodes[srcNode->mNodeName.C_Str()] = node;
    }

    ReadKeyframeData(animation, _scene->mRootNode, cacheAnimNodes);
    return animation;
}

std::shared_ptr<asAnimationNode> Converter::ParseAnimationNode(shared_ptr<asAnimation> animation, aiNodeAnim* srcNode)
{
    std::shared_ptr<asAnimationNode> node = make_shared<asAnimationNode>();
    node->name = srcNode->mNodeName;

    uint32 keyCount = max(max(srcNode->mNumPositionKeys, srcNode->mNumScalingKeys), srcNode->mNumRotationKeys);

    for (uint32 k = 0; k < keyCount; k++)
    {
        asKeyframeData frameData;

        bool found = false;
        uint32 t = node->keyframe.size();

        // Position
        if (::fabsf((float)srcNode->mPositionKeys[k].mTime - (float)t) <= 0.0001f)
        {
            aiVectorKey key = srcNode->mPositionKeys[k];
            frameData.time = (float)key.mTime;
            ::memcpy_s(&frameData.translation, sizeof(Vec3), &key.mValue, sizeof(aiVector3D));

            found = true;
        }

        // Rotation
        if (::fabsf((float)srcNode->mRotationKeys[k].mTime - (float)t) <= 0.0001f)
        {
            aiQuatKey key = srcNode->mRotationKeys[k];
            frameData.time = (float)key.mTime;

            frameData.rotation.x = key.mValue.x;
            frameData.rotation.y = key.mValue.y;
            frameData.rotation.z = key.mValue.z;
            frameData.rotation.w = key.mValue.w;

            found = true;
        }

        // Scale
        if (::fabsf((float)srcNode->mScalingKeys[k].mTime - (float)t) <= 0.0001f)
        {
            aiVectorKey key = srcNode->mScalingKeys[k];
            frameData.time = (float)key.mTime;
            ::memcpy_s(&frameData.scale, sizeof(Vec3), &key.mValue, sizeof(aiVector3D));

            found = true;
        }

        if (found == true)
            node->keyframe.push_back(frameData);
    }

    // Keyframe 늘려주기
    if (node->keyframe.size() < animation->frameCount)
    {
        uint32 count = animation->frameCount - node->keyframe.size();
        asKeyframeData keyFrame = node->keyframe.back();

        for (uint32 n = 0; n < count; n++)
            node->keyframe.push_back(keyFrame);
    }

    return node;
}

void Converter::ReadKeyframeData(shared_ptr<asAnimation> animation, aiNode* srcNode, map<string, shared_ptr<asAnimationNode>>& cache)
{
    shared_ptr<asKeyframe> keyframe = make_shared<asKeyframe>();
    keyframe->boneName = srcNode->mName.C_Str();

    shared_ptr<asAnimationNode> findNode = cache[keyframe->boneName];

    for (uint32 i = 0; i < animation->frameCount; i++)
    {
        asKeyframeData frameData;

        if (findNode == nullptr)
        {
            Matrix transform(srcNode->mTransformation[0]);
            transform = transform.Transpose();
            frameData.time = (float)i;
            transform.Decompose(OUT frameData.scale, OUT frameData.rotation, OUT frameData.translation);
        }
        else
        {
            frameData = findNode->keyframe[i];
        }
        keyframe->transforms.push_back(frameData);
    }

    animation->keyframes.push_back(keyframe);

    for (uint32 i = 0; i < srcNode->mNumChildren; i++)
    {
        ReadKeyframeData(animation, srcNode->mChildren[i], cache);
    }
}
*/
void Converter::WriteAnimationData(shared_ptr<asAnimation> animation, wstring finalPath)
{
    auto path = filesystem::path(finalPath);

    // 폴더가 없으면 만든다.
    filesystem::create_directory(path.parent_path());

    shared_ptr<FileUtils> file = make_shared<FileUtils>();
    file->Open(finalPath, FileMode::Write);

    file->Write<string>(animation->name);
    file->Write<float>(animation->frameRate);
    file->Write<uint32>(animation->frameCount);

    file->Write<uint32>(animation->keyframes.size());

    for (shared_ptr<asKeyframe> keyframe : animation->keyframes)
    {
        file->Write<string>(keyframe->boneName);

        file->Write<uint32>(keyframe->transforms.size());
        file->Write(&keyframe->transforms[0], sizeof(asKeyframeData) * keyframe->transforms.size());
    }
}

AssetId Converter::AddExported(const vector<SubAssetInfo>& prev, OUT vector<SubAssetInfo>& exported, wstring fileName, ResourceType resourceType)
{
    const auto matches = [&](const SubAssetInfo& info)
    {
        if (info.resourceType != resourceType)
            return false;
        if (info.fileName == fileName)
            return true;
        return resourceType == ResourceType::Texture
            && fs::path(info.fileName).stem() == fs::path(fileName).stem();
    };

    for (const SubAssetInfo& info : exported)
    {
        if (matches(info))
        {
            // Skip if already exported
            return info.assetId;
        }
    }

    AssetId assetId;
    for (const SubAssetInfo& info : prev)
    {
        if (matches(info))
        {
            assetId = info.assetId;
            break;
        }
    }
    if (assetId.IsValid() == false)
    {
        assetId = AssetId::CreateAssetId();
    }

    SubAssetInfo info = SubAssetInfo();
    info.fileName = fileName;
    info.resourceType = resourceType;
    info.assetId = assetId;
    exported.push_back(info);

    return assetId;
}

int Converter::GetBoneIndex(const string& name)
{
    for (shared_ptr<asBone>& bone : _bones)
    {
        if (bone->name == name)
            return bone->index;
    }

    return -1;
}

Matrix Converter::ConvertMatrix(const aiMatrix4x4& from)
{
    Matrix to;
    to._11 = from.a1; to._12 = from.b1; to._13 = from.c1; to._14 = from.d1;
    to._21 = from.a2; to._22 = from.b2; to._23 = from.c2; to._24 = from.d2;
    to._31 = from.a3; to._32 = from.b3; to._33 = from.c3; to._34 = from.d3;
    to._41 = from.a4; to._42 = from.b4; to._43 = from.c4; to._44 = from.d4;
    return to;
}

void Converter::ResolveImportTransform(const ModelImportSettings& importSettings)
{
    ModelImportPreset resolvedPreset = importSettings.preset;

    float scale = 1.0f;
    Vec3 rotation = Vec3::Zero;
    if (resolvedPreset == ModelImportPreset::Unreal)
    {
        scale = 0.01f;
        rotation.x = 90.0f;
    }
    else if (resolvedPreset == ModelImportPreset::Custom)
    {
        scale = max(importSettings.scale, 0.0001f);
        rotation = importSettings.rotation;
    }

    _applyImportTransform = resolvedPreset == ModelImportPreset::Unreal ||
        resolvedPreset == ModelImportPreset::Custom;
    _importTransform = Matrix::CreateScale(scale) *
        Matrix::CreateRotationX(XMConvertToRadians(rotation.x)) *
        Matrix::CreateRotationY(XMConvertToRadians(rotation.y)) *
        Matrix::CreateRotationZ(XMConvertToRadians(rotation.z));
}