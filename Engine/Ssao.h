#pragma once
#include "Viewport.h"

class Ssao
{
public:
	Ssao();
	~Ssao();
	void OnSize(int32 width, int32 height, float fovy, float farZ);

	ComPtr<ID3D11ShaderResourceView> GetNormalDepthSRV() { return _normalDepthSRV; }
	void SetNormalDepthRenderTarget(ID3D11DepthStencilView* dsv);

private:
    void BuildFrustumCorners(float fovy, float farZ);

	void BuildQuad();
	void BuildTextureViews();
    void BuildOffsetVectors();


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
	ComPtr<ID3D11RenderTargetView> _ambientRTV1;
	ComPtr<ID3D11ShaderResourceView> _ambientSRV1;

	uint32 _renderTargetWidth;
	uint32 _renderTargetHeight;

	Vec4 _frustumFarCorner[4];
	Vec4 _offsets[14];

	Viewport _ambientMapViewport;
};

