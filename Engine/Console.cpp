#include "pch.h"
#include "Console.h"

using namespace std::chrono;

Console::Console()
    : Super("Console")
{
}

Console::~Console()
{
}

void Console::Init()
{
}

void Console::OnGUI()
{
    Super::OnGUI();

    //float curTime = TIME->GetGameTime();
    //if (nextLogTime < curTime)
    //{
    //    DBG->Log("Console Auto Log");
    //    nextLogTime = curTime + 1.0f;
    //}


    // 상단 툴바
    if (ImGui::Button("Clear"))
        DBG->Clear();

    ImGui::SameLine();
    ImGui::Checkbox("AutoScroll", &autoScroll);

    //ImGui::SameLine();
    //ImGui::SeparatorText("Filter");

    ImGui::SameLine();
    ImGui::Checkbox("Info", &showInfo);
    ImGui::SameLine();
    ImGui::Checkbox("Warning", &showWarning);
    ImGui::SameLine();
    ImGui::Checkbox("Error", &showError);

    ImGui::Separator();

    // 상/하단 영역 분리
    ImVec2 avail = ImGui::GetContentRegionAvail();
    float listHeight = (avail.y - detailHeight);
    if (listHeight < 80.0f) listHeight = 80.0f;

    // 상단: 로그 리스트
    ImGui::BeginChild("ConsoleList", ImVec2(0, listHeight), true, ImGuiWindowFlags_AlwaysVerticalScrollbar);

    const auto& logs = DBG->GetLogs();

    // 선택된 로그를 찾기 위한 포인터(하단 출력용)
    const LogEntry* selected = nullptr;

    for (const auto& logElem : logs)
    {
        if (!PassFilter(logElem.level)) continue;

        if (logElem.id == selectedId) selected = &logElem;

        ImVec4 color = GetLogColor(logElem.level);
        ImGui::PushStyleColor(ImGuiCol_Text, color);

        // 한 줄 표시: 시간 + 메시지(요약)
        std::string timeStr = FormatTime(logElem.time);
        ImGui::TextUnformatted(timeStr.c_str());
        ImGui::SameLine();

        ImGui::PushID((void*)(uintptr_t)logElem.id); // 64-bit 안전

        bool isSelected = (selectedId == logElem.id);
        if (ImGui::Selectable(logElem.message.c_str(), isSelected, ImGuiSelectableFlags_SpanAllColumns))
            selectedId = logElem.id;

        if (ImGui::IsItemFocused() && selectedId != logElem.id)
            selectedId = logElem.id;

        ImGui::PopID();

        ImGui::PopStyleColor();
    }

    // 새 로그가 들어왔는지
    bool hasNewLogs = logs.size() != lastLogCount;

    // 렌더링 후(내용이 생겨 scrollMaxY가 변한 뒤) 바닥 여부 판단
    float scrollY_after = ImGui::GetScrollY();
    float scrollMaxY_after = ImGui::GetScrollMaxY();

    const float kBottomEpsilon = 2.0f;
    bool isAtBottomNow = (scrollMaxY_after - scrollY_after) <= kBottomEpsilon;

    if (autoScroll && hasNewLogs && wasAtBottom)
    {
        ImGui::SetScrollHereY(1.0f);
        // SetScrollHereY 후 다시 계산해 상태 업데이트(선택 사항이지만 안정적)
        scrollY_after = ImGui::GetScrollY();
        scrollMaxY_after = ImGui::GetScrollMaxY();
        isAtBottomNow = (scrollMaxY_after - scrollY_after) <= kBottomEpsilon;
    }

    // 상태 저장(다음 프레임용)
    wasAtBottom = isAtBottomNow;
    lastLogCount = logs.size();
    ImGui::EndChild();

    // 하단: 상세/스택
    ImGui::Separator();

    ImGui::BeginChild("ConsoleDetail", ImVec2(0, 0), true);

    // 선택된 로그 다시 찾아도 되지만, 위에서 잡아둔 selected가 없을 수 있어 재검색
    if (!selected)
    {
        for (const auto& e : logs)
        {
            if (e.id == selectedId) { selected = &e; break; }
        }
    }

    if (!selected)
    {
        ImGui::TextDisabled("Select a log to see details.");
        ImGui::EndChild();
        return;
    }

    // 헤더: 시간/레벨
    const char* lv = (selected->level == LogLevel::Info) ? "Info"
        : (selected->level == LogLevel::Warning) ? "Warning"
        : "Error";

    ImGui::Text("Time: %s", FormatTime_0p2s(selected->time).c_str());

    ImVec4 color = GetLogColor(selected->level);
    ImGui::PushStyleColor(ImGuiCol_Text, color);
    ImGui::Text("Level: %s", lv);
    ImGui::PopStyleColor();

    // 상세 메시지
    ImGui::SeparatorText("StackTrace");

    if (selected->stack.empty())
    {
        ImGui::TextDisabled("(no stack trace)");
    }
    else
    {
        for (const auto& line : selected->stack)
            ImGui::Text(line.c_str());
    }

    ImGui::EndChild();
}

bool Console::PassFilter(LogLevel lv)
{
    if (lv == LogLevel::Info) 
        return showInfo;

    if (lv == LogLevel::Warning) 
        return showWarning;

    if (lv == LogLevel::Error) 
        return showError;

    return true;
}

string Console::FormatTime(const chrono::system_clock::time_point& timePoint)
{
    auto tt = system_clock::to_time_t(timePoint);
    std::tm tm{};
#ifdef _WIN32
    localtime_s(&tm, &tt);
#else
    tm = *std::localtime(&tt);
#endif

    std::ostringstream oss;
    oss << std::setfill('0')
        << "[" << std::setw(2) << tm.tm_hour << ":"
        << std::setw(2) << tm.tm_min << ":"
        << std::setw(2) << tm.tm_sec << "]";

    return oss.str();
}

string Console::FormatTime_0p2s(const chrono::system_clock::time_point& timePoint)
{
    using namespace std::chrono;

    auto tt = system_clock::to_time_t(timePoint);
    std::tm tm{};
#ifdef _WIN32
    localtime_s(&tm, &tt);
#else
    tm = *std::localtime(&tt);
#endif

    // 0.1초 단위(=100ms)로 내림
    auto ms = duration_cast<milliseconds>(timePoint.time_since_epoch()).count();
    int tenth = (int)((ms / 10) % 100); // 0~99

    std::ostringstream oss;
    oss << std::setfill('0')
        << std::setw(2) << tm.tm_hour << ":"
        << std::setw(2) << tm.tm_min << ":"
        << std::setw(2) << tm.tm_sec << "."
        << tenth;

    return oss.str();
}

ImVec4 Console::GetLogColor(LogLevel level)
{
    ImVec4 color = ImVec4(1, 1, 1, 1);
    if (level == LogLevel::Warning) color = ImVec4(1, 1, 0.3f, 1);
    else if (level == LogLevel::Error) color = ImVec4(1, 0.3f, 0.3f, 1);
    return color;
}
