#include "pch.h"
#include "ShopNPC.h"
#include "OnGUIUtils.h"
#include "Button.h"
#include "ThirdPersonCamMove.h"

void ShopNPC::Start()
{
    Button* shopCloseButton = _shopCloseButton.Resolve();
    if (shopCloseButton != nullptr)
    {
        shopCloseButton->AddOnClickedEvent([this]()
            {
                CloseShopUI();
            });
    }
    else
    {
        DBG->LogError("Shop Close Button is null.");
    }

    for (int i = 0; i < _shopItemButtons.size(); ++i)
    {
        Button* shopItemButton = _shopItemButtons[i].Resolve();
        if (shopItemButton != nullptr)
        {
            shopItemButton->AddOnClickedEvent([this, i]()
                {
                    DBG->Log("Shop Item Button %d clicked.", i);
                });
        }
        else
        {
            DBG->LogError("Shop Item Button %d is null.", i);
        }
    }
}

void ShopNPC::Update()
{
    if (GM->GetGameState() != GameState::World)
        return;

    const Vec3 playerPos = GM->GetPlayerPosition();
    const Vec3 npcPos = GetTransform()->GetPosition();
    if (Vec3::Distance(playerPos, npcPos) < 3.f)
    {
        GameObject* guideImageObj = _guideImage.Resolve();
        if (guideImageObj != nullptr)
        {
            guideImageObj->SetActive(true);
        }

        if (INPUT->GetButtonDown(KEY_TYPE::E))
        {
            OpenShopUI();
        }
    }
    else
    {
        GameObject* guideImageObj = _guideImage.Resolve();
        if (guideImageObj != nullptr)
        {
            guideImageObj->SetActive(false);
        }
        CloseShopUI();
    }
}

bool ShopNPC::OnGUI()
{
    bool changed = Super::OnGUI();

    changed |= OnGUIUtils::DrawGameObjectRef("Guide Image", _guideImage);
    changed |= OnGUIUtils::DrawGameObjectRef("Shop UI", _shopUI);
    changed |= OnGUIUtils::DrawComponentRef("Shop Close Button", _shopCloseButton);

    uint32 shopItemButtonsCount = static_cast<uint32>(_shopItemButtons.size());
    changed |= OnGUIUtils::DrawUInt32("Shop Item Buttons Count", &shopItemButtonsCount, 1);
    if (shopItemButtonsCount != _shopItemButtons.size())
    {
        _shopItemButtons.resize(shopItemButtonsCount);
        changed = true;
    }

    for (uint32 i = 0; i < shopItemButtonsCount; ++i)
    {
        string buttonLabel = "Shop Item Button " + std::to_string(i);
        changed |= OnGUIUtils::DrawComponentRef(buttonLabel.c_str(), _shopItemButtons[i]);
    }

    return changed;
}

void ShopNPC::OpenShopUI()
{
    GameObject* shopUIObj = _shopUI.Resolve();
    if (shopUIObj != nullptr && shopUIObj->IsActiveInLocal() == false)
    {
        INPUT->ShowMouseCursor();
        CUR_SCENE->FindComponent<ThirdPersonCamMove>()->SetEnabled(false);
        shopUIObj->SetActive(true);
    }
}

void ShopNPC::CloseShopUI()
{
    GameObject* shopUIObj = _shopUI.Resolve();
    if (shopUIObj != nullptr && shopUIObj->IsActiveInLocal())
    {
        INPUT->CaptureMouseCursor();
        CUR_SCENE->FindComponent<ThirdPersonCamMove>()->SetEnabled(true);
        shopUIObj->SetActive(false);
    }
}
