#pragma once
#include "IExecute.h"
#include "Geometry.h"

class GameObject;

class ParticleDemo : public IExecute
{
public:
	void Init() override;
	void Update() override;
	void Render() override;

private:
	shared_ptr<Shader> _shader;
};