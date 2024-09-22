#pragma once
#include "IExecute.h"
#include "Geometry.h"

class GameObject;

class GlobalTestDemo : public IExecute
{
public:
	void Init() override;
	void Update() override;
	void Render() override;

private:
	shared_ptr<Shader> _shader;

	// Object
	shared_ptr<GameObject> _obj;
	// Camera
	shared_ptr<GameObject> _camera;

	//Vec3 _lightDir = Vec3(0.f, -1.f, 0.f);
};

