#pragma once
#include "ResourceBase.h"
#include "Geometry.h"

class Mesh : public ResourceBase
{
    using Super = ResourceBase;

public:
	static constexpr ResourceType StaticType = ResourceType::Mesh;
	static string GetExtension() { return ".mesh"; }
    Mesh();
    virtual ~Mesh();

	void CreateQuad();
	void CreateCube();
	void CreateGrid(int32 sizeX, int32 sizeZ);
	void CreateSphere();
    void CreateFromGeometry(shared_ptr<Geometry<VertexTextureNormalTangentData>> geometry);

	shared_ptr<VertexBuffer> GetVertexBuffer() { return _vertexBuffer; }
	shared_ptr<IndexBuffer> GetIndexBuffer() { return _indexBuffer; }

	shared_ptr<Geometry<VertexTextureNormalTangentData>> GetGeometry() { return _geometry; }

    template<class Archive>
    void serialize(Archive& ar)
    {
        Super::serialize(ar);
        ar(CEREAL_NVP(_geometry));

        if (Archive::is_loading::value)
			CreateBuffers();
    }

	void CreateBuffers(const int indexCount = -1);

private:
	// Mesh
	shared_ptr<Geometry<VertexTextureNormalTangentData>> _geometry;

	shared_ptr<VertexBuffer> _vertexBuffer;
	shared_ptr<IndexBuffer> _indexBuffer;
};

