#pragma once
#include "MonoBehaviour.h"

class CameraMove : public MonoBehaviour
{
public:
	virtual void Start() override;
	virtual void Update() override;

private:
	float _moveSpeed = 10.f;
};

