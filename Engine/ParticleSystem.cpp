#include "pch.h"
#include "ParticleSystem.h"
#include "Shader.h"
#include "Material.h"

ParticleSystem::ParticleSystem()
	: Super(ComponentType::ParticleSystem)
{
	BuildVB();
}

ParticleSystem::~ParticleSystem()
{
}

void ParticleSystem::Reset()
{
	_firstRun = true;
	_age = 0.0f;
}

void ParticleSystem::Update()
{
	_timeStep = TIME->GetDeltaTime();
	_gameTime = TIME->GetGameTime();

	_age += _timeStep;
}

void ParticleSystem::Render()
{
	Super::Render();

	const shared_ptr<Shader>& shader = _material->GetShader();
	//
	// Set constants.
	//
	//_fx->SetViewProj(VP);
	//_fx->SetGameTime(_gameTime);
	//_fx->SetTimeStep(_timeStep);
	//_fx->SetEyePosW(_eyePosW);
	//_fx->SetEmitPosW(_emitPosW);
	//_fx->SetEmitDirW(_emitDirW);
	//_fx->SetTexArray(_texArraySRV.Get());
	//_fx->SetRandomTex(_randomTexSRV.Get());

	_desc.timeStep = _timeStep;
	_desc.gameTime = _gameTime;
	_desc.emitDirW = _emitDirW;
	_desc.emitPosW = _emitPosW;
	shader->PushParticleData(_desc);

	//
	// Set IA stage.
	//
	//dc->IASetInputLayout(InputLayouts::Particle.Get());
	DC->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);

	// On the first pass, use the initialization VB.  Otherwise, use
	// the VB that contains the current particle list.
	if (_firstRun)
		_initVB->PushData();
		//dc->IASetVertexBuffers(0, 1, _initVB.GetAddressOf(), &stride, &offset);
	else
		_drawVB->PushData();
		//dc->IASetVertexBuffers(0, 1, _drawVB.GetAddressOf(), &stride, &offset);

	//
	// Draw the current particle list using stream-out only to update them.  
	// The updated vertices are streamed-out to the target VB. 
	//
	uint32 offset = 0;
	DC->SOSetTargets(1, _streamOutVB->GetComPtr().GetAddressOf(), &offset);

	D3DX11_TECHNIQUE_DESC techDesc;
	shader->Effect()->GetTechniqueByIndex(0)->GetDesc(&techDesc);
	shader->Effect()->GetTechniqueByIndex(0)->GetPassByIndex(0)->Apply(0, DC.Get());
	if (_firstRun)
	{
		shader->Draw(0, 0, 1, 0);
		_firstRun = false;
	}
	else
	{
		DC->DrawAuto();
	}

	// done streaming-out--unbind the vertex buffer
	ID3D11Buffer* bufferArray[1] = { 0 };
	DC->SOSetTargets(1, bufferArray, &offset);

	// ping-pong the vertex buffers
	std::swap(_drawVB, _streamOutVB);

	//
	// Draw the updated particle system we just streamed-out. 
	//
	//dc->IASetVertexBuffers(0, 1, _drawVB.GetAddressOf(), &stride, &offset);
	_drawVB->PushData();

	shader->Effect()->GetTechniqueByIndex(1)->GetDesc(&techDesc);
	shader->Effect()->GetTechniqueByIndex(1)->GetPassByIndex(0)->Apply(0, DC.Get());
	DC->DrawAuto();
}

void ParticleSystem::SetMaterial(shared_ptr<Material> material)
{
	Super::SetMaterial(material);
	material->SetRandomTex(RESOURCES->Get<Texture>(L"RandomTex"));
}

void ParticleSystem::BuildVB()
{
	//
	// Create the buffer to kick-off the particle system.
	//

	/*
	D3D11_BUFFER_DESC vbd;
	vbd.Usage = D3D11_USAGE_DEFAULT;
	vbd.ByteWidth = sizeof(ParticleVertex) * 1;
	vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vbd.CPUAccessFlags = 0;
	vbd.MiscFlags = 0;
	vbd.StructureByteStride = 0;

	// The initial particle emitter has type 0 and age 0.  The rest
	// of the particle attributes do not apply to an emitter.
	ParticleVertex p;
	ZeroMemory(&p, sizeof(ParticleVertex));
	p.Age = 0.0f;
	p.Type = 0;

	D3D11_SUBRESOURCE_DATA vinitData;
	vinitData.pSysMem = &p;

	HR(device->CreateBuffer(&vbd, &vinitData, _initVB.GetAddressOf()));

	//
	// Create the ping-pong buffers for stream-out and drawing.
	//
	vbd.ByteWidth = sizeof(ParticleVertex) * MAX_PARTICLES;
	vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER | D3D11_BIND_STREAM_OUTPUT;

	HR(device->CreateBuffer(&vbd, 0, _drawVB.GetAddressOf()));
	HR(device->CreateBuffer(&vbd, 0, _streamOutVB.GetAddressOf()));
	*/

	ParticleVertex p;
	ZeroMemory(&p, sizeof(ParticleVertex));
	p.Age = 0.0f;
	p.Type = 0;

	vector<ParticleVertex> vertices;
	vertices.push_back(p);
	_initVB = make_shared<VertexBuffer>();
	_initVB->Create(vertices, 0, false, true);

	_drawVB = make_shared<VertexBuffer>();
	_drawVB->CreateStreamOut<ParticleVertex>(MAX_PARTICLES);

	_streamOutVB = make_shared<VertexBuffer>();
	_streamOutVB->CreateStreamOut<ParticleVertex>(MAX_PARTICLES);
}
