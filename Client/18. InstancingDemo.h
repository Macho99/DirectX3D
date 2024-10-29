#pragma once
#include "IExecute.h"
#include "Geometry.h"

class GameObject;

class InstancingDemo : public IExecute
{
public:
	void Init() override;
	void Update() override;
	void Render() override;

private:
	shared_ptr<Shader> _shader;

	shared_ptr<GameObject> _camera;
	vector<shared_ptr<GameObject>> _objs;
};

