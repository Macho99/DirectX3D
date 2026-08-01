#pragma once

struct AnimationClipImportSetting
{
    bool extractRootMotion = false;
    bool applyRootPositionXZ = false;
    bool applyRootPositionY = false;
    bool applyRootRotation = false;

    template<class Archive>
    void serialize(Archive& ar)
    {
        ar(
            CEREAL_NVP(extractRootMotion),
            CEREAL_NVP(applyRootPositionXZ),
            CEREAL_NVP(applyRootPositionY),
            CEREAL_NVP(applyRootRotation)
        );
    }

    bool OnGUI();
};
