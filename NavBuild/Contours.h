#pragma once
#include "HeightFieldBase.h"

class Contours : public HeightFieldBase
{
    struct WalkStartInfo
    {
        int startX = -1;
        int startZ = -1;
        int spanIdx = -1;
        int dir = -1;

        bool IsValid() const
        {
            return !(startX == -1 || startZ == -1 || spanIdx == -1 || dir == -1);
        }
    };

public:
    Contours(const class CompactHeightField& heightField, const NavBuildSettings& settings);

    const vector<vector<ContourVertex>>& GetContours() const { return _contours; }
    //void GreedySimplify(float maxError);
    void RDPSimplify(float maxError);

private:
    using EdgeMap = unordered_multimap<Vertex2D, int, Vertex2DHash>;
    void FindWalkStartInfos(const CompactHeightField& heightField, OUT vector<WalkStartInfo>& walkStartInfos, int regionSize);
    vector<ContourVertex> BuildOneLoopByWalking(const CompactHeightField& heightField, const WalkStartInfo& walkStartInfo);
    int GetNeighborSpanIdx(const CompactHeightField& heightField, int cx, int cz, int targetY);
    ContourVertex GetCornerVertex(const CompactHeightField& heightField, int cx, int cz, int spanIdx, int dir, int neighborRegion);

    void RDP(const vector<ContourVertex>& loop, vector<bool>& keep, int si, int ei, float maxError);

private:
    float PointToSegmentDist(const Vertex& p, const Vertex& a, const Vertex& b);

private:
    vector<vector<ContourVertex>> _contours;
    const CompactHeightField& _heightField;
};