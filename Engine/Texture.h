#pragma once
#include "ResourceBase.h"

class Texture : public ResourceBase
{
	using Super = ResourceBase;
public:
	Texture();
	~Texture();

	ComPtr<ID3D11ShaderResourceView> GetComPtr() { return _shaderResourveView; }

	virtual void Load(const wstring& path) override;

	ComPtr<ID3D11Texture2D> GetTexture2D() const;
	void SetSRV(ComPtr<ID3D11ShaderResourceView> srv);

    void SetSize(Vec2 size) { _size = size; }
	Vec2 GetSize() const { return _size; }

	const DirectX::ScratchImage& GetInfo() { return _img; }

private:
	ComPtr<ID3D11ShaderResourceView> _shaderResourveView;
	Vec2 _size = {1.f, 1.f};
	DirectX::ScratchImage _img = {};
};

