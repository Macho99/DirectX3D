#pragma once
class PostProcess
{
public:
    PostProcess();
    virtual ~PostProcess() = default;

public:
    virtual void SetHDR_SRV(ComPtr<ID3D11ShaderResourceView> srv) {}
    virtual void Render(ComPtr<ID3D11RenderTargetView> rtv);
    void SetEnabled(bool enabled) { _isEnabled = enabled; }
    bool IsEnabled() const { return _isEnabled; }
    virtual void OnSize(UINT width, UINT height) {}
    virtual void SetDebugTextureSRV(ResourceRef<Texture> texture) {}

protected:
    void DrawQuad(Material* material);

private:
    bool _isEnabled = true;
    ResourceRef<Mesh> _mesh;
};