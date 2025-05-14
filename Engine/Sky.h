#pragma once

class Sky
{
public:
	Sky(const std::wstring& cubemapFilename, const wstring& shaderFileName);
	~Sky();

	ComPtr<ID3D11ShaderResourceView> CubeMapSRV();

	void Render(const shared_ptr<Camera>& camera);

private:
	shared_ptr<VertexBuffer> _vb;
	shared_ptr<IndexBuffer> _ib;
	//ComPtr<ID3D11Buffer> _vb;
	//ComPtr<ID3D11Buffer> _ib;

	shared_ptr<class Texture> _texture;
	shared_ptr<class Material> _material;
	uint32 _indexCount;
};