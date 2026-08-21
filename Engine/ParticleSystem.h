#pragma once
#include "Renderer.h"

#define MAX_PARTICLES 10000

struct ParticleVertex
{
	Vec3 InitialPos;
	Vec3 InitialVel;
	Vec2 Size;
	float Age;
	uint32 Type;
};

#define PARTICLE_SYSTEM_MODE_LIST(X) \
    X(Loop)                         \
    X(Disable)                      \
    X(Destroy)

enum class ParticleSystemMode : uint8
{
#define X(name) name,
    PARTICLE_SYSTEM_MODE_LIST(X)
#undef X

    Max
};

static const char* ParticleSystemModeNames[] =
{
#define X(name) #name,
    PARTICLE_SYSTEM_MODE_LIST(X)
#undef X
};

class ParticleSystem : public Renderer
{
	using Super = Renderer;
    DECLARE_COMPONENT(ParticleSystem)
public:
	ParticleSystem();
	~ParticleSystem();

	void Reset();

	void Update() override;
	void InnerRender(RenderTech renderTech) override;

	void SetEmitPosW(Vec3 emitPosW) { _desc.emitPosW = emitPosW; }
	void SetEmitDirW(Vec3 emitDirW) { _desc.emitDirW = emitDirW; }

	virtual void OnDisable() override;
    virtual bool TryInitialize() override;
    virtual bool OnGUI() override;
    virtual int GetVersion() const override { return Super::GetVersion() + 2; }
    template<typename Archive>
    void serialize(Archive& ar)
    {
        Super::serialize(ar);
        ar(cereal::make_nvp("_emitDirW", _desc.emitDirW));

        if (_version >= 1)
        {
            ar(CEREAL_NVP(_mode));
        }

		if (_version >= 2)
		{
			ar(CEREAL_NVP(_desc));
		}
    }

private:
	void BuildVB();

private:
	ParticleDesc _desc;
	ParticleSystemMode _mode = ParticleSystemMode::Loop;
	bool _firstRun;
	float _debugAge;

	//ComPtr<ID3D11Buffer> _initVB;
	//ComPtr<ID3D11Buffer> _drawVB;
	//ComPtr<ID3D11Buffer> _streamOutVB;
	shared_ptr<VertexBuffer> _initVB;
	shared_ptr<VertexBuffer> _drawVB;
	shared_ptr<VertexBuffer> _streamOutVB;

	ComPtr<ID3D11InputLayout> _inputLayout;

    ResourceRef<Material> _cachedMaterial; // 메테리얼 세팅
};

const D3D11_INPUT_ELEMENT_DESC ParticleInputDesc[5] =
{
	{"POSITION", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0,  0, D3D11_INPUT_PER_VERTEX_DATA, 0},
	{"VELOCITY", 0, DXGI_FORMAT_R32G32B32_FLOAT,	0, 16, D3D11_INPUT_PER_VERTEX_DATA, 0},
	{"SIZE",     0, DXGI_FORMAT_R32G32_FLOAT,		0, 28, D3D11_INPUT_PER_VERTEX_DATA, 0},
	{"AGE",      0, DXGI_FORMAT_R32_FLOAT,			0, 34, D3D11_INPUT_PER_VERTEX_DATA, 0},
	{"TYPE",     0, DXGI_FORMAT_R32_UINT,			0, 40, D3D11_INPUT_PER_VERTEX_DATA, 0},
};
