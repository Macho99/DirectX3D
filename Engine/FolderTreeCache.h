#pragma once

struct PathHash
{
    size_t operator()(const fs::path& p) const noexcept
    {
        // generic_u8string은 슬래시 통일(경로 비교/해시 안정)
        auto s = p.generic_u8string();
        return std::hash<std::string>()(std::string(reinterpret_cast<const char*>(s.data()), s.size()));
    }
};

struct PathEq
{
    bool operator()(const fs::path& a, const fs::path& b) const noexcept
    {
        // 같은 절대경로라고 가정(사용자 쪽에서 Normalize 해주시면 더 좋습니다)
        return a == b;
    }
};

class FolderTreeCache
{
public:
    struct Node
    {
        fs::path abs;
        std::string displayName; // UTF-8, ImGui 라벨용
        std::vector<Node*> children; // 소유는 unique_ptr 풀에서 관리

        bool dirty = true;      // true면 다음에 열릴 때 스캔
        bool scanned = false;   // 한 번이라도 스캔했는지
        bool hasChildren = false; // 빠른 leaf 판단(스캔 후 채움)
    };

public:
    void SetRoot(const fs::path& rootAbs);
    void Invalidate(const fs::path& folderAbs);
    void InvalidateAll();
    Node* GetRoot() const { return _root; }

    // 열려있는 노드만 스캔하도록 Draw 중에 호출하세요.
    void EnsureScanned(Node* n);

private:
    Node* CreateNode(const fs::path& abs);

private:
    fs::path _rootAbs;
    Node* _root = nullptr;

    std::vector<std::unique_ptr<Node>> _pool;
    std::unordered_map<fs::path, Node*, PathHash, PathEq> _lookup;
};

