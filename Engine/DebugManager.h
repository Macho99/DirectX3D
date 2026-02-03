#pragma once

enum class LogLevel
{
    Info,
    Warning,
    Error
};

struct LogEntry
{
    uint64_t id = 0;
    LogLevel level = LogLevel::Info;

    chrono::system_clock::time_point time;
    string message;                 // 리스트에 보이는 요약/전체
    vector<std::string> stack;      // 스택 프레임 문자열들
};

class DebugManager
{
    DECLARE_SINGLE(DebugManager);
public:
    void Init();
    void OnDestroy();

    void Log(const string& message);
    void LogW(const wstring& message);

    void LogWarning(const string& message);
    void LogWarningW(const wstring& message);

    void LogError(const string& message);
    void LogErrorW(const wstring& message);

    // Console에서 접근
    const vector<LogEntry>& GetLogs();
    void Clear();

private:
    void AddLog(LogLevel level, const string& message);
    static vector<string> CaptureStackTrace(int skipFrames = 3, int maxFrames = 32);

private:
    vector<LogEntry> _logs;
    mutex _logMutex;
    int _nextId;
};