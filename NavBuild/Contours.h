#pragma once
#include "HeightFieldBase.h"

class Contours : public HeightFieldBase
{
public:
    Contours(const class CompactHeightField& heightField, const NavBuildSettings& settings);

    const vector<vector<ContourVertex>>& GetContours() const { return _contours; }
    //void GreedySimplify(float maxError);
    void RDPSimplify(float maxError);

private:
    using EdgeMap = unordered_multimap<Vertex2D, int, Vertex2DHash>;
    bool FindWalkStartPos(const CompactHeightField& heightField, const int region, int& startX, int& startZ, int & findSpanIdx, int & findDir);
    vector<ContourVertex> BuildOneLoopByWalking(const CompactHeightField& heightField, int startX, int startZ, int startSpan, int startDir);
    int GetNeighborSpanIdx(const CompactHeightField& heightField, int cx, int cz, int targetY);
    ContourVertex GetCornerVertex(const CompactHeightField& heightField, int cx, int cz, int spanIdx, int dir, int neighborRegion);

    void RDP(const vector<ContourVertex>& loop, vector<bool>& keep, int si, int ei, float maxError);

private:
    float PointToSegmentDist(const Vertex& p, const Vertex& a, const Vertex& b);

private:
    vector<vector<ContourVertex>> _contours;
    const CompactHeightField& _heightField;
};