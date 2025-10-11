#pragma once
#include "Viewport.h"

class Ssao
{
public:
	Ssao();
	~Ssao();
	void OnSize(int32 width, int32 height, float fovy, float farZ);
	void Clear();
	void Draw();

	ComPtr<ID3D11ShaderResourceView> GetNormalDepthSRV() { return _normalDepthSRV; }
	shared_ptr<Texture> GetSsaoMap() { return _texture0; }
	void SetNormalDepthRenderTarget(ID3D11DepthStencilView* dsv);

private:
    void BuildFrustumCorners(float fovy, float farZ);
	void BuildQuad();
	void BuildTextureViews();
    void BuildOffsetVectors();
	void BlurAmbientMap(int32 blurCount);	
	void BlurAmbientMap(shared_ptr<Texture> inputTexture, ComPtr<ID3D11RenderTargetView> outputRTV, bool horzBlur);

private:
    unique_ptr<VertexBuffer> _screenQuadVB;
    unique_ptr<IndexBuffer> _screenQuadIB;
	//ComPtr<ID3D11Buffer> _screenQuadVB;
	//ComPtr<ID3D11Buffer> _screenQuadIB;

	ComPtr<ID3D11ShaderResourceView> _randomVectorSRV;
	ComPtr<ID3D11RenderTargetView> _normalDepthRTV;
	ComPtr<ID3D11ShaderResourceView> _normalDepthSRV;

	// Need two for ping-ponging during blur.
	ComPtr<ID3D11RenderTargetView> _ambientRTV0;
	ComPtr<ID3D11ShaderResourceView> _ambientSRV0;
	shared_ptr<Texture> _texture0;

	ComPtr<ID3D11RenderTargetView> _ambientRTV1;
	ComPtr<ID3D11ShaderResourceView> _ambientSRV1;
	shared_ptr<Texture> _texture1;

	uint32 _renderTargetWidth;
	uint32 _renderTargetHeight;

	//Vec4 _frustumFarCorner[4];
	//Vec4 _offsets[14];

	Viewport _ambientMapViewport;
	shared_ptr<Material> _ssaoMaterial;
	shared_ptr<Material> _blurMaterial;
	SsaoDesc _ssaoDesc;
};

