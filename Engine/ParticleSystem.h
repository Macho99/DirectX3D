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

class ParticleSystem : public Renderer
{
	using Super = Renderer;
public:
	ParticleSystem();
	~ParticleSystem();

	void Reset();

	void Update() override;
	void Render() override;

	void SetMaterial(shared_ptr<Material> material) override;

	void SetEmitPosW(Vec3 emitPosW) { _emitPosW = emitPosW; }
	void SetEmitDirW(Vec3 emitDirW) { _emitDirW = emitDirW; }

private:
	void BuildVB();

private:
	ParticleDesc _desc;
	bool _firstRun;
	float _age;
	float _timeStep;
	float _gameTime;
	Vec3 _emitPosW;
	Vec3 _emitDirW;

	//ComPtr<ID3D11Buffer> _initVB;
	//ComPtr<ID3D11Buffer> _drawVB;
	//ComPtr<ID3D11Buffer> _streamOutVB;
	shared_ptr<VertexBuffer> _initVB;
	shared_ptr<VertexBuffer> _drawVB;
	shared_ptr<VertexBuffer> _streamOutVB;
};

