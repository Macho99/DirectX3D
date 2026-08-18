#pragma once
#include "MonoBehaviour.h"

class MyBillboard : public MonoBehaviour
{
    using Super = MonoBehaviour;
    DECLARE_MONO_BEHAVIOUR(MyBillboard)
public:
	virtual void Update();
};

