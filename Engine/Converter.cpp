#include "pch.h"
#include "Converter.h"
#include <filesystem>
#include "Utils.h"
#include "FileUtils.h"
#include "Material.h"

Converter::Converter()
{
    _importer = make_shared<Assimp::Importer>();
}

Converter::~Converter()
{
}

void Converter::ReadAssetFile(wstring file)
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
}

void Converter::ExportModelData(wstring savePath)
{
    wstring finalPath = savePath + L".mesh";
    ReadModelData(_scene->mRootNode, -1, -1);
    ReadSkinData();

    //Write CSV File
    {
        FILE* file;
        ::fopen_s(&file, "../Vertices.csv", "w");

        for (shared_ptr<asBone>& bone : _bones)
        {
            string name = bone->name;
            ::fprintf(file, "%d,%s\n", bone->index, bone->name.c_str());
        }

        ::fprintf(file, "\n");

        for (shared_ptr<asMesh>& mesh : _meshes)
        {
            string name = mesh->name;
            ::printf("%s\n", name.c_str());

            for (UINT i = 0; i < mesh->vertices.size(); i++)
            {
                Vec3 p = mesh->vertices[i].position;
                Vec4 indices = mesh->vertices[i].blendIndices;
                Vec4 weights = mesh->vertices[i].blendWeights;

                ::fprintf(file, "%f,%f,%f,", p.x, p.y, p.z);
                ::fprintf(file, "%f,%f,%f,%f,", indices.x, indices.y, indices.z, indices.w);
                ::fprintf(file, "%f,%f,%f,%f\n", weights.x, weights.y, weights.z, weights.w);
            }
        }

        ::fclose(file);
    }

    WriteModelFile(finalPath);
}

//void Converter::ExportMaterialData(wstring savePath)
//{
//	wstring finalPath = savePath + L".mat";
//	ReadMaterialData();
//	WriteMaterialData(finalPath);
//}

void Converter::ExportAnimationData(wstring savePath, uint32 index)
{
    wstring finalPath = savePath + L".clip";
    assert(index < _scene->mNumAnimations);

    shared_ptr<asAnimation> animation = ReadAnimationData(_scene->mAnimations[index]);
    WriteAnimationData(animation, finalPath);
}

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
        ReadModelData(_scene->mRootNode, -1, -1);
        ReadSkinData();
        if (_meshes.size() > 0)
        {
            SubAssetInfo info = SubAssetInfo();
            wstring assetName = Utils::ToWString(_meshes[0]->name) + L".mesh";
            wstring finalPath = artifactPath + L"\\" + assetName;
            WriteModelFile(finalPath);
            AddExported(prev, exported, assetName, ResourceType::ModelMesh);
        }
    }

    {
        for (uint32 index = 0; index < _scene->mNumAnimations; ++index)
        {
            shared_ptr<asAnimation> animation = ReadAnimationData(_scene->mAnimations[index]);

            SubAssetInfo info = SubAssetInfo();
            wstring assetName = Utils::ToWString(animation->name) + L".clip";
            wstring finalPath = artifactPath + L"\\" + assetName;
            WriteAnimationData(animation, finalPath);
            AddExported(prev, exported, assetName, ResourceType::Animation);
        }
    }
}

void Converter::ReadModelData(aiNode* node, int32 index, int32 parent)
{
    shared_ptr<asBone> bone = make_shared<asBone>();
    bone->index = index;
    bone->parent = parent;
    bone->name = node->mName.C_Str();

    // Relative Transform (Parent)
    Matrix transform(node->mTransformation[0]);
    bone->transform = transform.Transpose();

    // Local Transform (Root)
    Matrix matParent = Matrix::Identity;
    if (parent >= 0)
    {
        matParent = _bones[parent]->transform;
    }
    bone->transform = bone->transform * matParent;

    _bones.push_back(bone);

    ReadMeshData(node, index);

    for (uint32 i = 0; i < node->mNumChildren; ++i)
    {
        ReadModelData(node->mChildren[i], _bones.size(), index);
    }
}

void Converter::ReadMeshData(aiNode* node, int32 bone)
{
    if (node->mNumMeshes < 1)
        return;

    for (uint32 i = 0; i < node->mNumMeshes; ++i)
    {
        shared_ptr<asMesh> mesh = make_shared<asMesh>();
        mesh->name = node->mName.C_Str();
        mesh->boneIndex = bone;

        uint32 index = node->mMeshes[i];
        const aiMesh* srcMesh = _scene->mMeshes[index];

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
    }
}

void Converter::ReadSkinData()
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
        fileUtils->Write<int32>(bone->parent);
        fileUtils->Write<Matrix>(bone->transform);
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
        Material material;

        material.SetDiffuseMap(WriteTexture(asMaterial->diffuseFile, assetPath, artifactPath, prev, exported));
        material.SetSpecularMap(WriteTexture(asMaterial->specularFile, assetPath, artifactPath, prev, exported));
        material.SetNormalMap(WriteTexture(asMaterial->normalFile, assetPath, artifactPath, prev, exported));

        MaterialDesc& desc = material.GetMaterialDesc();
        desc.ambient = asMaterial->ambient;
        desc.diffuse = asMaterial->diffuse;
        desc.specular = asMaterial->specular;
        desc.emissive = asMaterial->emissive;

        wstring assetName = Utils::ToWString(asMaterial->name) + L".mat";
        AddExported(prev, exported, assetName, ResourceType::Material);
    }
}

ResourceRef<Texture> Converter::WriteTexture(string fileName, const fs::path& assetPath, const fs::path& artifactFolder, const vector<SubAssetInfo>& prev, OUT vector<SubAssetInfo>& exported)
{
    const aiTexture* srcTexture = _scene->GetEmbeddedTexture(fileName.c_str());
    // fbx 파일에 텍스쳐가 포함되어 있을 경우
    if (srcTexture)
    {
        string pathStr = (artifactFolder / fileName).string();

        if (srcTexture->mHeight == 0)
        {
            shared_ptr<FileUtils> file = make_shared<FileUtils>();
            file->Open(Utils::ToWString(pathStr), FileMode::Write);
            file->Write(srcTexture->pcData, srcTexture->mWidth);
        }
        else
        {
            D3D11_TEXTURE2D_DESC desc;
            ZeroMemory(&desc, sizeof(D3D11_TEXTURE2D_DESC));
            desc.Width = srcTexture->mWidth;
            desc.Height = srcTexture->mHeight;
            desc.MipLevels = 1;
            desc.ArraySize = 1;
            desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
            desc.SampleDesc.Count = 1;
            desc.SampleDesc.Quality = 0;
            desc.Usage = D3D11_USAGE_IMMUTABLE;

            D3D11_SUBRESOURCE_DATA subResource = { 0 };
            subResource.pSysMem = srcTexture->pcData;

            ComPtr<ID3D11Texture2D> texture;
            HRESULT hr = DEVICE->CreateTexture2D(&desc, &subResource, texture.GetAddressOf());
            CHECK(hr);

            DirectX::ScratchImage img;
            ::CaptureTexture(DEVICE.Get(), DC.Get(), texture.Get(), img);

            // Save To File
            hr = DirectX::SaveToDDSFile(*img.GetImages(), DirectX::DDS_FLAGS_NONE, Utils::ToWString(fileName).c_str());
            CHECK(hr);
        }

        AssetId assetId = AddExported(prev, exported, Utils::ToWString(fileName), ResourceType::Texture);
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

        // 현재 찾은 노드 중에 제일 긴 시간으로 애니메이션 시간 갱신
        animation->duration = max(animation->duration, node->keyframe.back().time);

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

void Converter::WriteAnimationData(shared_ptr<asAnimation> animation, wstring finalPath)
{
    auto path = filesystem::path(finalPath);

    // 폴더가 없으면 만든다.
    filesystem::create_directory(path.parent_path());

    shared_ptr<FileUtils> file = make_shared<FileUtils>();
    file->Open(finalPath, FileMode::Write);

    file->Write<string>(animation->name);
    file->Write<float>(animation->duration);
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
    AssetId assetId;
    for (const SubAssetInfo& info : prev)
    {
        if (info.fileName == fileName && info.resourceType == resourceType)
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

uint32 Converter::GetBoneIndex(const string& name)
{
    for (shared_ptr<asBone>& bone : _bones)
    {
        if (bone->name == name)
            return bone->index;
    }

    assert(false);
    return 0;
}
