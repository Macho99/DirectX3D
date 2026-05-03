#pragma once
#include "MonoBehaviour.h"

class CameraMove : public MonoBehaviour
{
    using Super = MonoBehaviour;
	DECLARE_MONO_BEHAVIOUR(CameraMove)
public:
	virtual void Start() override;
	virtual void Update() override;

	virtual bool OnGUI() override;

private:
    void MoveFocusTarget();

private:
	float _moveSpeed = 10.f;
	float _sprintSpeed = 25.f;
	float _mouseSpeed = 0.1f;
	POINT _prevMousePos = {};

	TransformRef _focusMoveTransform;
	float _curFocusMoveTime = 0;
	float _focusMoveLookDist = 8.f;
	float _focusMoveUpDist = 1.f;
};

