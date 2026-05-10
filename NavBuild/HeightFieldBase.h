#pragma once
class HeightFieldBase
{
public:
    HeightFieldBase() = delete;
    HeightFieldBase(const Bounds& bound, float cellSize, float cellHeight);
    HeightFieldBase(const HeightFieldBase& other);

public:
    int GetWidth() const { return _width; }
    int GetDepth() const { return _depth; }

    Vec3 GetBoundMin() const { return _bmin; }
    Vec3 GetBoundMax() const { return _bmax; }

    float GetCellSize() const { return _cs; }
    float GetCellHeight() const { return _ch; }

    void GetCellWorldPos(int cx, int cz, OUT float& wx, OUT float& wz) const;
    void GetVertexWorldPos(int cx, int cz, float & wx, float & wz) const;
    void GetVertexWorldPos(float cx, float cz, float& wx, float& wz) const;
    void GetWorldHeight(int cy, OUT float& wy) const;
    void GetWorldHeight(float cy, OUT float& wy) const;
    int GetColumnIndex(int cx, int cz) const { return cx + cz * _width; }

    Vec3 ToWorldPos(const Vec3& v) const;
    Vec3 ToNavPos(const Vec3& v) const;

    int Cross2D(const Vertex& a, const Vertex& b, const Vertex& c) const;
    float Cross2D(const Vec3& a, const Vec3& b, const Vec3& c) const;
    int Dot2D(const Vertex& a, const Vertex& b, const Vertex& c) const;
    bool IsConvex(const Vertex& a, const Vertex& b, const Vertex& c) const;

    bool PointInTri2D(const Vertex& p, const Vertex& a, const Vertex& b, const Vertex& c) const;
    bool PointInTri2D(const Vec3& p, const Vec3& a, const Vec3& b, const Vec3& c) const;

    float GetTriY(float x, float z, const Vec3& v0, const Vec3& v1, const Vec3& v2) const;

    bool IsPointInPoly(const Vec3& point, const vector<Vertex>& verts, const Poly& poly) const;
    bool IsPointInTriangle(const Vec3& point, const vector<Vec3>& verts, const Triangle& tri) const;

protected:
    void GetCellIndex(float wx, float wz, OUT int& cx, OUT int& cz) const;
    int GetCellHeight(float wy) const;

    template<typename Func>
    void ParallelFor(int startInclusive, int endExclusive, Func&& func)
    {
        if (endExclusive <= startInclusive)
            return;

        std::atomic<int> nextIndex = startInclusive;

        const unsigned int hwThreadCount = std::thread::hardware_concurrency();
        const unsigned int workerCount = std::max(1u, hwThreadCount);

        std::vector<std::thread> workers;
        workers.reserve(workerCount);

        for (unsigned int workerIdx = 0; workerIdx < workerCount; ++workerIdx)
        {
            workers.emplace_back([&]()
                {
                    while (true)
                    {
                        int index = nextIndex.fetch_add(1);

                        if (index >= endExclusive)
                            break;

                        func(index);
                    }
                });
        }

        for (std::thread& worker : workers)
        {
            worker.join();
        }
    }

    template<typename Func>
    void ParallelForRange(int startInclusive, int endExclusive, Func&& func)
    {
        if (endExclusive <= startInclusive)
            return;

        const int totalCount = endExclusive - startInclusive;

        unsigned int hwThreadCount = std::thread::hardware_concurrency();
        unsigned int workerCount = std::max(1u, hwThreadCount);

        workerCount = std::min<unsigned int>(workerCount, totalCount);

        std::vector<std::thread> workers;
        workers.reserve(workerCount);

        const int chunkSize = (totalCount + workerCount - 1) / workerCount;

        for (unsigned int workerIdx = 0; workerIdx < workerCount; ++workerIdx)
        {
            const int rangeStart = startInclusive + static_cast<int>(workerIdx) * chunkSize;
            const int rangeEnd = std::min(rangeStart + chunkSize, endExclusive);

            if (rangeStart >= rangeEnd)
                break;

            workers.emplace_back([rangeStart, rangeEnd, &func]()
                {
                    for (int i = rangeStart; i < rangeEnd; ++i)
                    {
                        func(i);
                    }
                });
        }

        for (std::thread& worker : workers)
        {
            worker.join();
        }
    }

protected:
    int _width = 0;   // x 방향 cell 개수
    int _depth = 0;   // z 방향 cell 개수

    const Vec3 _bmin;       // 월드 최소
    const Vec3 _bmax;       // 월드 최대

    const float _cs; // cell size (xz)
    const float _ch; // cell height (y)

    static const int _dx[4];
    static const int _dz[4];
};

