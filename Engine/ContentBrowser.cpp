#include "pch.h"
#include "ContentBrowser.h"

ContentBrowser::ContentBrowser()
    : Super("ContentBrower")
{
}

ContentBrowser::~ContentBrowser()
{
    watcher.Stop();
}

wstring ContentBrowser::ToStr(FsAction fsAction)
{
    switch (fsAction)
    {
        case FsAction::Added: return L"Added";
        case FsAction::Removed: return L"Removed";
        case FsAction::Modified: return L"Modified";
        case FsAction::Renamed: return L"Renamed";
        default: return L"?";
    }
}

void ContentBrowser::Init(EditorManager* editorManager)
{
    Super::Init(editorManager);

    fs::path assetsRoot = L"..\\Resources";

    if (!watcher.Start(assetsRoot, true, [&](const FsEvent& e)
        {
            // 워처 스레드: 절대 여기서 무거운 일 하지 말고 Push만!
            eventThreadQueue.Push(e);
        }))
    {
        DBG->LogW(L"Watcher start failed");
    }
    else
    {
        DBG->LogW(L"Watching: " + assetsRoot.wstring());
    }
}

void ContentBrowser::Update()
{
    pendingEvents.clear();
    eventThreadQueue.PopAll(pendingEvents);

    // 1) 필터 + 디바운서 입력
    for (auto& e : pendingEvents)
    {
        if (!IsInterestingFile(e.absPath))
            continue;

        debouncer.Push(e);
    }

    // 2) 디바운스 완료분 배출 (예: 300ms)
    readyEvents.clear();
    debouncer.PopReady(300, readyEvents);

    // 3) 최종 처리(여기서부터 메인 스레드)
    for (auto& e : readyEvents)
    {
        if (e.action == FsAction::Renamed)
        {
            DBG->LogW(L"[FS] " + ToStr(e.action)
                + L" : " + e.oldAbsPath.wstring()
                + L" -> " + e.absPath.wstring());
        }
        else
        {
            DBG->LogW(L"[FS] " + ToStr(e.action)
                + L" : " + e.absPath.wstring());
        }
    }
}

void ContentBrowser::OnGUI()
{
}

bool ContentBrowser::IsInterestingFile(const fs::path& p)
{
    if (!p.has_extension()) return false;

    auto ext = p.extension().wstring();
    for (auto& ch : ext) ch = (wchar_t)towlower(ch);

    // 일단 임포트 관련 핵심만
    if (ext == L".fbx") return true;
    if (ext == L".png" || ext == L".tga" || ext == L".jpg" || ext == L".jpeg") return true;

    // meta는 다음 단계에서 다시 포함시킬 겁니다.
    // if (ext == L".meta") return true;

    return false;
}