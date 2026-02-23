#include "pch.h"
#include "FolderTreeCache.h"

void FolderTreeCache::SetRoot(const fs::path& rootAbs)
{
    _rootAbs = rootAbs;
    _pool.clear();
    _lookup.clear();
    _root = CreateNode(_rootAbs);
    // 루트는 보통 바로 보이니 처음부터 스캔해도 되고(선호),
    // 안 해도 됩니다. 여기서는 "열릴 때" 스캔 방식 유지.
    _root->dirty = true;
    _root->scanned = false;
}

void FolderTreeCache::Invalidate(const fs::path& folderAbs)
{
    DBG->LogW(L"[FolderTreeCache] Invalidate: " + folderAbs.wstring());
    auto it = _lookup.find(folderAbs);
    if (it != _lookup.end())
    {
        it->second->dirty = true;
    }
    else
    {
        // 노드가 아직 캐시에 없으면 무시해도 됩니다(열리면 생성됨).
    }
}

void FolderTreeCache::InvalidateAll()
{
    for (auto& [_, n] : _lookup)
        n->dirty = true;
}

void FolderTreeCache::EnsureScanned(Node* n)
{
    if (!n) return;
    if (!n->dirty && n->scanned) return;

    DBG->LogW(L"[FolderTreeCache] Scan Start : " + n->abs.wstring());

    n->dirty = false;
    n->scanned = true;

    vector<fs::path> dirs;
    dirs.reserve(64);

    std::error_code ec;
    fs::directory_options opt = fs::directory_options::skip_permission_denied;

    for (fs::directory_iterator it(n->abs, opt, ec); !ec && it != fs::directory_iterator(); it.increment(ec))
    {
        const auto& de = *it;
        if (de.is_directory(ec) && !ec)
            dirs.push_back(de.path());
    }

    std::sort(dirs.begin(), dirs.end(),
        [](const fs::path& a, const fs::path& b)
        {
            // 정렬 비용도 "갱신 시 1회"만 발생
            return a.filename().wstring() < b.filename().wstring();
        });

    // 기존 children 갱신: "현재 존재하는 하위 폴더" 기준으로 재구성
    // - 새 폴더는 생성
    // - 삭제된 폴더는 children에서 제거(풀에서 제거까지는 안 함: 단순화/안전)
    vector<Node*> newChildren;
    newChildren.reserve(dirs.size());

    for (auto& childAbs : dirs)
    {
        Node* child = nullptr;
        if (auto it2 = _lookup.find(childAbs); it2 != _lookup.end())
        {
            child = it2->second;
        }
        else
        {
            child = CreateNode(childAbs);
        }
        newChildren.push_back(child);
    }

    n->children.swap(newChildren);
    n->hasChildren = !n->children.empty();
}

FolderTreeCache::Node* FolderTreeCache::CreateNode(const fs::path& abs)
{
    auto up = std::make_unique<Node>();
    up->abs = abs;
    up->displayName = Utils::ToUtf8(abs);

    Node* raw = up.get();
    _pool.emplace_back(std::move(up));
    _lookup.emplace(raw->abs, raw);
    return raw;
}