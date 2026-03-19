#include "pch.h";
#include "ComponentRegistry.h"

void ComponentRegistry::Init()
{
    sort(_descs.begin(), _descs.end(), [](const ComponentDesc& a, const ComponentDesc& b)
        {
            return string(a.name) < string(b.name);
        });
}
