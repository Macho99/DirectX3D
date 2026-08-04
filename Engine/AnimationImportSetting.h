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
};