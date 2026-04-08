#include "pch.h"
#include "Mesh.h"
#include "GeometryHelper.h"

Mesh::Mesh() : Super(StaticType)
{

}

Mesh::~Mesh()
{

}

void Mesh::CreateQuad()
{
	_geometry = make_shared<Geometry<VertexTextureNormalTangentData>>();
	GeometryHelper::CreateQuad(_geometry);
	CreateBuffers();
}

void Mesh::CreateCube()
{
	_geometry = make_shared<Geometry<VertexTextureNormalTangentData>>();
	GeometryHelper::CreateCube(_geometry);
	CreateBuffers();
}

void Mesh::CreateGrid(int32 sizeX, int32 sizeZ)
{
	_geometry = make_shared<Geometry<VertexTextureNormalTangentData>>();
	GeometryHelper::CreateGrid(_geometry, sizeX, sizeZ);
	CreateBuffers();
}

void Mesh::CreateSphere()
{
	_geometry = make_shared<Geometry<VertexTextureNormalTangentData>>();
	GeometryHelper::CreateSphere(_geometry);
	CreateBuffers();
}

void Mesh::CreateFromGeometry(shared_ptr<Geometry<VertexTextureNormalTangentData>> geometry)
{
    _geometry = geometry;
    CreateBuffers();
}

void Mesh::CreateBuffers(const int indexCount)
{
	_vertexBuffer = make_shared<VertexBuffer>();
	_vertexBuffer->Create(_geometry->GetVertices(), "MeshVB");
	_indexBuffer = make_shared<IndexBuffer>();
	_indexBuffer->Create(_geometry->GetIndices(), indexCount);
}