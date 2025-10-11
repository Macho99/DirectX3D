#pragma once
class Blur
{
public:
    Blur();
    ~Blur();

    shared_ptr<Texture> GetDebugTexture() { return _texture0; }
    void OnSize(int32 width, int32 height);

    ComPtr<ID3D11RenderTargetView> GetRTV() { return _ambientRTV0; }
    ID3D11Texture2D* GetTexture2D() { return _ambientTexture0.Get(); }

	void ProcessBlur(int32 blurCount);

private:
	void ProcessBlur(shared_ptr<Texture> inputTexture, ComPtr<ID3D11RenderTargetView> outputRTV, bool horzBlur);

private:
    unique_ptr<VertexBuffer> _screenQuadVB;
    unique_ptr<IndexBuffer> _screenQuadIB;

	// Need two for ping-ponging during blur.
    ComPtr<ID3D11Texture2D> _ambientTexture0;
	ComPtr<ID3D11RenderTargetView> _ambientRTV0;
	ComPtr<ID3D11ShaderResourceView> _ambientSRV0;
	shared_ptr<Texture> _texture0;

    ComPtr<ID3D11Texture2D> _ambientTexture1;
	ComPtr<ID3D11RenderTargetView> _ambientRTV1;
	ComPtr<ID3D11ShaderResourceView> _ambientSRV1;
	shared_ptr<Texture> _texture1;

    shared_ptr<Material> _blurMat;
    BlurDesc _blurDesc;
};