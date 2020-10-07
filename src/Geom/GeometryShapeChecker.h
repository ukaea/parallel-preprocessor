/***************************************************************************
 *   Copyright (c) 2019 Qingfeng Xia  <qingfeng.xia@ukaea.uk>              *
 *   This file is part of parallel-preprocessor                            *
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

/// This file is adapted from FreeCAD project

#ifndef PPP_SHAPE_CHECKER_H
#define PPP_SHAPE_CHECKER_H

#include "GeometryProcessor.h"
#include "OccUtils.h"


namespace Geom
{
    using namespace PPP;
    /// \ingroup Geom
    /**
     * check error and try to fix it within tolerance, otherwise report error
     * FreeCAD Part workbench, ShapeCheck, BOP check
     * todo: ShapeHealing toolkit of OCCT has a ShapeAnalysis package
     */
    class GeometryShapeChecker : public GeometryProcessor
    {
        TYPESYSTEM_HEADER();

    protected:
        bool checkingBooleanOperation = false;
        // need a bool parameter to control:
        bool suppressBOPCheckFailed = false;

    private:
        // VectorType<std::shared_ptr<std::string>> myShapeCheckResults;

    public:
        // using GeometryProcessor::GeometryProcessor;

        virtual void prepareInput() override final
        {
            checkingBooleanOperation = parameterValue<bool>("checkingBooleanOperation", false);
            suppressBOPCheckFailed = parameterValue<bool>("suppressBOPCheckFailed", false);
            GeometryProcessor::prepareInput();
        }

        virtual void processItem(const ItemIndexType i) override final
        {
            const TopoDS_Shape& aShape = item(i);
            std::string err = checkShape(aShape);
            writeItemReport(i, err);
            if (err.size() == 0 &&
                checkingBooleanOperation) // Two common not serious errors are skipped in this BOP check
            {
                bool hasFault = false;
                try
                {
                    hasFault = BOPSingleCheck(aShape); // will not report error message
                }
                catch (const std::exception& e)
                {
                    hasFault = true;
                    LOG_F(ERROR, "BOP check has exception %s for item %lu ", e.what(), i);
                }
                catch (...)
                {
                    hasFault = true;
                    LOG_F(ERROR, "BOP check has exception for item %lu ", i);
                }

                if (hasFault)
                {
                    auto df = generateDumpName("dump_BOPCheckFailed", {i}) + ".brep";
                    OccUtils::saveShape({item(i)}, df);
                    if (suppressBOPCheckFailed)
                    {
                        LOG_F(ERROR, "BOP check found fault for item %lu, so suppress this item", i);
                        suppressItem(i);
                    }
                    else
                    {
                        LOG_F(ERROR, "BOP check found fault for item %lu, ignore", i);
                    }
                }
            }
        }

        /// report, save and display erroneous shape
        virtual void prepareOutput() override final
        {
            GeometryProcessor::prepareOutput(); // report()
        }

    protected:
        /** basic check, geometry reader may has already done this check
         * adapted from FreeCAD project:  BOPcheck
         * */
        std::string checkShape(const TopoDS_Shape& _cTopoShape) const
        {
            std::stringstream error_msg(std::ios_base::ate);
            BRepCheck_Analyzer aChecker(_cTopoShape);
            if (!aChecker.IsValid())
            {
                TopoDS_Iterator it(_cTopoShape);
                for (; it.More(); it.Next())
                {
                    if (!aChecker.IsValid(it.Value()))
                    {
                        const Handle(BRepCheck_Result)& result = aChecker.Result(it.Value());
                        const BRepCheck_ListOfStatus& status = result->StatusOnShape(it.Value());

                        BRepCheck_ListIteratorOfListOfStatus it(status);
                        while (it.More())
                        {
                            BRepCheck_Status& val = it.Value();
                            switch (val)
                            {
                            case BRepCheck_NoError:
                                // error_msg << ";No error";  // exmty string as a sign of no error
                                break;
                            case BRepCheck_InvalidPointOnCurve:
                                error_msg << ";Invalid point on curve";
                                break;
                            case BRepCheck_InvalidPointOnCurveOnSurface:
                                error_msg << ";Invalid point on curve on surface";
                                break;
                            case BRepCheck_InvalidPointOnSurface:
                                error_msg << ";Invalid point on surface";
                                break;
                            case BRepCheck_No3DCurve:
                                error_msg << ";No 3D curve";
                                break;
                            case BRepCheck_Multiple3DCurve:
                                error_msg << ";Multiple 3D curve";
                                break;
                            case BRepCheck_Invalid3DCurve:
                                error_msg << ";Invalid 3D curve";
                                break;
                            case BRepCheck_NoCurveOnSurface:
                                error_msg << ";No curve on surface";
                                break;
                            case BRepCheck_InvalidCurveOnSurface:
                                error_msg << ";Invalid curve on surface";
                                break;
                            case BRepCheck_InvalidCurveOnClosedSurface:
                                error_msg << ";Invalid curve on closed surface";
                                break;
                            case BRepCheck_InvalidSameRangeFlag:
                                error_msg << ";Invalid same-range flag";
                                break;
                            case BRepCheck_InvalidSameParameterFlag:
                                error_msg << ";Invalid same-parameter flag";
                                break;
                            case BRepCheck_InvalidDegeneratedFlag:
                                error_msg << ";Invalid degenerated flag";
                                break;
                            case BRepCheck_FreeEdge:
                                error_msg << ";Free edge";
                                break;
                            case BRepCheck_InvalidMultiConnexity:
                                error_msg << ";Invalid multi-connexity";
                                break;
                            case BRepCheck_InvalidRange:
                                error_msg << ";Invalid range";
                                break;
                            case BRepCheck_EmptyWire:
                                error_msg << ";Empty wire";
                                break;
                            case BRepCheck_RedundantEdge:
                                error_msg << ";Redundant edge";
                                break;
                            case BRepCheck_SelfIntersectingWire:
                                error_msg << ";Self-intersecting wire";
                                break;
                            case BRepCheck_NoSurface:
                                error_msg << ";No surface";
                                break;
                            case BRepCheck_InvalidWire:
                                error_msg << ";Invalid wires";
                                break;
                            case BRepCheck_RedundantWire:
                                error_msg << ";Redundant wires";
                                break;
                            case BRepCheck_IntersectingWires:
                                error_msg << ";Intersecting wires";
                                break;
                            case BRepCheck_InvalidImbricationOfWires:
                                error_msg << ";Invalid imbrication of wires";
                                break;
                            case BRepCheck_InvalidImbricationOfShells:
                                error_msg << ";BRepCheck_InvalidImbricationOfShells";
                                break;
                            case BRepCheck_EmptyShell:
                                error_msg << ";Empty shell";
                                break;
                            case BRepCheck_RedundantFace:
                                error_msg << ";Redundant face";
                                break;
                            case BRepCheck_UnorientableShape:
                                error_msg << ";Unorientable shape";
                                break;
                            case BRepCheck_NotClosed:
                                error_msg << ";Not closed";
                                break;
                            case BRepCheck_NotConnected:
                                error_msg << ";Not connected";
                                break;
                            case BRepCheck_SubshapeNotInShape:
                                error_msg << ";Subshape not in shape";
                                break;
                            case BRepCheck_BadOrientation:
                                error_msg << ";Bad orientation";
                                break;
                            case BRepCheck_BadOrientationOfSubshape:
                                error_msg << ";Bad orientation of subshape";
                                break;
                            case BRepCheck_InvalidToleranceValue:
                                error_msg << ";Invalid tolerance value";
                                break;
                            case BRepCheck_EnclosedRegion:
                                error_msg << ";Enclosed region";
                                break;
                            case BRepCheck_InvalidPolygonOnTriangulation:
                                error_msg << ";Invalid polygon on triangulation";
                                break;
                            case BRepCheck_CheckFail:
                                error_msg << ";Check failed";
                                break;
                            }
                            it.Next();
                        }
                    }
                }
            }
            return error_msg.str();
        }


        /// check single TopoDS_Shape, adapted from FreeCAD and some other online forum
        // two common, notorious errors should be turn off curveOnSurfaceMode, continuityMode
        // didn't use BRepAlgoAPI_Check because it calls BRepCheck_Analyzer itself and
        // it doesn't give us access to it. so I didn't want to run BRepCheck_Analyzer twice to get invalid results.
        // BOPAlgo_ArgumentAnalyzer can check 2 objects with respect to a boolean op.
        // this is left for another time.
        bool BOPSingleCheck(const TopoDS_Shape& shapeIn)
        {
            bool runSingleThreaded = true; //
            // bool logErrors = true;
            bool argumentTypeMode = true;
            bool selfInterMode = true; // self interference, for single solid should be no such error
            bool smallEdgeMode = true;
            bool rebuildFaceMode = true;
            bool continuityMode = false; // BOPAlgo_GeomAbs_C0
            bool tangentMode = true;     // not implemented in OpenCASCADE
            bool mergeVertexMode = true;
            bool mergeEdgeMode = true;
            bool curveOnSurfaceMode = false; // BOPAlgo_InvalidCurveOnSurface: tolerance compatability check

            // I don't why we need to make a copy, but it doesn't work without it.
            // BRepAlgoAPI_Check also makes a copy of the shape.
            TopoDS_Shape BOPCopy = BRepBuilderAPI_Copy(shapeIn).Shape();
            BOPAlgo_ArgumentAnalyzer BOPCheck;

            //   BOPCheck.StopOnFirstFaulty() = true; //this doesn't run any faster but gives us less results.
            BOPCheck.SetShape1(BOPCopy);
            // all settings are false by default. so only turn on what we want.
            BOPCheck.ArgumentTypeMode() = argumentTypeMode;
            BOPCheck.SelfInterMode() = selfInterMode;
            BOPCheck.SmallEdgeMode() = smallEdgeMode;
            BOPCheck.RebuildFaceMode() = rebuildFaceMode;
#if OCC_VERSION_HEX >= 0x060700
            BOPCheck.ContinuityMode() = continuityMode;
#endif
#if OCC_VERSION_HEX >= 0x060900
            BOPCheck.SetParallelMode(!runSingleThreaded); // this doesn't help for speed right now(occt 6.9.1).
            BOPCheck.SetRunParallel(!runSingleThreaded);  // performance boost, use all available cores
            BOPCheck.TangentMode() = tangentMode;         // these 4 new tests add about 5% processing time.
            BOPCheck.MergeVertexMode() = mergeVertexMode;
            BOPCheck.MergeEdgeMode() = mergeEdgeMode;
            BOPCheck.CurveOnSurfaceMode() = curveOnSurfaceMode;
#endif

            BOPCheck.Perform();
            if (!BOPCheck.HasFaulty())
                return false;
            return true;
        }
    }; // end of class

} // namespace Geom

#endif