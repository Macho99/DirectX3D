#pragma once
#include <MonoBehaviour.h>
class ShopNPC : public MonoBehaviour
{
    using Super = MonoBehaviour;
    DECLARE_MONO_BEHAVIOUR(ShopNPC);
public:
    virtual void Start() override;
    virtual void Update() override;
    virtual bool OnGUI() override;

    virtual int GetVersion() const override { return 2; }

    void OpenShopUI();
    void CloseShopUI();

    template<typename Archive>
    void serialize(Archive& ar)
    {
        Super::serialize(ar);


        if (_version >= 2)
        {
            ar(CEREAL_NVP(_guideImage));
            ar(CEREAL_NVP(_shopUI));
            ar(CEREAL_NVP(_shopCloseButton));
            ar(CEREAL_NVP(_shopItemButtons));
        }
    }

private:
    GameObjectRef _guideImage;
    GameObjectRef _shopUI;
    ComponentRef<Button> _shopCloseButton;
    vector<ComponentRef<Button>> _shopItemButtons;
};

