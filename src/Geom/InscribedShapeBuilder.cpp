#include "InscribedShapeBuilder.h"
#include "OccUtils.h"

#include "BRepBuilderAPI_MakeVertex.hxx"
#include <GeomLProp_SLProps.hxx>


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

    /// work only for composolid or solid, after imprinting, excluding all shared faces
    std::vector<TopoDS_Face> getFreeFaces(const ItemType& s)
    {
        TopTools_MapOfShape aFwdFMap;
        TopTools_MapOfShape aRvsFMap;
        std::vector<TopoDS_Face> faces; // item must be the pointer to TopoDS_Shape or derived

        TopExp_Explorer ex(s, TopAbs_FACE);
        // faces.reserve(ex.Depth());
        TopLoc_Location aLocDummy;
        for (; ex.More(); ex.Next())
        {
            const TopoDS_Face& F = TopoDS::Face(ex.Current());
            // TopAbs_INTERNAL  can be a face embedded in a solid
            if (F.Orientation() == TopAbs_INTERNAL or F.Orientation() == TopAbs_EXTERNAL)
                continue;
            const Handle(Geom_Surface)& S = BRep_Tool::Surface(F, aLocDummy);
            if (S.IsNull())
                continue; // is that common?
            if (not isSharedFace(F, aFwdFMap, aRvsFMap))
                faces.push_back(F);
        }
        return faces;
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
        /// still can not find a void space inside the shape, then generete grid of OBB and search
        /// throw ProcessorError("not implement for grid void space search");
        return c;
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

    /// may ignore if surface is not elementary types
    std::vector<double> InscribedShapeBuilder::calcAreaPerSurfaceType(const ItemType& s)
    {
        auto v = std::vector<double>(SurfacTypeCount, 0.0);
        const bool elementarySurfaceOnly = true;
        // for each surface,  calc area, added to v[stype], no interior, skipShared
#if 1
        TopTools_MapOfShape aFwdFMap;
        TopTools_MapOfShape aRvsFMap;
        TopLoc_Location aLocDummy;

        for (TopExp_Explorer ex(s, TopAbs_FACE); ex.More(); ex.Next())
        {
            const TopoDS_Face& F = TopoDS::Face(ex.Current());

            if (isSharedFace(F, aFwdFMap, aRvsFMap))
                continue;
#else
        TopLoc_Location aLocDummy;
        auto freeFaces = getFreeFaces(s);
        for (const auto& F : freeFaces)
        {
#endif
            const Handle(Geom_Surface)& S = BRep_Tool::Surface(F, aLocDummy);
            if (S.IsNull())
                continue; // is that common?

            GeomAdaptor_Surface AS(S);
            GeomAbs_SurfaceType sType = AS.GetType();
            if (sType == GeomAbs_OffsetSurface)
            {
                const auto bs = AS.BasisSurface();
                sType = bs->GetType();
            }

            if (sType < GeomAbs_BezierSurface || sType == GeomAbs_SurfaceOfRevolution)
                v[int(sType)] += OccUtils::area(F);
            else
            {
                if (!elementarySurfaceOnly)
                    v[int(sType)] += OccUtils::area(F);
            }
        }
        return v;
    }

    /// distance, contact point and normal
    struct ContactInfo
    {
        double distance;
        PointType point;
        gp_Dir normal;
        Standard_Integer hash;
    };

    // return zero distance if there is contact and interference
    std::vector<ContactInfo> calcContact(const ItemType& s, const PointType& p)
    {
        TopLoc_Location aLocDummy;
        auto freeFaces = getFreeFaces(s);
        std::vector<ContactInfo> infos;
        for (const auto& F : freeFaces)
        {
            auto V = BRepBuilderAPI_MakeVertex(p).Vertex();
            auto dss = BRepExtrema_DistShapeShape(); // GeomAPI_ExtremaSurfaceSurface
            dss.LoadS1(F);
            dss.LoadS2(V);
            dss.Perform();
            auto contact = dss.PointOnShape1(1);
            auto distance = dss.Value();
            Standard_Integer hash = HashCode(F, INT_MAX);
#if 0
            double u, v;
            dss.ParOnFaceS1(1, u, v); // Exception here, why?

            const Handle(Geom_Surface)& S = BRep_Tool::Surface(F, aLocDummy);
            if (S.IsNull())
                continue; // this has been filtered out by getFreeFaces()
            GeomAdaptor_Surface AS(S);

            const double precision = 1e-3;
            GeomLProp_SLProps props(AS.Surface(), u, v, 1 /* max 1 derivation */, precision);
            auto normal = props.Normal();
#else
            gp_Dir normal;
#endif
            infos.emplace_back(ContactInfo{distance, contact, normal, hash});
        }
        return infos;
    }

    /// TODO: not completed, how to grow the shape?
    InscribedShapeType calcInscribedSphere(const ItemType& s, const GeometryProperty& gp, const Bnd_OBB& obb)
    {
        auto c = calcInitPoint(s, gp, obb);
        auto infos = calcContact(s, c);
        std::vector<double> dist;
        for (const auto& info : infos)
        {
            dist.push_back(info.distance);
        }
        auto min = *std::min_element(dist.begin(), dist.end());
        Standard_Real initLength = Precision::Confusion();
        if (min > 0)
            initLength = min;
        // todo: based on infos, it may be possible to quickly get local max inscribed sphere
        Bnd_Sphere ss(gp_XYZ(c.X(), c.Y(), c.Z()), initLength, 10, 10); // what does the U, V mean here?
        return ss;
    }

    /// TODO: not completed, maybe based on inscribed_sphere
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
        auto a = estimateShapeType(s, gp, obb);
        auto ins = calcInscribedSphere(s, gp, obb);
        return ins;
        /*
        if (a == GeomAbs_Plane)
        {
            return calcInscribedOBB(s, gp, obb);
        }
        else if (a == GeomAbs_Sphere)
        {
            return ins;
        }
        else
        {
            return calcInscribedCylinder(s, gp, obb);
        }
        */
    }

} // namespace Geom