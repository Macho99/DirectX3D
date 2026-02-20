#pragma once
struct ImportManifest
{
public:
    ImportManifest() {}
    ~ImportManifest() {}

    template<class Archive>
    void serialize(Archive& ar)
    {
        ar(CEREAL_NVP(size),
            CEREAL_NVP(timestamp),
            CEREAL_NVP(hash));
    }

    // Returns true if Manifest need to save
    bool Refresh(fs::path filePath, OUT bool& isDirty);

private:
    uint64_t size = 0;
    uint64_t timestamp = 0;
    string hash = "";
};