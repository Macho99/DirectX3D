#include "pch.h"
#include "MyBillboard.h"

void MyBillboard::Update()
{
	auto go = GetGameObject();

	Vec3 up = Vec3(0, 1, 0);
	Vec3 camPos = CUR_SCENE->GetMainCamera()->GetTransform()->GetPosition();
	Vec3 myPos = GetTransform()->GetPosition();

	Vec3 forward = camPos - myPos;
	forward.Normalize();

	Matrix mat = Matrix::CreateWorld(myPos, forward, up);
	Vec3 pos, scale;
	Quaternion rot;
	mat.Decompose(scale, rot, pos);
	Vec3 euler = Transform::ToEulerAngles(rot);

	GetTransform()->SetRotation(euler);
}
