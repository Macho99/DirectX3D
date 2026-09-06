#pragma once
#include <array>
#include <algorithm>
#include <chrono>
#include <d3d11.h>
#include <wrl/client.h>
#include "imgui.h"

// Immediate-context only. Query slots are never reused while GPU work is pending.
class GpuProfiler
{
public:
    enum Pass { Frame, Scene, Shadows, Bloom, SSR, GrassCull, GrassDraw, PassCount };
    static GpuProfiler& Get() { static GpuProfiler instance; return instance; }

    void Init(ID3D11Device* device, ID3D11DeviceContext* context)
    {
        Shutdown();
        _context = context;
        D3D11_QUERY_DESC desc = { D3D11_QUERY_TIMESTAMP_DISJOINT, 0 };
        for (auto& frame : _frames)
        {
            if (FAILED(device->CreateQuery(&desc, frame.disjoint.GetAddressOf()))) { _failed = true; return; }
            desc.Query = D3D11_QUERY_TIMESTAMP;
            for (auto& span : frame.spans)
                if (FAILED(device->CreateQuery(&desc, span.begin.GetAddressOf())) ||
                    FAILED(device->CreateQuery(&desc, span.end.GetAddressOf()))) { _failed = true; return; }
            desc.Query = D3D11_QUERY_TIMESTAMP_DISJOINT;
        }
    }

    void Shutdown()
    {
        _frames = {}; _stats = {}; _context = nullptr; _active = -1;
        _failed = false; _skipped = 0; _invalid = 0; _generation = 0; _sequence = 0;
        _comparison = {}; _captureSlot = -1; _captureId = 0;
        _countSceneDraws = false; _sceneDraws = 0; _cpuUpdate = 0;
    }

    void BeginFrame()
    {
        if (!_context || _failed) return;
        // Poll once; never flush or wait for an unfinished query.
        std::array<FrameQueries*, 6> pending;
        for (unsigned i = 0; i < _frames.size(); ++i) pending[i] = &_frames[i];
        std::sort(pending.begin(), pending.end(), [](const FrameQueries* a, const FrameQueries* b) { return a->sequence < b->sequence; });
        for (auto* entry : pending)
        {
            auto& frame = *entry;
            if (!frame.pending) continue;
            D3D11_QUERY_DATA_TIMESTAMP_DISJOINT data = {};
            HRESULT hr = _context->GetData(frame.disjoint.Get(), &data, sizeof(data), D3D11_ASYNC_GETDATA_DONOTFLUSH);
            if (hr == S_FALSE) continue;
            if (FAILED(hr)) { _failed = true; return; }
            if (data.Disjoint || !data.Frequency) { frame.pending = false; ++_invalid; continue; }
            bool ready = true;
            for (unsigned i = 0; i < frame.count; ++i)
            {
                auto& span = frame.spans[i];
                HRESULT a = _context->GetData(span.begin.Get(), &span.start, sizeof(span.start), D3D11_ASYNC_GETDATA_DONOTFLUSH);
                HRESULT b = _context->GetData(span.end.Get(), &span.stop, sizeof(span.stop), D3D11_ASYNC_GETDATA_DONOTFLUSH);
                if (FAILED(a) || FAILED(b)) { _failed = true; return; }
                if (a != S_OK || b != S_OK) ready = false;
            }
            if (!ready) continue;
            std::array<double, PassCount> totals = {};
            std::array<bool, PassCount> seen = {};
            bool valid = !frame.overflow;
            for (unsigned i = 0; i < frame.count; ++i)
            {
                const auto& span = frame.spans[i];
                if (span.stop < span.start) { valid = false; break; }
                totals[span.pass] += double(span.stop - span.start) * 1000.0 / double(data.Frequency);
                seen[span.pass] = true;
            }
            if (valid && frame.generation == _generation)
                for (int i = 0; i < PassCount; ++i)
                    if (seen[i]) _stats[i].Add(totals[i]);
            if (valid && seen[Scene] && frame.cpuSceneReady && _captureSlot >= 0 && frame.captureId == _captureId)
            {
                auto& result = _comparison[_captureSlot];
                result.cpuUpdate += frame.cpuUpdate;
                result.cpuRender += frame.cpuRender;
                result.gpuScene += totals[Scene];
                result.draws += frame.draws;
                if (++result.samples == 120) _captureSlot = -1;
            }
            if (!valid) ++_invalid;
            frame.pending = false;
        }
        if (!_enabled) return;
        for (unsigned i = 0; i < _frames.size(); ++i)
        {
            if (_frames[i].pending) continue;
            _active = int(i);
            auto& frame = _frames[i]; frame.count = 0; frame.generation = _generation;
            frame.sequence = ++_sequence; frame.overflow = false;
            frame.captureId = _captureSlot >= 0 ? _captureId : 0;
            frame.cpuSceneReady = false;
            _context->Begin(frame.disjoint.Get());
            _frameToken = Begin(Frame);
            return;
        }
        ++_skipped;
    }

    int Begin(Pass pass)
    {
        if (_active < 0) return -1;
        auto& frame = _frames[_active];
        if (frame.count == frame.spans.size()) { frame.overflow = true; ++_skipped; return -1; }
        const int token = int(frame.count++);
        frame.spans[token].pass = pass;
        _context->End(frame.spans[token].begin.Get());
        return token;
    }
    void End(int token)
    {
        if (_active >= 0 && token >= 0) _context->End(_frames[_active].spans[token].end.Get());
    }
    void EndFrame()
    {
        if (_active < 0) return;
        End(_frameToken);
        auto& frame = _frames[_active];
        _context->End(frame.disjoint.Get()); frame.pending = true; _active = -1;
    }

    using CpuClock = std::chrono::steady_clock;
    void SetCpuUpdate(double ms) { _cpuUpdate = ms; }
    void BeginCpuScene()
    {
        _sceneDraws = 0;
        _countSceneDraws = true;
        _cpuSceneStart = CpuClock::now();
    }
    void CountDraw() { if (_countSceneDraws) ++_sceneDraws; }
    void EndCpuScene()
    {
        const double ms = std::chrono::duration<double, std::milli>(CpuClock::now() - _cpuSceneStart).count();
        _countSceneDraws = false;
        if (_active < 0) return;
        auto& frame = _frames[_active];
        frame.cpuRender = ms;
        frame.cpuUpdate = _cpuUpdate;
        frame.draws = _sceneDraws;
        frame.cpuSceneReady = true;
    }

    void OnBatchComparisonGUI()
    {
        ImGui::SeparatorText("Batching comparison (whole scene)");
        ImGui::TextWrapped("Show only the original models, capture Before; then show only the batched model, capture After. Keep the camera, resolution and settings identical.");
        ImGui::TextWrapped("Wait for loading to finish. Disable grass CPU readback. Each capture collects 120 matching CPU/GPU frames; results stay in memory.");
        if (_failed) ImGui::TextUnformatted("GPU queries failed; comparison unavailable until restart.");
        ImGui::BeginDisabled(_captureSlot >= 0 || _failed);
        if (ImGui::Button("Capture Before")) StartComparison(0);
        ImGui::SameLine();
        if (ImGui::Button("Capture After")) StartComparison(1);
        ImGui::EndDisabled();
        if (_captureSlot >= 0)
        {
            ImGui::Text("Capturing %s: %u / 120", _captureSlot == 0 ? "Before" : "After", _comparison[_captureSlot].samples);
            if (ImGui::Button("Cancel capture")) { _comparison[_captureSlot] = {}; _captureSlot = -1; }
        }
        const char* labels[] = { "CPU scene update (ms)", "CPU scene render submission (ms)", "GPU scene (ms)", "Scene Draw Calls (all passes)" };
        if (ImGui::BeginTable("BatchComparisonResults", 4,
            ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable | ImGuiTableFlags_SizingStretchProp))
        {
            ImGui::TableSetupColumn("Metric", ImGuiTableColumnFlags_WidthStretch, 3.0f);
            ImGui::TableSetupColumn("Before mean", ImGuiTableColumnFlags_WidthStretch, 1.0f);
            ImGui::TableSetupColumn("After mean", ImGuiTableColumnFlags_WidthStretch, 1.0f);
            ImGui::TableSetupColumn("Reduction (%)", ImGuiTableColumnFlags_WidthStretch, 1.0f);
            ImGui::TableHeadersRow();
            const auto& a = _comparison[0]; const auto& b = _comparison[1];
            const double av[] = { a.cpuUpdate, a.cpuRender, a.gpuScene, a.draws };
            const double bv[] = { b.cpuUpdate, b.cpuRender, b.gpuScene, b.draws };
            for (int i = 0; i < 4; ++i)
            {
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::TextWrapped("%s", labels[i]);
                ImGui::TableSetColumnIndex(1);
                if (a.samples == 120) ImGui::Text("%.3f", av[i] / a.samples);
                else ImGui::TextUnformatted("--");
                ImGui::TableSetColumnIndex(2);
                if (b.samples == 120) ImGui::Text("%.3f", bv[i] / b.samples);
                else ImGui::TextUnformatted("--");
                ImGui::TableSetColumnIndex(3);
                if (a.samples == 120 && b.samples == 120 && av[i] > 0)
                {
                    const double before = av[i] / a.samples, after = bv[i] / b.samples;
                    ImGui::Text("%.1f%%", (before - after) * 100.0 / before);
                }
                else ImGui::TextUnformatted("--");
            }
            ImGui::EndTable();
        }
        ImGui::TextWrapped("CPU render measures Scene::Render wall time, including driver waits. It excludes editor UI, Present and FPS limiting. This comparison alone does not prove a CPU bottleneck.");
    }

    void OnGUI()
    {
        ImGui::SetNextWindowSize(ImVec2(760, 390), ImGuiCond_FirstUseEver);
        if (ImGui::Begin("GPU Profiler"))
        {
            ImGui::BeginDisabled(_captureSlot >= 0);
            ImGui::Checkbox("Measure GPU", &_enabled);
            ImGui::EndDisabled();
            ImGui::SameLine();
            if (ImGui::Button("Reset samples")) { _stats = {}; ++_generation; _skipped = _invalid = 0; }
            if (_failed) ImGui::TextUnformatted("GPU query creation/readback failed. Restart to retry.");
            ImGui::TextUnformatted("Milliseconds | latest / mean / max | last 120 valid samples");
            ImGui::TextUnformatted("Frame includes editor rendering, excludes Present. Results are delayed.");
            ImGui::TextUnformatted("Nested rows overlap; do not add them. Repeated passes sum per frame.");
            const char* names[] = { "Frame", "Scene", "CSM shadows", "Bloom", "SSR", "Grass cull", "Grass draw (all passes)" };
            for (int i = 0; i < PassCount; ++i)
            {
                const auto& s = _stats[i];
                if (!s.count) { ImGui::Text("%s: --", names[i]); continue; }
                double sum = 0, peak = 0;
                for (unsigned j = 0; j < s.count; ++j) { sum += s.values[j]; if (s.values[j] > peak) peak = s.values[j]; }
                ImGui::Text("%s: %.3f ms", names[i], sum / s.count);
            }
            ImGui::Text("Skipped frames/spans: %u | invalid frames: %u", _skipped, _invalid);
            ImGui::TextUnformatted("Reset after changing camera/settings. Missing passes retain their last samples.");
        }
        ImGui::End();
    }

private:
    void StartComparison(int slot)
    {
        _comparison[slot] = {};
        _captureSlot = slot;
        ++_captureId;
        _enabled = true;
    }
    struct Comparison
    {
        double cpuUpdate = 0, cpuRender = 0, gpuScene = 0, draws = 0;
        unsigned samples = 0;
    };
    std::array<Comparison, 2> _comparison = {};
    int _captureSlot = -1;
    UINT64 _captureId = 0;
    CpuClock::time_point _cpuSceneStart;
    double _cpuUpdate = 0;
    unsigned _sceneDraws = 0;
    bool _countSceneDraws = false;
    struct Span
    {
        Microsoft::WRL::ComPtr<ID3D11Query> begin, end;
        UINT64 start = 0, stop = 0;
        Pass pass = Frame;
    };
    struct FrameQueries
    {
        Microsoft::WRL::ComPtr<ID3D11Query> disjoint;
        std::array<Span, 256> spans;
        unsigned count = 0, generation = 0;
        UINT64 sequence = 0;
        bool pending = false, overflow = false;
        UINT64 captureId = 0;
        double cpuUpdate = 0, cpuRender = 0;
        unsigned draws = 0;
        bool cpuSceneReady = false;
    };
    struct Stats
    {
        std::array<double, 120> values = {};
        unsigned count = 0, next = 0;
        double latest = 0;
        void Add(double value) { latest = value; values[next] = value; next = (next + 1) % unsigned(values.size()); if (count < values.size()) ++count; }
    };
    std::array<FrameQueries, 6> _frames;
    std::array<Stats, PassCount> _stats;
    ID3D11DeviceContext* _context = nullptr;
    int _active = -1, _frameToken = -1;
    unsigned _skipped = 0, _invalid = 0, _generation = 0;
    UINT64 _sequence = 0;
    bool _enabled = true, _failed = false;
};
