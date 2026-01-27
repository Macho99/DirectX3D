#include "pch.h"
#include "Transform.h"

Transform::Transform() : Super(ComponentType::Transform)
{
    static TransformID nextId = 1;
    _id = nextId++;
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

	shared_ptr<Transform> parent;
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
	for (const shared_ptr<Transform>& child : _children)
		child->UpdateTransform();
}

void Transform::SetScale(const Vec3& worldScale)
{
	shared_ptr<Transform> parent;
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
	shared_ptr<Transform> parent;
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
	shared_ptr<Transform> parent;
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

void Transform::SetParent(shared_ptr<Transform> newParent)
{
	const TransformID myID = GetID();
	shared_ptr<Transform> oldParent;
	TryGetParent(OUT oldParent);

	if (newParent != nullptr || oldParent != nullptr)
	{
		if (newParent != nullptr)
		{
			if (newParent->GetID() == myID)
			{
				wcout << L"자기 자신을 부모로 설정하려고 시도함" << endl;
				return;
			}

			if (IsAncestorOf(newParent))
			{
				wcout << L"자손 노드를 부모로 설정하려고 시도함" << endl; // Prevent cycle
				return;
			}

			newParent->_children.push_back(GetGameObject()->GetTransform());
		}
		// newParent == nullptr
		else
		{
			if (CUR_SCENE->IsInScene(myID))
			{
				vector<shared_ptr<Transform>>& rootObjects = CUR_SCENE->GetRootObjects();
				rootObjects.push_back(GetGameObject()->GetTransform());
			}
		}

		// oldParent Setting
		{
			if (oldParent)
			{
				// Remove from old parent's children
				vector<shared_ptr<Transform>>& siblings = oldParent->_children;
				RemoveFromTransforms(siblings, myID);
			}
			// oldParent == nullptr
			else
			{
				if (CUR_SCENE->IsInScene(myID))
				{
					vector<shared_ptr<Transform>>& rootObjects = CUR_SCENE->GetRootObjects();
					RemoveFromTransforms(rootObjects, myID);
				}
			}
		}
	}
	
	_parent = newParent;
}

void Transform::SetSiblingIndex(int index)
{
	vector<shared_ptr<Transform>>* siblings;

	shared_ptr<Transform> parent;
	if (TryGetParent(OUT parent))
	{
		siblings = &parent->_children;
	}
	else
	{
		if (CUR_SCENE->IsInScene(GetID()) == false)
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

	const TransformID myId = GetID();
	auto it = std::find_if(siblings->begin(), siblings->end(),
		[myId](const shared_ptr<Transform>& t)
		{
			return t->GetID() == myId;
		});

	size_t oldIndex = static_cast<size_t>(std::distance(siblings->begin(), it));

	if (index < 0) index = 0;

	size_t newIndex = static_cast<size_t>(index);
	if (newIndex >= siblings->size())
		newIndex = siblings->size() - 1;

	// 이미 그 위치면 종료
	if (oldIndex == newIndex)
		return;

	shared_ptr<Transform> self = *it;
	siblings->erase(it);

	if (newIndex > oldIndex)
		newIndex -= 1;

	siblings->insert(siblings->begin() + newIndex, std::move(self));
}

bool Transform::IsAncestorOf(shared_ptr<Transform>& target)
{
    if (target == nullptr)
        return false;

	const TransformID myID = this->GetID();
    shared_ptr<Transform> current = target->GetParent();
    while (current)
    {
        if (current->GetID() == myID)
            return true;
        current = current->GetParent();
    }
    return false;
}

void Transform::RemoveFromTransforms(vector<shared_ptr<Transform>>& transforms, TransformID targetId)
{
	auto iter = std::remove_if(transforms.begin(), transforms.end(),
		[targetId](shared_ptr<Transform>& transform)
		{
			return transform->GetID() == targetId;
		});
	transforms.erase(iter, transforms.end());
}
