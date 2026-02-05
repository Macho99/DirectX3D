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
	Vec3 angles;

	// roll (x-axis rotation)
	double sinr_cosp = 2 * (q.w * q.x + q.y * q.z);
	double cosr_cosp = 1 - 2 * (q.x * q.x + q.y * q.y);
	angles.x = std::atan2(sinr_cosp, cosr_cosp);

	// pitch (y-axis rotation)
	double sinp = std::sqrt(1 + 2 * (q.w * q.y - q.x * q.z));
	double cosp = std::sqrt(1 - 2 * (q.w * q.y - q.x * q.z));
	angles.y = 2 * std::atan2(sinp, cosp) - 3.14159f / 2;

	// yaw (z-axis rotation)
	double siny_cosp = 2 * (q.w * q.z + q.x * q.y);
	double cosy_cosp = 1 - 2 * (q.y * q.y + q.z * q.z);
	angles.z = std::atan2(siny_cosp, cosy_cosp);

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

void Transform::SetParent(TransformRef& newParentRef)
{
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
