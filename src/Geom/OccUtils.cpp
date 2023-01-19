/***************************************************************************
 *   Copyright (c) 2002 Jürgen Riegel <juergen.riegel@web.de>              *
 *   This file was originated from the FreeCAD CAx development system.     *
 *   Copyright (c) 2019 Qingfeng Xia  <qingfeng.xia@ukaea.uk>              *
 *   This file is part of parallel-preprocessor CAx development system.    *
 *                                                                         *
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU Library General Public           *
 *   License as published by the Free Software Foundation; either          *
 *   version 2 of the License, or (at your option) any later version.      *
 *                                                                         *
 *   This library  is distributed in the hope that it will be useful,      *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this library; see the file COPYING.LIB. If not,    *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,         *
 *   Suite 330, Boston, MA  02111-1307, USA                                *
 *                                                                         *
 ***************************************************************************/

/// shape operation functions, adapted from FreeCAD project
/// src/Mod/Part/App/TopoShape.cpp

#include "OccUtils.h"
#include "PPP/Logger.h"
#include "PPP/TypeDefs.h"
#include "PPP/UniqueId.h"
#include "PPP/Utilities.h"

// from Salome Geom module
#include <GEOMAlgo_Gluer2.hxx>

namespace Geom
{
    using namespace PPP;


    /* static utility metheds */
    namespace OccUtils
    {
        void saveShape(const std::vector<TopoDS_Shape>& shapes, const std::string file_name)
        {
            TopoDS_Builder cBuilder;
            TopoDS_Compound merged;
            cBuilder.MakeCompound(merged);
            for (auto& item : shapes)
            {
                cBuilder.Add(merged, item);
            }
            saveShape(merged, file_name);
        }

        /// this function may serve as unit test for GeometryWriter
        void saveAssemblyToStepFile(const TopoDS_Shape& shape, const std::string file_name)
        {
            Handle(XCAFApp_Application) hApp = XCAFApp_Application::GetApplication();
            Handle(TDocStd_Document) aDoc;
            hApp->NewDocument(TCollection_ExtendedString("MDTV-CAF"), aDoc);

            Handle(XCAFDoc_ShapeTool) shapeTool = XCAFDoc_DocumentTool::ShapeTool(aDoc->Main());
            TDF_Label lab1 = XCAFDoc_DocumentTool::ShapeTool(aDoc->Main())->NewShape();
            shapeTool->SetShape(lab1, shape);
            TDataStd_Name::Set(lab1, file_name.c_str());

            shapeTool->UpdateAssemblies();
            Interface_Static::SetCVal("write.step.schema", "AP214IS");
            Interface_Static::SetIVal("write.step.assembly", 1);

            STEPCAFControl_Writer writer;
            writer.Transfer(aDoc, STEPControl_AsIs);
            writer.Write(file_name.c_str());
        }

        void saveShape(const TopoDS_Shape& shape, const std::string file_name)
        {
            if (Utilities::hasFileExt(file_name, "step") || Utilities::hasFileExt(file_name, "stp"))
            {
                STEPControl_Writer aWriter;
                IFSelect_ReturnStatus aStat = aWriter.Transfer(shape, STEPControl_AsIs);
                aStat = aWriter.Write(file_name.c_str());
                if (aStat != IFSelect_RetDone)
                    std::cout << "Step file writing error" << std::endl;
            }
            else if (Utilities::hasFileExt(file_name, "brep") || Utilities::hasFileExt(file_name, "brp"))
            {
                BRepTools::Write(shape, file_name.c_str());
            }
            else
            {
                LOG_F(ERROR, "file extension is not supported for %s", file_name.c_str());
            }
        }

        TopoDS_Shape loadShape(const std::string file_name)
        {
            if (not fs::exists(file_name))
                throw std::runtime_error("file not exist" + file_name);

            if (Utilities::hasFileExt(file_name, "brep") || Utilities::hasFileExt(file_name, "brp"))
            {
                BRep_Builder cBuilder;
                TopoDS_Shape shape;

                BRepTools::Read(shape, file_name.c_str(), cBuilder);
                if (shape.IsNull())
                    throw std::runtime_error("brep read data is Null for:" + file_name);
                return shape;
            }
            else if (Utilities::hasFileExt(file_name, "step") || Utilities::hasFileExt(file_name, "stp"))
            {
                STEPControl_Reader aReader;
                IFSelect_ReturnStatus stat = aReader.ReadFile(file_name.c_str());

                if (stat != IFSelect_RetDone)
                {
                    throw std::runtime_error("step file read error for " + file_name);
                }

                aReader.TransferRoots();   // translate all roots, return the number of transferred
                return aReader.OneShape(); // a compound if there are more than one shapes
            }
#if OCC_VERSION_HEX >= 0x070400
            else if (Utilities::hasFileExt(file_name, "stl"))
            {
                Handle(Poly_Triangulation) aTriangulation = RWStl::ReadFile(file_name.c_str());
                // the second default parameter: Message_ProgressIndicator
                // extracted from readstl() in XSDRAWSTLVRML.cxx, does that means can only read single face?
                TopoDS_Face aFace;
                BRep_Builder aB;
                aB.MakeFace(aFace);
                aB.UpdateFace(aFace, aTriangulation);
                return aFace;
            }
#else
            /// in lower version, each triangle is read as TopoDS_Face which is not efficient!
#endif
            else
            {
                throw std::runtime_error("file suffix not supported for" + file_name);
            }
        }

        TopoDS_Shape loadShape(std::shared_ptr<std::stringstream> ss)
        {
            BRep_Builder cBuilder;
            TopoDS_Shape shape;

            BRepTools::Read(shape, (*ss.get()), cBuilder);
            return shape;
        }

        /// only works on compound? return face count
        std::map<std::string, int> exploreShape(const TopoDS_Shape& aShape)
        {
            using namespace std;
            std::map<std::string, int> info;
            TopAbs_ShapeEnum type = aShape.ShapeType();
            // int Nb_compounds, Nb_compsolids, Nb_solids, Nb_shells, Nb_faces, Nb_wires, Nb_edges, Nb_vertices;
            if (type == TopAbs_COMPOUND)
            {
                ShapeExtend_Explorer anExp;
                Handle(TopTools_HSequenceOfShape) seqShape = new TopTools_HSequenceOfShape;
                Handle(TopTools_HSequenceOfShape) compounds, compsolids, solids, shells, faces, wires, edges, vertices;

                anExp.SeqFromCompound(aShape, false);
                anExp.DispatchList(seqShape, vertices, edges, wires, faces, shells, solids, compsolids, compounds);
                info = {{"compounds", compounds->Length()}, {"compsolids", compsolids->Length()},
                        {"solids", solids->Length()},       {"shells", shells->Length()},
                        {"faces", faces->Length()},         {"wires", wires->Length()},
                        {"vertices", vertices->Length()},   {"edges", edges->Length()}};
            }
            else if (type == TopAbs_COMPSOLID)
            {
                int Nb_solids = 0;
                int Nb_faces = 0;
                for (TopExp_Explorer anExp(aShape, TopAbs_SOLID); anExp.More(); anExp.Next())
                {
                    Nb_solids += 1;
                }
                for (TopExp_Explorer anExp(aShape, TopAbs_FACE); anExp.More(); anExp.Next())
                {
                    Nb_faces += 1;
                }
                info = {{"compounds", 0}, {"compsolids", 1}, {"solids", Nb_solids}, {"faces", Nb_faces}};
            }
            else
            {
                throw std::runtime_error("Error: only COMPSOLID and COMPOUND are supported as input shape");
            }
            return info;
        }

        unsigned long countSubShapes(const TopoDS_Shape& _shape, const TopAbs_ShapeEnum Type)
        {
            if (Type == TopAbs_SHAPE)
            {
                int count = 0;
                for (TopoDS_Iterator it(_shape); it.More(); it.Next())
                    ++count;
                return count;
            }
            TopTools_IndexedMapOfShape anIndices; // key are unique
            TopExp::MapShapes(_shape, Type, anIndices);
            return anIndices.Extent();
        }

        TopoDS_Shape unifyFaces(const TopoDS_Shape& _shape)
        {
            LOG_F(INFO, "in unifyFaces(), face count before simplify = %lu", countSubShapes(_shape, TopAbs_FACE));

            // SimplifyShape,  if tangent faces and edges lie on the same geometry
            // must turn on unify edges, otherwise, faces number will not decrease!
            ShapeUpgrade_UnifySameDomain USD(_shape, false, true, false);
            // UnifyFaces mode on, UnifyEdges mode off, ConcatBSplines mode off.
            // USD.SetLinearTolerance(1e-4); // make tol bigger, it does not help remove faces
            USD.Build();

            ShapeUpgrade_UnifySameDomain USD2(USD.Shape(), true, true, false);
            USD2.Build();
            TopoDS_Shape Result = USD2.Shape();
            LOG_F(INFO, "in unifyFaces(), face count after simplify = %lu", countSubShapes(Result, TopAbs_FACE));
            return Result;
        }


        TopoDS_Shape glueFaces(const TopoDS_Shape& s, const Standard_Real tolerance)
        {
            TopoDS_Shape aRes; // empty shape, as the returned if error
            Standard_Integer iErr;
            GEOMAlgo_Gluer2 gluer; // version 2 of gluer, instead of version 1
            gluer.SetArgument(s);
            gluer.SetKeepNonSolids(false); // what does that mean?
            if (tolerance > 0.0)
                gluer.SetTolerance(tolerance);

            // 2. Detect interfered shapes, it can NOT be skipped  even there is no interference
            gluer.Detect();
            iErr = gluer.ErrorStatus();
            if (iErr)
            {
                switch (iErr)
                {
                case 11:
                    Standard_Failure::Raise("GEOMAlgo_GlueDetector failed");
                    break;
                case 13:
                case 14:
                    Standard_Failure::Raise("PerformImagesToWork failed");
                    break;
                default:
                {
                    // description of all errors see in GEOMAlgo_Gluer2.cxx
                    TCollection_AsciiString aMsg("Error in GEOMAlgo_Gluer2 with code ");
                    aMsg += TCollection_AsciiString(iErr);
                    Standard_Failure::Raise(aMsg.ToCString());
                    break;
                }
                }
                return aRes;
            }

            // 3. Set shapes to glue. If the operator is absent, the whole gluing will be done
            // gluer.SetShapesToGlue();

            // 4. Gluing
            gluer.Perform();
            // checking errors and warnings
            iErr = gluer.ErrorStatus();
            if (iErr)
            {
                switch (iErr)
                {
                case 11:
                    Standard_Failure::Raise("GEOMAlgo_GlueDetector failed");
                    break;
                case 13:
                case 14:
                    Standard_Failure::Raise("PerformImagesToWork failed");
                    break;
                default:
                {
                    // description of all errors see in GEOMAlgo_Gluer2.cxx
                    TCollection_AsciiString aMsg("Error in GEOMAlgo_Gluer2 with code ");
                    aMsg += TCollection_AsciiString(iErr);
                    Standard_Failure::Raise(aMsg.ToCString());
                    break;
                }
                }
                return aRes;
            }
            Standard_Integer iWrn = gluer.WarningStatus();
            if (iWrn)
            {
                switch (iWrn)
                {
                case 1:
                    LOG_F(WARNING, "No shapes to glue (ignore this if shape has been merged)");
                    break;
                default:
                    // description of all warnings see in GEOMAlgo_Gluer2.cxx
                    LOG_F(WARNING, "Warning in GEOMAlgo_Gluer2 with code %d", iWrn);
                    break;
                }
            }

            return gluer.Shape();
        }

        std::shared_ptr<std::stringstream> saveShapeToStream(const std::vector<TopoDS_Shape>& shapes,
                                                             const std::string& fileType)
        {
            std::shared_ptr<std::stringstream> ss = std::make_shared<std::stringstream>();
            std::string su = fileType;

            TopoDS_Builder cBuilder;
            TopoDS_Compound merged;
            cBuilder.MakeCompound(merged);
            for (const auto& item : shapes)
            {
                cBuilder.Add(merged, item);
            }

            if (su == "brep" or su == "brp")
            {
                BRepTools::Write(merged, (*ss.get()));
            }
            else if (su == "stl")
            {
                StlAPI_Writer STLwriter;
                const char* tmp_file = "./tmp_stl.stl";
                STLwriter.Write(merged, tmp_file); // no API to Write into OStream
                std::ifstream ifs(tmp_file);
                if (ifs)
                {
                    (*ss.get()) << ifs.rdbuf();
                    ifs.close(); // tmp_file delete is not necessary
                }
            }
            else
            {
                return nullptr;
            }
            return ss;
        }


        TopoDS_Compound createCompound(const ItemContainerType theSolids,
                                       std::shared_ptr<const MapType<ItemHashType, ShapeErrorType>> suppressed)
        {
            TopoDS_Builder cBuilder;
            TopoDS_Compound merged;
            cBuilder.MakeCompound(merged);
            size_t i = 0;
            for (const auto& item : theSolids)
            {
                if (suppressed)
                {
                    bool itemSuppressed = (*suppressed).at(item.first) != ShapeErrorType::NoError;
                    if (not itemSuppressed)
                        cBuilder.Add(merged, item.second);
                }
                else
                {
                    cBuilder.Add(merged, item.second);
                }
                i++;
            }
            VLOG_F(LOGLEVEL_DEBUG, "result compound has %lu solids", i);
            return merged;
        }

        TopoDS_CompSolid createCompSolid(const ItemContainerType theSolids,
                                         std::shared_ptr<const MapType<ItemHashType, ShapeErrorType>> suppressed)
        {
            TopoDS_Builder cBuilder;
            // TopoDS_Shape* merged;
            TopoDS_CompSolid merged;
            cBuilder.MakeCompSolid(merged);
            size_t i = 0;
            for (const auto& item : theSolids)
            {
                if (suppressed)
                {
                    bool itemSuppressed = (*suppressed).at(item.first) != ShapeErrorType::NoError;
                    if (not itemSuppressed)
                        cBuilder.Add(merged, item.second);
                }
                else
                {
                    cBuilder.Add(merged, item.second);
                }
                i++;
            }
            return merged;
        }

        TopoDS_Shape createCompound(std::vector<TopoDS_Shape> shapes)
        {
            TopoDS_Builder cBuilder;
            // to do compound
            TopoDS_Compound merged;
            cBuilder.MakeCompound(merged);
            for (const auto& item : shapes)
            {
                cBuilder.Add(merged, item);
            }
            return merged;
        }

        bool isBndBoxOverlapped(const Bnd_OBB& thisBox, const Bnd_OBB& otherBox)
        {
            return not thisBox.IsOut(otherBox);
        }

        /** gap can control strong or weak collision/overlapping */
        bool isBndBoxOverlapped(const Bnd_Box& thisBox, const Bnd_Box& otherBox, Standard_Real gap)
        {
            /// NOTE: BndBox.Distance() can deal with infinity boundbox correctly
            /// gap can not be zero, to do float point comparison
            Standard_Real tol = Precision::Confusion(); // abs tol in default length unit, mm
            if (gap < 0 and gap > -tol)                 // Precision::Confusion() = 1e-7
                gap = -tol;
            if (gap > 0 and gap < tol)
                gap = tol;

            Standard_Real tXmin, tYmin, tZmin, tXmax, tYmax, tZmax;
            Standard_Real oXmin, oYmin, oZmin, oXmax, oYmax, oZmax;
            thisBox.Get(tXmin, tYmin, tZmin, tXmax, tYmax, tZmax);
            otherBox.Get(oXmin, oYmin, oZmin, oXmax, oYmax, oZmax);

            if (tXmin > oXmax + gap || tXmax < oXmin - gap)
                return false;
            if (tYmin > oYmax + gap || tYmax < oYmin - gap)
                return false;
            if (tZmin > oZmax + gap || tZmax < oZmin - gap)
                return false;
            return true;
        }

        bool isBndBoxCoincident(const TopoDS_Shape& s, const Bnd_Box& otherBox, Standard_Real reltolerance)
        {
            return isBndBoxCoincident(calcBndBox(s), otherBox, reltolerance);
        }
        bool isBndBoxCoincident(const Bnd_Box& thisBox, const Bnd_Box& otherBox, Standard_Real reltolerance)
        {
            const int dim = 3;
            Standard_Real A[dim * 2];
            Standard_Real B[dim * 2];
            thisBox.Get(A[0], A[1], A[2], A[3], A[4], A[5]);
            otherBox.Get(B[0], B[1], B[2], B[3], B[4], B[5]);
            Standard_Real T[dim]; // absolute tol in each dim
            Standard_Real reltol = std::abs(reltolerance);
            // assert boundbox is not zero, reltolerance must not be zero
            for (int i = 0; i < dim; i++)
            {
                T[i] = std::max(A[i + dim] - A[i], B[i + dim] - B[i]) * reltol;
                if (T[i] < Precision::Confusion())
                    T[i] = Precision::Confusion();
                if (A[i] < B[i] - T[i] || A[i + dim] > B[i + dim] + T[i])
                    return false;
            }
            return true;
        }
        Bnd_Box calcBndBox(const TopoDS_Shape& s)
        {
            Bnd_Box thisBox;
            thisBox.SetGap(0.0);
            BRepBndLib::Add(s, thisBox);
            return thisBox;
        }

        bool floatEqual(double a, double b, double reltol)
        {
            double ref = std::min(std::abs(a), std::abs(b));
            double tol = std::max(std::abs(ref * reltol), Precision::Confusion() * reltol);
            if (std::abs(a - b) < tol)
                return true;
            else
                return false;
        }

        Standard_Boolean isCoincidentDomain(const TopoDS_Shape& s1, const TopoDS_Shape& s2)
        {
            Bnd_Box boundingBox;
            boundingBox.SetGap(0.0); // todo: set a different control tolerance?
            BRepBndLib::Add(s1, boundingBox);

            Bnd_Box boundingBox2;
            boundingBox.SetGap(0.0);
            BRepBndLib::Add(s2, boundingBox2);

            bool areaEqual = floatEqual(area(s1), area(s2));
            return isBndBoxCoincident(boundingBox, boundingBox2) && areaEqual;
        }


        void _printShapeList(const TopTools_ListOfShape& listOfMod, const std::string name)
        {
            if (listOfMod.Extent())
            {
                for (const auto& ss : listOfMod)
                {
                    std::cout << name << " shape type = " << ss.ShapeType() << std::endl;
                }
            }
        }

        // https://github.com/3drepo/occt/blob/master/src/BRepGProp/BRepGProp.cxx
        GeometryProperty geometryProperty(const TopoDS_Shape& shape)
        {
            GProp_GProps v_props, s_props, l_props;
            BRepGProp::LinearProperties(shape, l_props);
            BRepGProp::SurfaceProperties(shape, s_props);
            BRepGProp::VolumeProperties(shape, v_props);

            int edgeCount = 0;
            TopExp_Explorer Ex(shape, TopAbs_EDGE);
            while (Ex.More())
            {
                edgeCount++;
                Ex.Next();
            }

            int faceCount = 0;
            {
                TopExp_Explorer Ex(shape, TopAbs_FACE);
                while (Ex.More())
                {
                    faceCount++;
                    Ex.Next();
                }
            }

            int solidCount = 0;
            {
                TopExp_Explorer Ex(shape, TopAbs_SOLID);
                while (Ex.More())
                {
                    solidCount++;
                    Ex.Next();
                }
            }

            GeometryProperty gp;
            gp.tolerance = tolerance(shape);
            gp.volume = v_props.Mass(); // will this be zero for non-solid shape?
            gp.area = s_props.Mass();
            gp.perimeter = l_props.Mass();
            gp.edgeCount = edgeCount;
            gp.faceCount = faceCount;
            gp.solidCount = solidCount;
            gp_Pnt center = v_props.CentreOfMass();
            gp.centerOfMass.push_back(center.X());
            gp.centerOfMass.push_back(center.Y());
            gp.centerOfMass.push_back(center.Z());
            return gp;
        }

        Standard_Real area(const TopoDS_Shape& s)
        {
            GProp_GProps s_props;
            BRepGProp::SurfaceProperties(s, s_props, true);
            return s_props.Mass();
        }

        Standard_Real perimeter(const TopoDS_Shape& s)
        {
            GProp_GProps l_props;
            BRepGProp::LinearProperties(s, l_props, true);
            return l_props.Mass();
        }

        Standard_Real volume(const TopoDS_Shape& s)
        {
            GProp_GProps v_props;
            BRepGProp::VolumeProperties(s, v_props); // onlyClose, skipShared
            return v_props.Mass();
        }

        Standard_Real tolerance(const TopoDS_Shape& s)
        {
            ShapeAnalysis_ShapeTolerance analysis;
            Standard_Integer mode = 0;                      // 0: average, > 0 max,  <0 min
            double tolerance = analysis.Tolerance(s, mode); // shapeType = Shape
            return tolerance;
        }

        /// half float has the max number about 65000,
        /// assuming geometry has mm as unit, the volume will overflow, should be scaled
        /// see the LENGTH_SCALE defined in UniqueId.h
        UniqueIdType uniqueId(const GeometryProperty& p)
        {
            return UniqueId::geometryUniqueId(p.volume, p.centerOfMass);
        }

        UniqueIdType uniqueId(const TopoDS_Shape& s)
        {
            GProp_GProps v_props;
            BRepGProp::VolumeProperties(s, v_props);

            UniqueIdType gid = UniqueId::geometryUniqueId(
                v_props.Mass(), {v_props.CentreOfMass().X(), v_props.CentreOfMass().Y(), v_props.CentreOfMass().Z()});
            return gid;
        }

        Standard_Real distance(const TopoDS_Shape& s1, const TopoDS_Shape& s2)
        {
            auto dss = BRepExtrema_DistShapeShape(); // GeomAPI_ExtremaSurfaceSurface
            dss.LoadS1(s1);
            dss.LoadS2(s2);
            dss.Perform();

            if (dss.IsDone())
            {
                // LOG_F(INFO, "nbSolution: %d, distance  = %lf", dss.NbSolution(), dss.Value());
                return dss.Value(); // return zero if there is contact and interference
                // see more example usage:
                // https://github.com/siconos/siconos/blob/master/mechanics/src/occ/OccUtils.cpp
            }
            else
            {
                LOG_F(ERROR, "distance() failed for error in BRepExtrema_DistShapeShape, return a big value");
                return 1e20; // invalid distance, would a very number safer
            }
        }

        int countDeletedShape(const std::shared_ptr<BRepAlgoAPI_BuilderAlgo>& mkGFA, TopAbs_ShapeEnum stype)
        {
            int mfc = 0;
            const auto& inputs = mkGFA->Arguments();
            for (const auto& s : inputs)
            {
                for (TopExp_Explorer anExp(s, stype); anExp.More(); anExp.Next())
                {
                    if (mkGFA->IsDeleted(anExp.Current()))
                        mfc++; // (myFillHistory && myHistory ? myHistory->IsRemoved(theS) : Standard_False);
                }
            }
            return mfc;
        }

        int countModifiedShape(const std::shared_ptr<BRepAlgoAPI_BuilderAlgo>& mkGFA, TopAbs_ShapeEnum stype)
        {
            int mfc = 0;
            const auto& inputs = mkGFA->Arguments();
            for (const auto& s : inputs)
            {
                for (TopExp_Explorer anExp(s, stype); anExp.More(); anExp.Next())
                {
                    auto listOfMod = mkGFA->Modified(anExp.Current()); // myHistory->Modified(theS);
                    if (listOfMod.Extent())
                        mfc++;
                }
            }
            return mfc;
        }

        void summarizeBuilderAlgo(const std::shared_ptr<BRepAlgoAPI_BuilderAlgo> mkGFA)
        {
            if (mkGFA->HasDeleted())
            {
                std::cout << "This builder has deleted shapes, " << countDeletedShape(mkGFA, TopAbs_EDGE) << std::endl;
            }
            if (mkGFA->HasGenerated())
            {
                std::cout << "This builder has generated shapes" << std::endl;
            }
            if (mkGFA->HasModified())
            {
                std::cout << "This builder has modified input shapes:" << std::endl;
                // const auto& inputs = mkGFA->Arguments();

                std::cout << "modified faces count = " << countModifiedShape(mkGFA, TopAbs_FACE) << std::endl;
                std::cout << "modified edges count = " << countModifiedShape(mkGFA, TopAbs_EDGE) << std::endl;
                std::cout << "modified vertex count = " << countModifiedShape(mkGFA, TopAbs_VERTEX) << std::endl;
                // mkGFA->GetReport(); but how to print?
                // TopTools_DataMapOfShapeListOfShape& mapOfMod =
            }
        }

        /// mkGFA reference itself is input and output parameter, can merge more than 2 solids
        std::vector<TopoDS_Shape> generalFuse(const std::vector<TopoDS_Shape>& shapes, const Standard_Real tolerance,
                                              std::shared_ptr<BRepAlgoAPI_BuilderAlgo> mkGFA)
        {
            if (not mkGFA)
            {
                mkGFA = std::make_shared<BRepAlgoAPI_BuilderAlgo>();
                mkGFA->SetRunParallel(false);
            }
            auto tol = tolerance; // set 0.0 to temporally disable fuzzy operation
            TopTools_ListOfShape GFAArguments;
            for (const TopoDS_Shape& it : shapes)
            {
                if (it.IsNull())
                    throw OSD_Exception("one shape is null");
                if (tol > 0.0)
                    // workaround for http://dev.opencascade.org/index.php?q=node/1056#comment-520
                    GFAArguments.Append(BRepBuilderAPI_Copy(it).Shape());
                else
                    GFAArguments.Append(it);
            }
            mkGFA->SetArguments(GFAArguments);
            if (tol > 0.0)
                mkGFA->SetFuzzyValue(tolerance);
#if OCC_VERSION_HEX >= 0x070000
            mkGFA->SetNonDestructive(Standard_True);
#endif
#if OCC_VERSION_HEX >= 0x070300
            mkGFA->SetUseOBB(true);
#endif
            mkGFA->Build();
            if (!mkGFA->IsDone())
                throw OSD_Exception("General Fusion failed");

            std::vector<TopoDS_Shape> res; // output is limited to Solid shape type
            for (TopExp_Explorer anExp(mkGFA->Shape(), TopAbs_SOLID); anExp.More(); anExp.Next())
            {
                res.push_back(anExp.Current());
            }
            return res;
        }

        TopoDS_Shape fuseShape(const VectorType<TopoDS_Shape> v, bool occInternalParallel)
        {
            assert(v.size() >= 2UL);

            TopoDS_Shape result;

            BRepAlgoAPI_Fuse bc{v[0], v[1]};
            bc.SetRunParallel(occInternalParallel);
            // bc.SetFuzzyValue(); tolerance?
            bc.SetNonDestructive(Standard_True);
            if (not bc.IsDone())
            {
                LOG_F(ERROR, "BRepAlgoAPI_Fuse (boolean union) is not done");
            }
            result = bc.Shape();
            for (size_t i = 2; i < v.size(); i++)
            {
                BRepAlgoAPI_Fuse bc{result, v[i]};
                bc.SetRunParallel(occInternalParallel);
                // bc.SetFuzzyValue(); tolerance?
                bc.SetNonDestructive(Standard_True);
                result = bc.Shape();
            }
            return result;
        }

        /// if error happen, the return shape may be invalid
        TopoDS_Shape commonShape(const VectorType<TopoDS_Shape> v, bool occInternalParallel)
        {
            assert(v.size() >= 2UL);

            TopoDS_Shape result;

            BRepAlgoAPI_Common bc{v[0], v[1]};
            bc.SetRunParallel(occInternalParallel);
            // bc.SetFuzzyValue(); tolerance?
            bc.SetNonDestructive(Standard_True);
            if (not bc.IsDone())
            {
                LOG_F(ERROR, "BRepAlgoAPI_Common is not done");
            }
            result = bc.Shape();
            for (size_t i = 2; i < v.size(); i++)
            {
                BRepAlgoAPI_Common bc{result, v[i]};
                bc.SetRunParallel(occInternalParallel);
                // bc.SetFuzzyValue(); tolerance?
                bc.SetNonDestructive(Standard_True);
                result = bc.Shape();
            }
            return result;
        }

        TopoDS_Shape cutShape(const TopoDS_Shape& from, const TopoDS_Shape& substractor)
        {
            BRepAlgoAPI_Cut bc{from, substractor};
            return bc.Shape();
        }

        /// later, add MeshParameter data class, mappable to IMeshTools_Parameters(OCCT 7.4)
        std::shared_ptr<BRepMesh_IncrementalMesh> meshShape(const TopoDS_Shape& aShape, double resolution)
        {
#if OCC_VERSION_HEX >= 0x070400
            IMeshTools_Parameters aMeshParams;
            aMeshParams.Deflection = resolution;
            aMeshParams.Angle = 0.5;
            aMeshParams.Relative = Standard_False;
            aMeshParams.InParallel = Standard_True;
            aMeshParams.MinSize = Precision::Confusion();
            aMeshParams.InternalVerticesMode = Standard_True;
            aMeshParams.ControlSurfaceDeflection = Standard_True;
            auto aMesher = std::make_shared<BRepMesh_IncrementalMesh>(aShape, aMeshParams);
#elif OCC_VERSION_HEX >= 0x070000 // not sure occ 7.0 has BRepMesh_IncrementalMesh class
            // IMeshTools_Parameters is available for occt 7.4
            const Standard_Real aLinearDeflection = resolution;
            const Standard_Real anAngularDeflection = 0.5;
            const Standard_Boolean isRelative = Standard_False;   // ??
            const Standard_Boolean isInParallel = Standard_False; // turn off the internal parallel
            const Standard_Boolean adaptiveMin = Standard_False;
            auto aMesher = std::make_shared<BRepMesh_IncrementalMesh>(aShape, aLinearDeflection, isRelative,
                                                                      anAngularDeflection, isInParallel, adaptiveMin);
#else
#error "older version of OCC 6.x is not implemented for this function"
            // see makeSTL() of https://gitlab.onelab.info/gmsh/gmsh/blob/master/Geo/GModelIO_OCC.cpp
#endif
            const Standard_Integer aStatus = aMesher->GetStatusFlags();
            if (aStatus)
            {
                // LOG_F(ERROR, "failed to mesh the shape, return the shared_ptr to a false value");
                aMesher.reset(); // set to nullptr
            }
            return aMesher;
        }

        /// stl may not work for interior mesh
        Standard_Boolean saveMesh(TopoDS_Shape& aShape, const std::string file_name)
        {

            if (file_name.find("stl") != file_name.npos)
            {
#if OCC_VERSION_HEX >= 0x070300
                // RWStl::WriteFile(file_name.c_str(), aTriangulation);
                // low level API used by StlAPI_Writer
                // in lower version, each triangle is read as TopoDS_Face which is not efficient
                auto m = meshShape(aShape);
                auto stl_exporter = StlAPI_Writer(); // high level API
                // only ASCII mode in occt 7.3
                /* by default ascii write mode, occ 7.4 support binary STL file write
                if(fileMode == "ascii")
                    stl_exporter.SetASCIIMode(true);
                else // binary, just set the ASCII flag to False
                    stl_exporter.SetASCIIMode(false);
                */
                stl_exporter.Write(aShape, file_name.c_str()); // shape must has mesh for each face
                return true;
#else
                return false;
#endif
            }

            else
            {
                LOG_F(ERROR, "output mesh file suffix is not supported: %s", file_name.c_str());
                return false;
            }
        }

        TopoDS_Shape scaleShape(const TopoDS_Shape& from, double scale, const gp_Pnt origin)
        {
            gp_Trsf ts;
            ts.SetScale(origin, scale);
            return BRepBuilderAPI_Transform(from, ts, true);
        }

    } // namespace OccUtils
} // namespace Geom
