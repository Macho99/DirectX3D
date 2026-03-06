#pragma once
#include "MonoBehaviour.h"

class CameraMove : public MonoBehaviour
{
    using Super = MonoBehaviour;
public:
	virtual void Start() override;
	virtual void Update() override;

	virtual bool OnGUI() override;

private:
	float _moveSpeed = 10.f;
	float _sprintSpeed = 25.f;
	float _mouseSpeed = 0.002f;
	POINT _prevMousePos = {};
};

