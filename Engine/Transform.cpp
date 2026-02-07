#include "pch.h"
#include "Transform.h"

Transform::Transform() : Super(ComponentType::Transform)
{

}

Transform::~Transform()
{

}

void Transform::Awake()
{

}

void Transform::Update()
{
}

Vec3 Transform::ToEulerAngles(Quaternion q)
{
	q.Normalize();
	Vec3 angles;

	// roll (x-axis rotation)
	const double sinr_cosp = 2.0 * (q.w * q.x + q.y * q.z);
	const double cosr_cosp = 1.0 - 2.0 * (q.x * q.x + q.y * q.y);
	angles.x = static_cast<float>(std::atan2(sinr_cosp, cosr_cosp));

	// pitch (y-axis rotation)
	double sinp = 2.0 * (q.w * q.y - q.z * q.x);
	sinp = std::clamp(sinp, -1.0, 1.0);
	angles.y = (float)std::asin(sinp);

	// yaw (z-axis rotation)
	const double siny_cosp = 2.0 * (q.w * q.z + q.x * q.y);
	const double cosy_cosp = 1.0 - 2.0 * (q.y * q.y + q.z * q.z);
	angles.z = static_cast<float>(std::atan2(siny_cosp, cosy_cosp));

	return angles;
}

void Transform::UpdateTransform()
{
	Matrix matScale = Matrix::CreateScale(_localScale);
	Matrix matRotation = Matrix::CreateRotationX(_localRotation.x);
	matRotation *= Matrix::CreateRotationY(_localRotation.y);
	matRotation *= Matrix::CreateRotationZ(_localRotation.z);
	Matrix matTranslation = Matrix::CreateTranslation(_localPosition);

	_matLocal = matScale * matRotation * matTranslation;

	Transform* parent;
	if (TryGetParent(OUT parent))
	{
		_matWorld = _matLocal * parent->GetWorldMatrix();
	}
	else
	{
		_matWorld = _matLocal;
	}

	Quaternion quat;
	_matWorld.Decompose(_scale, quat, _position);
	_rotation = ToEulerAngles(quat);

	// Children
	for (const TransformRef& child : _children)
		child.Resolve()->UpdateTransform();
}

void Transform::SetScale(const Vec3& worldScale)
{
	Transform* parent;
	if (TryGetParent(OUT parent))
	{
		Vec3 parentScale = parent->GetScale();
		Vec3 scale = worldScale;
		scale.x /= parentScale.x;
		scale.y /= parentScale.y;
		scale.z /= parentScale.z;
		SetLocalScale(scale);
	}
	else
	{
		SetLocalScale(worldScale);
	}
}

void Transform::SetRotation(const Vec3& worldRotation)
{
	Transform* parent;
	if (TryGetParent(OUT parent))
	{
		Matrix inverseMatrix = parent->GetWorldMatrix().Invert();

		Vec3 rotation;
		rotation.TransformNormal(worldRotation, inverseMatrix);

		SetLocalRotation(rotation);
	}
	else
		SetLocalRotation(worldRotation);
}

void Transform::SetPosition(const Vec3& worldPosition)
{
	Transform* parent;
	if (TryGetParent(OUT parent))
	{
		Matrix worldToParentLocalMatrix = parent->GetWorldMatrix().Invert();

		Vec3 position;
		position.Transform(worldPosition, worldToParentLocalMatrix);

		SetLocalPosition(position);
	}
	else
	{
		SetLocalPosition(worldPosition);
	}
}

void Transform::SetWorldMatrix(Matrix& newWorld)
{
	Matrix parentWorld = Matrix::Identity;
	if (HasParent())
		parentWorld = GetParent()->GetWorldMatrix();

	// newLocal = inverse(parentWorld) * newWorld
	Matrix localMatrix = HasParent()
		? newWorld * parentWorld.Invert()
		: newWorld;

	Vec3 scale, pos;
	Quaternion rot;
	if (!localMatrix.Decompose(scale, rot, pos))
		return;

	rot.Normalize();
	if (!std::isfinite(pos.x) || !std::isfinite(pos.y) || !std::isfinite(pos.z) ||
		!std::isfinite(scale.x) || !std::isfinite(scale.y) || !std::isfinite(scale.z) ||
		!std::isfinite(rot.x) || !std::isfinite(rot.y) || !std::isfinite(rot.z) || !std::isfinite(rot.w))
	{
		return;
	}

    _localPosition = pos;
    _localRotation = Transform::ToEulerAngles(rot);
    _localScale = scale;
    UpdateTransform();
}

void Transform::SetParent(TransformRef& newParentRef)
{
	Matrix prevWorld = GetWorldMatrix();

	const Guid myID = _guid;
	Transform* oldParent;
	TryGetParent(OUT oldParent);
    Transform* newParent = newParentRef.Resolve();
    TransformRef myRef(_guid);

	if (newParent == nullptr && oldParent == nullptr)
		return;

	if (newParent == oldParent)
		return;

	if (newParent != nullptr)
	{
		if (newParent->_guid == myID)
		{
			wcout << L"자기 자신을 부모로 설정하려고 시도함" << endl;
			return;
		}

		if (IsAncestorOf(newParentRef))
		{
			wcout << L"자손 노드를 부모로 설정하려고 시도함" << endl; // Prevent cycle
			return;
		}

		newParent->_children.push_back(myRef);
	}
	// newParent == nullptr
	else
	{
		if (CUR_SCENE->IsInScene(_gameObject))
		{
			vector<TransformRef>& rootObjects = CUR_SCENE->GetRootObjects();
			rootObjects.push_back(myRef);
		}
	}

	// oldParent Setting
	{
		if (oldParent)
		{
			// Remove from old parent's children
			vector<TransformRef>& siblings = oldParent->_children;
			RemoveFromTransforms(siblings, myID);
		}
		// oldParent == nullptr
		else
		{
			if (CUR_SCENE->IsInScene(_gameObject))
			{
				vector<TransformRef>& rootObjects = CUR_SCENE->GetRootObjects();
				RemoveFromTransforms(rootObjects, myID);
			}
		}
	}
	
	_parent = newParentRef;
    SetWorldMatrix(prevWorld);
}

void Transform::SetSiblingIndex(int index)
{
	vector<TransformRef>* siblings;

	Transform* parent;
	if (TryGetParent(OUT parent))
	{
		siblings = &parent->_children;
	}
	else
	{
		if (CUR_SCENE->IsInScene(_gameObject) == false)
		{
			wcout << L"씬에 할당하지 않고 SetSiblingIndex()을 호출하였습니다." << endl;
			return;
		}
		siblings = &CUR_SCENE->GetRootObjects();
	}
	
    if (siblings == nullptr)
        return;

    if (siblings->empty())
        return;

	auto it = std::find(siblings->begin(), siblings->end(), TransformRef(_guid));

	size_t oldIndex = static_cast<size_t>(std::distance(siblings->begin(), it));

	if (index < 0) index = 0;

	size_t newIndex = static_cast<size_t>(index);
	if (newIndex >= siblings->size())
		newIndex = siblings->size() - 1;

	// 이미 그 위치면 종료
	if (oldIndex == newIndex)
		return;

	TransformRef self = *it;
	siblings->erase(it);

	if (newIndex > oldIndex)
		newIndex -= 1;

	siblings->insert(siblings->begin() + newIndex, std::move(self));
}

bool Transform::IsAncestorOf(TransformRef& targetRef)
{
    Transform* target = targetRef.Resolve();
    if (target == nullptr)
        return false;

	const Guid myID = this->_guid;
    Transform* current = target->GetParent();
    while (current)
    {
        if (current->_guid == myID)
            return true;
        current = current->GetParent();
    }
    return false;
}

void Transform::RemoveFromTransforms(vector<TransformRef>& transforms, TransformRef targetId)
{
	auto iter = std::remove(transforms.begin(), transforms.end(), targetId);
	transforms.erase(iter, transforms.end());
}
