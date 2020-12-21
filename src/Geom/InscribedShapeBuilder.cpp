#include "InscribedShapeBuilder.h"
#include "OccUtils.h"

#include "BRepBuilderAPI_MakeVertex.hxx"


namespace Geom
{
    /// this function is not thread-safe
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


    /// generate coordinate index, then save all, otherwise, local not global max
    /// const std::vector<bool> voids, or 3D array
    PointType searchOBBGrid(const ItemType& shape, const Bnd_OBB& obb)
    {
        gp_Trsf trsf;
        auto ax3 = gp_Ax3(obb.Center(), obb.ZDirection(), obb.XDirection());
        trsf.SetTransformation(ax3);

        const int dim = 3;
        const int gz[dim] = {4, 4, 4}; // should consider aspect ratio
        const Standard_Real s[dim] = {obb.XHSize() / gz[0], obb.XHSize() / gz[1], obb.XHSize() / gz[2]};
        auto& c = obb.Center();
        int i[dim] = {-gz[0], -gz[1], -gz[2]}; // shift to cell center by i+0.5
        for (; i[2] < gz[2]; i[2]++)
        {
            for (; i[1] < gz[1]; i[1]++)
            {
                for (; i[0] < gz[0]; i[0]++)
                {
                    PointType p = {s[0] * (i[0] + 0.5), s[1] * (i[1] + 0.5), s[20] * (i[2] + 0.5)};
                    p.Transform(trsf);
                    if (OccUtils::distance(shape, p) > Precision::Confusion())
                    {
                        // size_t vi = i
                        /// assigned to result value voids[]
                        return p;
                    }
                }
            }
        }
        throw ProcessorError("searched all grid, can not find a void point");
    }

    PointType calcInitPoint(const ItemType& s, const GeometryProperty& gp, const Bnd_OBB& obb)
    {
        PointType cm = PointType(gp.centerOfMass[0], gp.centerOfMass[1], gp.centerOfMass[2]);
        if (OccUtils::distance(s, cm) > Precision::Confusion())
            return cm;
        PointType cb{obb.Center()};
        if (OccUtils::distance(s, cb) > Precision::Confusion())
            return cb;
        PointType c{(cm.X() + cb.X()) * 0.5, (cm.Y() + cb.Y()) * 0.5, (cm.Z() + cb.Z()) * 0.5};
        if (OccUtils::distance(s, c) > Precision::Confusion())
            return c;
        /// still can not find a void space inside the shape, then generete grid of OBB, then
        throw ProcessorError("not implement for grid void space search");
    }


    /*
        template <typename ShapeT>
        ShapeT initInscribedShape(const ItemType& s, const GeometryProperty& gp, const Bnd_OBB& obb);
        template <>
        Bnd_Sphere initInscribedShape<Bnd_Sphere>(const ItemType& s, const GeometryProperty& gp, const Bnd_OBB& obb)
        {

            return Bnd_Sphere(gp_XYZ(c.X(), c.Y(), c.Z()), Precision::Confusion(), 10, 10);
        }

        template <> Bnd_OBB initInscribedShape<Bnd_OBB>(const ItemType& s, const GeometryProperty& gp, const Bnd_OBB&
       obb)
        {
            PointType c{obb.Center()};
            // if ()  detect if the point is out of this shape,
            // c = PointType(gp.centerOfMass[0], gp.centerOfMass[1],gp.centerOfMass[2]);

            return Bnd_OBB();
        }


    template <typename ShapeT> ShapeT _calcInscribedShape(const ItemType& s, const ShapeT& initShape)
    {
        ShapeT maxShape = initShape;

        // center must not moved out Bnd_Bnd,  radius

        return maxShape;
    }
    */

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


    /// how to grow the shape? DOF
    InscribedShapeType calcInscribedSphere(const ItemType& s, const GeometryProperty& gp, const Bnd_OBB& obb)
    {
        auto c = calcInitPoint(s, gp, obb);
        Standard_Real initLength = 1;                                   // Precision::Confusion()
        Bnd_Sphere ss(gp_XYZ(c.X(), c.Y(), c.Z()), initLength, 10, 10); // what does the U, V mean here?
        return ss;
    }

    InscribedShapeType calcInscribedOBB(const ItemType& s, const GeometryProperty& gp, const Bnd_OBB& obb)
    {
        Bnd_OBB b(obb);
        Standard_Real initLength = 1; // Precision::Confusion()
        b.SetXComponent(obb.XDirection(), initLength);
        b.SetYComponent(obb.YDirection(), initLength);
        b.SetZComponent(obb.ZDirection(), initLength);
        return b;
    }

    InscribedShapeType calcInscribedCylinder(const ItemType& s, const GeometryProperty& gp, const Bnd_OBB& obb)
    {
        InscribedShapeType ret;

        return ret;
    }

    //

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