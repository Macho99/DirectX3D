#include "pch.h"
#include "DebugManager.h"
#include "Utils.h"

#ifdef _WIN32
#include <Windows.h>
#include <DbgHelp.h>
#pragma comment(lib, "DbgHelp.lib")
#endif

void DebugManager::Init()
{
    std::lock_guard<std::mutex> lock(_logMutex);
    _logs.clear();
    _nextId = 1;
}

void DebugManager::OnDestroy()
{
    Clear();
}

void DebugManager::AddLog(LogLevel level, const string& message)
{
    LogEntry e;
    e.id = _nextId++;
    e.level = level;
    e.time = chrono::system_clock::now();
    e.message = message;

#ifdef _WIN32
    e.stack = CaptureStackTrace();
#endif

    lock_guard<mutex> lock(_logMutex);

    if (_logs.size() >= 200)
    {
        _logs.pop_front();
    }

    _logs.push_back(move(e));
}

vector<string> DebugManager::CaptureStackTrace(int skipFrames, int maxFrames)
{
    vector<string> out;

#ifdef _WIN32
    void* frames[64] = {};
    USHORT captured = CaptureStackBackTrace(skipFrames, (ULONG)maxFrames, frames, nullptr);
    if (captured == 0) return out;

    static std::atomic<bool> s_symInited{ false };
    static std::mutex s_symMutex;

    {
        std::lock_guard<std::mutex> lock(s_symMutex);
        if (!s_symInited.load())
        {
            SymSetOptions(SYMOPT_DEFERRED_LOADS | SYMOPT_UNDNAME | SYMOPT_LOAD_LINES);
            SymInitialize(GetCurrentProcess(), NULL, TRUE);
            s_symInited.store(true);
        }
    }

    HANDLE proc = GetCurrentProcess();

    // SYMBOL_INFO는 가변 길이 구조체라 크게 잡습니다.
    char buffer[sizeof(SYMBOL_INFO) + MAX_SYM_NAME] = {};
    SYMBOL_INFO* symbol = (SYMBOL_INFO*)buffer;
    symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
    symbol->MaxNameLen = MAX_SYM_NAME;

    IMAGEHLP_LINE64 line = {};
    line.SizeOfStruct = sizeof(IMAGEHLP_LINE64);

    for (USHORT i = 0; i < captured; ++i)
    {
        DWORD64 addr = (DWORD64)frames[i];

        std::string s;
        DWORD displacement = 0;
        DWORD64 disp64 = 0;

        if (SymFromAddr(proc, addr, &disp64, symbol))
        {
            s = symbol->Name;
        }
        else
        {
            s = "??";
        }

        if (SymGetLineFromAddr64(proc, addr, &displacement, &line))
        {
            s += "  (";
            s += line.FileName ? line.FileName : "unknown";
            s += ":";
            s += std::to_string(line.LineNumber);
            s += ")";
        }

        out.push_back(std::move(s));
    }
#endif

    return out;
}

void DebugManager::Log(const string& message)
{
    AddLog(LogLevel::Info, message);
}

void DebugManager::LogW(const wstring& message)
{
    AddLog(LogLevel::Info, Utils::ToString(message));
}

void DebugManager::LogWarning(const string& message)
{
    AddLog(LogLevel::Warning, message);
}

void DebugManager::LogWarningW(const wstring& message)
{
    AddLog(LogLevel::Warning, Utils::ToString(message));
}

void DebugManager::LogError(const string& message)
{
    AddLog(LogLevel::Error, message);
}

void DebugManager::LogErrorW(const wstring& message)
{
    AddLog(LogLevel::Error, Utils::ToString(message));
}

const deque<LogEntry>& DebugManager::GetLogs()
{
    return _logs;
}

void DebugManager::Clear()
{
    lock_guard<mutex> lock(_logMutex);
    _logs.clear();
}
