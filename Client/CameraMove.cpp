#include "pch.h"
#include "CameraMove.h"
#include "Transform.h"
#include "OnGUIUtils.h"

void CameraMove::Start()
{
}

void CameraMove::Update()
{
	float dt = TIME->GetDeltaTime();

	Vec3 pos = GetTransform()->GetPosition();
	float curMoveSpeed;
	if (INPUT->GetButton(KEY_TYPE::LSHIFT))
	{
		curMoveSpeed = _sprintSpeed;
	}
	else
	{
		curMoveSpeed = _moveSpeed;
	}

	if (INPUT->GetButton(KEY_TYPE::W))
		pos += GetTransform()->GetLook() * curMoveSpeed * dt;

	if (INPUT->GetButton(KEY_TYPE::S))
		pos -= GetTransform()->GetLook() * curMoveSpeed * dt;

	if (INPUT->GetButton(KEY_TYPE::A))
		pos -= GetTransform()->GetRight() * curMoveSpeed * dt;

	if (INPUT->GetButton(KEY_TYPE::D))
		pos += GetTransform()->GetRight() * curMoveSpeed * dt;


	if (INPUT->GetButton(KEY_TYPE::Q))
	{
		pos -= GetTransform()->GetUp() * curMoveSpeed * dt;
		//Vec3 rotation = GetTransform()->GetLocalRotation();
		//rotation.x += dt * 0.5f;
		//GetTransform()->SetLocalRotation(rotation);
	}

	if (INPUT->GetButton(KEY_TYPE::E))
	{
		pos += GetTransform()->GetUp() * curMoveSpeed * dt;
		//Vec3 rotation = GetTransform()->GetLocalRotation();
		//rotation.x -= dt * 0.5f;
		//GetTransform()->SetLocalRotation(rotation);
	}

	Vec3 rotation = GetTransform()->GetLocalRotation();
	if (INPUT->GetButtonDown(KEY_TYPE::RBUTTON))
	{
		_prevMousePos = INPUT->GetMousePos();
	}
	else if (INPUT->GetButton(KEY_TYPE::RBUTTON))
	{
		POINT curMousePos = INPUT->GetMousePos();
		Vec2 delta = Vec2(curMousePos.x - _prevMousePos.x, curMousePos.y - _prevMousePos.y);
		rotation.x += delta.y * _mouseSpeed;
		rotation.y += delta.x * _mouseSpeed;
		GetTransform()->SetLocalRotation(rotation);
		_prevMousePos = curMousePos;
	}
	GetTransform()->SetPosition(pos);

	//if (INPUT->GetButton(KEY_TYPE::Z))
	//{
	//	Vec3 rotation = GetTransform()->GetLocalRotation();
	//	rotation.y += dt * 0.5f;
	//	GetTransform()->SetLocalRotation(rotation);
	//}
	//
	//if (INPUT->GetButton(KEY_TYPE::C))
	//{
	//	Vec3 rotation = GetTransform()->GetLocalRotation();
	//	rotation.y -= dt * 0.5f;
	//	GetTransform()->SetLocalRotation(rotation);
	//}
}

bool CameraMove::OnGUI()
{
	bool changed = false;
	changed |= Super::OnGUI();
    changed |= OnGUIUtils::DrawFloat("Move Speed", &_moveSpeed, .1f);
    changed |= OnGUIUtils::DrawFloat("Sprint Speed", &_sprintSpeed, 1.f);
    changed |= OnGUIUtils::DrawFloat("Mouse Speed", &_mouseSpeed, 0.0001f);
	return changed;
}
