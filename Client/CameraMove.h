#pragma once
#include "MonoBehaviour.h"

class CameraMove : public MonoBehaviour
{
public:
	virtual void Start() override;
	virtual void Update() override;

private:
	float _moveSpeed = 7.f;
	float _sprintSpeed = 14.f;
	float _mouseSpeed = 0.2f;
	POINT _prevMousePos;
};

