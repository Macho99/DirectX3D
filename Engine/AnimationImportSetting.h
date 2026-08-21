#pragma once

struct AnimationClipImportSetting
{
    int version = 0;
    bool extractRootMotion = false;
    bool applyRootPositionXZ = false;
    bool applyRootPositionY = false;
    bool applyRootRotation = false;
    bool isDeadAnimation = false;
    string nextComboName = "";
    float animationSpeed = 1.f;

    template<class Archive>
    void serialize(Archive& ar)
    {
        ar(
            CEREAL_NVP(extractRootMotion),
            CEREAL_NVP(applyRootPositionXZ),
            CEREAL_NVP(applyRootPositionY),
            CEREAL_NVP(applyRootRotation)
        );

        if (version >= 3)
        {
            ar(CEREAL_NVP(isDeadAnimation));
        }

        if (version >= 4)
        {
            ar(CEREAL_NVP(nextComboName));
        }

        if (version >= 5)
        {
            ar(CEREAL_NVP(animationSpeed));
        }
    }

    bool OnGUI();
};

struct AnimationEvent
{
    string eventName = "";
    bool boolParam = false;
    int intParam = 0;
    float floatParam = 0.f;
    uint32 frame = 0;
    template<class Archive>
    void serialize(Archive& ar)
    {
        ar(
            CEREAL_NVP(eventName),
            CEREAL_NVP(boolParam),
            CEREAL_NVP(intParam),
            CEREAL_NVP(floatParam),
            CEREAL_NVP(frame)
        );
    }

    bool OnGUI();
};