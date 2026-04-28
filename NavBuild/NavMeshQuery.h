#pragma once
class PolyMeshField;
class DetailMeshField;

class NavMeshQuery
{
public:
    NavMeshQuery(const PolyMeshField& polyMeshField, const DetailMeshField& detailMeshField);

    vector<Vec3> FindPath(const Vec3& start, const Vec3& end) const;

private:
    const PolyMeshField& _polyMeshField;
    const DetailMeshField& _detailMeshField;
};
