#pragma once

struct ModelBone
{
	wstring name;
	int32 index;
	Matrix offsetMatrix;
	Matrix localMatrix;
	Matrix globalMatrix = Matrix::Identity;

    int32 parentIndex = -1;
};

struct ModelMesh
{
	void CreateBuffers();

	wstring name;

	// Mesh
	shared_ptr<Geometry<ModelVertexType>> geometry = make_shared<Geometry<ModelVertexType>>();
	shared_ptr<VertexBuffer> vertexBuffer;
	shared_ptr<IndexBuffer> indexBuffer;

	// Material
	wstring materialName = L"";
	ResourceRef<Material> material; // Cache

	// Bones
	int32 boneIndex;
	shared_ptr<ModelBone> bone; // Cache
};