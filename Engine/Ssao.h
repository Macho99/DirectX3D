#pragma once
#include "Viewport.h"
#include "Blur.h"

class Ssao
{
public:
	Ssao();
	~Ssao();
	void OnSize(int32 width, int32 height, float fovy, float farZ);
	void Clear();
	void Draw();

	ComPtr<ID3D11ShaderResourceView> GetNormalDepthSRV() { return _normalDepthSRV; }
	ResourceRef<Texture> GetSsaoMap() { return _blur.GetDebugTexture(); }
	void SetNormalDepthRenderTarget(ID3D11DepthStencilView* dsv);

private:
    void BuildFrustumCorners(float fovy, float farZ);
	void BuildQuad();
	void BuildTextureViews();
    void BuildOffsetVectors();

private:
	Blur _blur;

    unique_ptr<VertexBuffer> _screenQuadVB;
    unique_ptr<IndexBuffer> _screenQuadIB;
	//ComPtr<ID3D11Buffer> _screenQuadVB;
	//ComPtr<ID3D11Buffer> _screenQuadIB;

	ComPtr<ID3D11ShaderResourceView> _randomVectorSRV;
	ComPtr<ID3D11RenderTargetView> _normalDepthRTV;
	ComPtr<ID3D11ShaderResourceView> _normalDepthSRV;

	uint32 _renderTargetWidth;
	uint32 _renderTargetHeight;

	//Vec4 _frustumFarCorner[4];
	//Vec4 _offsets[14];

	Viewport _ambientMapViewport;
	ResourceRef<Material> _ssaoMaterial;
	SsaoDesc _ssaoDesc;
};

