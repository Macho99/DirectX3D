#include "pch.h"
#include "MyBillboard.h"

void MyBillboard::Update()
{
	GameObject* cameraObject = CUR_SCENE->GetMainCamera();
	if (cameraObject == nullptr)
		return;

	Transform* transform = GetTransform();
	Vec3 position = transform->GetPosition();
	Vec3 scale = transform->GetScale();
	Vec3 cameraPosition = cameraObject->GetTransform()->GetPosition();

	Matrix billboard = Matrix::CreateConstrainedBillboard(
		position,
		cameraPosition,
		Vec3::Up);
	transform->SetWorldMatrix(Matrix::CreateScale(scale) * billboard);
}
