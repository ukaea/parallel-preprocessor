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
     * check shape error and report error
     * FreeCAD Part workbench, has a GUI feature for ShapeCheck with tree view
     * todo: ShapeHealing toolkit of OCCT has a ShapeAnalysis package
     * The base `class Processor` now has report infrastructure, save msg into `myItemReports`
     */
    class GeometryShapeChecker : public GeometryProcessor
    {
        TYPESYSTEM_HEADER();

    protected:
        /// configurable parameters
        bool checkingBooleanOperation = false;
        bool suppressBOPCheckFailed = false;

    private:
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
            if (itemSuppressed(i))
                return;
            const TopoDS_Shape& aShape = item(i);
            std::string err = checkShape(aShape);
            auto ssp = std::make_shared<std::stringstream>();
            if (err.size() > 0)
                *ssp << err;

            // if basic checkShape has error, then BOPCheck must have fault?
            if (                          //  err.size() == 0 &&   skip BOP check if preliminary check has failed?
                checkingBooleanOperation) // Two common not serious errors are skipped in this BOP check
            {
                bool hasBOPFault = false;
                try
                {
                    hasBOPFault = BOPSingleCheck(aShape, *ssp);
                }
                catch (const std::exception& e)
                {
                    hasBOPFault = true;
                    LOG_F(ERROR, "BOP check has exception %s for item %lu ", e.what(), i);
                }
                catch (...)
                {
                    hasBOPFault = true;
                    LOG_F(ERROR, "BOP check has exception for item %lu ", i);
                }

                if (ssp->str().size() > 0)
                    setItemReport(i, ssp);
                if (hasBOPFault)
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

        /// `class Processor::report()`, save and display erroneous shape
        /// if this processor 's config has file path in the "report" parameter
        virtual void prepareOutput() override final
        {
            GeometryProcessor::prepareOutput();
        }

    protected:
        /** basic check, geometry reader may has already done some of the checks below
         * adapted from FreeCAD project:
         * https://github.com/FreeCAD/FreeCAD/blob/master/src/Mod/Part/Gui/TaskCheckGeometry.cpp
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
                                // error_msg << ";No error";  // empty string as a sign of no error
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

        /// see impl in BOPAlgo_ArgumentAnalyzer.cpp
        std::array<const char*, 12> BOPAlgo_StatusNames = {
            "CheckUnknown",            //
            "BadType",                 // either input shape IsNull()
            "SelfIntersect",           // self intersection, BOPAlgo_CheckerSI
            "TooSmallEdge",            // only for BOPAlgo_SECTION
            "NonRecoverableFace",      // TestRebuildFace()
            "IncompatibilityOfVertex", /// TestMergeSubShapes()
            "IncompatibilityOfEdge",
            "IncompatibilityOfFace",
            "OperationAborted",      // when error happens in TestSelfInterferences()
            "GeomAbs_C0",            // continuity
            "InvalidCurveOnSurface", //
            "NotValid"               //
        };

        const char* getBOPCheckStatusName(const BOPAlgo_CheckStatus& status)
        {
            size_t index = static_cast<size_t>(status);
            assert(index >= 0 || index < BOPAlgo_StatusNames.size());
            return BOPAlgo_StatusNames[index];
        }

        /** use `BOPAlgo_ArgumentAnalyzer` to check 2 shapes with respect to a boolean operation.
            two common errors are `curveOnSurfaceMode`, `continuityMode`
            it is not clear which one cause BoP failure: `selfInterference`, `curveOnSurfaceMode`
            Note: do NOT use BRepAlgoAPI_Check ( BRepCheck_Analyzer )
        */
        bool BOPSingleCheck(const TopoDS_Shape& shapeIn, std::stringstream& ss)
        {
            bool runSingleThreaded = true; // parallel is done on solid shape level for PPP, not on subshape level

            bool argumentTypeMode = true;
            bool selfInterMode = true; // self interference, for single solid should be no such error
            bool smallEdgeMode = true; // only needed for de-feature?

            bool continuityMode = false; // BOPAlgo_GeomAbs_C0, it should not cause BOP failure?
            bool tangentMode = false;    // not implemented in OpenCASCADE

            bool rebuildFaceMode = true;
            bool mergeVertexMode = true; // leader to `BOPAlgo_IncompatibilityOfEdge`
            bool mergeEdgeMode = true;
            bool curveOnSurfaceMode = false; // BOPAlgo_InvalidCurveOnSurface: tolerance compatability check

            // FreeCAD develper's note: I don't why we need to make a copy, but it doesn't work without it.
            /// maybe, mergeVertexMode() will modify the shape to check
            TopoDS_Shape BOPCopy = BRepBuilderAPI_Copy(shapeIn).Shape();
            BOPAlgo_ArgumentAnalyzer BOPCheck;

            // BOPCheck.StopOnFirstFaulty() = true; //this doesn't run any faster but gives us less results.
            // BOPCheck.SetFuzzyValue();
            BOPCheck.SetShape1(BOPCopy);
            // BOPCheck.SetOperation();  // by default, set to BOPAlgo_UNKNOWN
            // all settings are false by default. so only turn on what we want.
            BOPCheck.ArgumentTypeMode() = argumentTypeMode;
            BOPCheck.SelfInterMode() = selfInterMode;
            BOPCheck.SmallEdgeMode() = smallEdgeMode;
            BOPCheck.RebuildFaceMode() = rebuildFaceMode;
#if OCC_VERSION_HEX >= 0x060700
            BOPCheck.ContinuityMode() = continuityMode;
#endif
#if OCC_VERSION_HEX >= 0x060900
            BOPCheck.SetParallelMode(!runSingleThreaded);
            BOPCheck.SetRunParallel(!runSingleThreaded);

            BOPCheck.TangentMode() = tangentMode;         // these 4 new tests add about 5% processing time.
            BOPCheck.MergeVertexMode() = mergeVertexMode; // will it modify the shape to check?
            BOPCheck.MergeEdgeMode() = mergeEdgeMode;
            BOPCheck.CurveOnSurfaceMode() = curveOnSurfaceMode;
#endif

            BOPCheck.Perform(); // this perform() has internal try-catch block
            if (BOPCheck.HasFaulty())
            {
                const BOPAlgo_ListOfCheckResult& BOPResults = BOPCheck.GetCheckResult();
                BOPAlgo_ListIteratorOfListOfCheckResult BOPResultsIt(BOPResults);
                for (; BOPResultsIt.More(); BOPResultsIt.Next())
                {
                    const BOPAlgo_CheckResult& current = BOPResultsIt.Value();
                    if (current.GetCheckStatus() != BOPAlgo_CheckUnknown)
                        ss << ";" << getBOPCheckStatusName(current.GetCheckStatus());
                }
                return true; // has failure
            }
            return false; // no fault
        }

    }; // end of class

} // namespace Geom

#endif