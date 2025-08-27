#pragma once
class PostProcess
{
public:
    PostProcess();
    virtual ~PostProcess() = default;

public:
    virtual void Render(ComPtr<ID3D11ShaderResourceView> srv, ComPtr<ID3D11RenderTargetView> rtv);
    void SetEnabled(bool enabled) { _isEnabled = enabled; }
    bool IsEnabled() const { return _isEnabled; }

protected:
    void DrawQuad(Material* material);

private:
    bool _isEnabled = true;
    shared_ptr<Mesh> _mesh = nullptr;
};