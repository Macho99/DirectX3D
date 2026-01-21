#include "pch.h"
#include "ParticleSystem.h"
#include "Shader.h"
#include "Material.h"
#include "Windows.h"

ParticleSystem::ParticleSystem()
	: Super(ComponentType::ParticleSystem)
{
	BuildVB();

	//D3DX11_PASS_DESC passDesc;
	//shader->Effect()->GetTechniqueByIndex(0)->GetPassByIndex(0)->GetDesc(&passDesc);
	//CHECK(DEVICE->CreateInputLayout(ParticleInputDesc, 5, passDesc.pIAInputSignature,
	//	passDesc.IAInputSignatureSize, &_inputLayout));
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
	_emitPosW = GetTransform()->GetPosition();
	_timeStep = TIME->GetDeltaTime();
	_gameTime = TIME->GetGameTime();

	_age += _timeStep;
}

void ParticleSystem::InnerRender(RenderTech renderTech)
{
	// TODO: not Implemented
	assert(renderTech == RenderTech::Draw);

	Super::InnerRender(renderTech);

	Shader* shader = _material->GetShader();
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
	//DC->IASetInputLayout(_inputLayout.Get());
	DC->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);

	// On the first pass, use the initialization VB.  Otherwise, use
	// the VB that contains the current particle list.	
	uint32 stride = sizeof(ParticleVertex);
	uint32 offset = 0;
	if (_firstRun)
		_initVB->PushData();
		//DC->IASetVertexBuffers(0, 1, _initVB->GetComPtr().GetAddressOf(), &stride, &offset);
	else
		_drawVB->PushData();
		//DC->IASetVertexBuffers(0, 1, _drawVB->GetComPtr().GetAddressOf(), &stride, &offset);

	//
	// Draw the current particle list using stream-out only to update them.  
	// The updated vertices are streamed-out to the target VB. 
	//

	shader->BeginDraw(0, 0);
	DC->SOSetTargets(1, _streamOutVB->GetComPtr().GetAddressOf(), &offset);

	//D3DX11_TECHNIQUE_DESC techDesc;
	//shader->Effect()->GetTechniqueByIndex(0)->GetDesc(&techDesc);
	//shader->Effect()->GetTechniqueByIndex(0)->GetPassByIndex(0)->Apply(0, DC.Get());
	if (_firstRun)
	{
		DC->Draw(1, 0);
		_firstRun = false;
	}
	else
	{
		DC->DrawAuto();
	}
	shader->EndDraw(0, 0);
	// ping-pong the vertex buffers
	std::swap(_drawVB, _streamOutVB);
	// done streaming-out--unbind the vertex buffer

	ID3D11Buffer* bufferArray[1] = { 0 };
	//DC->SOSetTargets(1, bufferArray, &offset);

	shader->BeginDraw(1, 0);
	// done streaming-out--unbind the vertex buffer
	//ID3D11Buffer* bufferArray[1] = { 0 };
	DC->SOSetTargets(1, bufferArray, &offset);


	//
	// Draw the updated particle system we just streamed-out. 
	//
	//DC->IASetVertexBuffers(0, 1, _drawVB->GetComPtr().GetAddressOf(), &stride, &offset);
	_drawVB->PushData();

	//shader->Effect()->GetTechniqueByIndex(1)->GetDesc(&techDesc);
	//shader->Effect()->GetTechniqueByIndex(1)->GetPassByIndex(0)->Apply(0, DC.Get());
	DC->DrawAuto();
	shader->EndDraw(1, 0);
}

void ParticleSystem::SetMaterial(shared_ptr<Material> material)
{
	Super::SetMaterial(material);
	material->SetRandomTex(RESOURCES->Get<Texture>(L"RandomTex"));
	material->SetCastShadow(false);
    material->GetShader()->SetTechNum(RenderTech::Draw, 1);
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
	_initVB->Create(vertices, "ParticleInitVB", 0, false, true);

	_drawVB = make_shared<VertexBuffer>();
	_drawVB->CreateStreamOut<ParticleVertex>(MAX_PARTICLES);

	_streamOutVB = make_shared<VertexBuffer>();
	_streamOutVB->CreateStreamOut<ParticleVertex>(MAX_PARTICLES);
}
