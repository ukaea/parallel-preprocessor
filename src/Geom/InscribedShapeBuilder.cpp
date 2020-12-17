#include "InscribedShapeBuilder.h"
#include "OccUtils.h"

namespace Geom
{

    bool isSharedFace(const TopoDS_Face& F, TopTools_MapOfShape& aFwdFMap, TopTools_MapOfShape& aRvsFMap)
    {
        TopAbs_Orientation anOri = F.Orientation();
        Standard_Boolean isFwd = anOri == TopAbs_FORWARD;
        Standard_Boolean isRvs = Standard_False;
        if (!isFwd)
        {
            isRvs = anOri == TopAbs_REVERSED;
        }
        if ((isFwd && !aFwdFMap.Add(F)) || (isRvs && !aRvsFMap.Add(F)))
        {
            return true;
        }
        return false;
    }

    ///
    std::vector<double> InscribedShapeBuilder::calcAreaPerSurfaceType(const ItemType& s)
    {
        auto v = std::vector<double>(SurfacTypeCount, 0.0);
        // for each surface,  calc area, added to v[stype], no interior, skipShared
        TopTools_MapOfShape aFwdFMap;
        TopTools_MapOfShape aRvsFMap;
        TopLoc_Location aLocDummy;

        for (TopExp_Explorer ex(s, TopAbs_FACE); ex.More(); ex.Next())
        {
            const TopoDS_Face& F = TopoDS::Face(ex.Current());

            if (isSharedFace(F, aFwdFMap, aRvsFMap))
                continue;

            const Handle(Geom_Surface)& S = BRep_Tool::Surface(F, aLocDummy);
            if (S.IsNull())
                continue; // is that common?

            GeomAdaptor_Surface AS(S);
            GeomAbs_SurfaceType sType = AS.GetType();
            if (sType == GeomAbs_OffsetSurface)
            {
                // TODO: is that possible to get elementary surface type of OffsetSurface
                v[int(sType)] += OccUtils::area(F);
            }
            else
            {
                v[int(sType)] += OccUtils::area(F);
            }
        }
        return v;
    }



    ///
    InscribedShapeType calcInscribedSphere(const ItemType& s, const GeometryProperty& gp, const Bnd_OBB& obb)
    {
        InscribedShapeType ret;

        return ret;
    }

    InscribedShapeType calcInscribedOBB(const ItemType& s, const GeometryProperty& gp, const Bnd_OBB& obb)
    {
        InscribedShapeType ret;

        return ret;
    }

    InscribedShapeType calcInscribedCylinder(const ItemType& s, const GeometryProperty& gp, const Bnd_OBB& obb)
    {
        InscribedShapeType ret;

        return ret;
    }

    /// return inscribed shape type can be judged by vector size
    /// todo: find a way to decided, which is the best/max inscribed shape.
    ///       coil is fine
    ///       half cylinder, half sphere?  max (bbox and inscribed_volume)
    /// there maybe a way to check hollow shape, or outer shape?
    InscribedShapeType InscribedShapeBuilder::calcInscribedShape(const ItemType& s, const GeometryProperty& gp,
                                                                 const Bnd_OBB& obb)
    {
        InscribedShapeType ret;
        auto a = estimateShapeType(s, gp, obb);
        if (a == GeomAbs_Plane)
        {
            return calcInscribedOBB(s, gp, obb);
        }
        else if (a == GeomAbs_Sphere)
        {
            return calcInscribedSphere(s, gp, obb);
        }
        else
        {
            return calcInscribedCylinder(s, gp, obb);
        }
        return ret;
    }

} // namespace Geom