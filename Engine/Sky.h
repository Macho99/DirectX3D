#pragma once
#include "Material.h"

class Sky
{
public:
	Sky();
	~Sky();

    void SetMaterial(ResourceRef<class Material> material) { _material = material; }

	void Render(Camera* camera);

    template<class Archive>
    void serialize(Archive& ar)
    {
		ar(CEREAL_NVP(_material));
    }

private:
	ResourceRef<Material> _material;

	shared_ptr<VertexBuffer> _vb;
	shared_ptr<IndexBuffer> _ib;
	//ComPtr<ID3D11Buffer> _vb;
	//ComPtr<ID3D11Buffer> _ib;

	uint32 _indexCount;
};