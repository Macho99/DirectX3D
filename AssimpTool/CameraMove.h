#pragma once
#include "MonoBehaviour.h"

class CameraMove : public MonoBehaviour
{
public:
	virtual void Start() override;
	virtual void Update() override;

private:
	float _speed = 10.f;
};

