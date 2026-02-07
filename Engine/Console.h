#pragma once
#include "EditorWindow.h"
class Console :
    public EditorWindow
{
    using Super = EditorWindow;
public:
    Console();
    ~Console();

protected:
    void OnGUI() override;

private:
    bool PassFilter(LogLevel lv);
    string FormatTime(const chrono::system_clock::time_point& timePoint);
    string FormatTime_0p2s(const chrono::system_clock::time_point& timePoint);
    ImVec4 GetLogColor(LogLevel level);

private:
    bool showInfo = true;
    bool showWarning = true;
    bool showError = true;

    bool autoScroll = true;
    bool wasAtBottom = true;
    size_t lastLogCount = 0;

    uint64_t selectedId = 0;
    float detailHeight = 250.0f;
};

