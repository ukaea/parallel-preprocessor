#include "CollisionDetector.h"

namespace Geom
{
    bool CollisionDetector::hasCollision(const TopoDS_Shape& shape1, const TopoDS_Shape& shape2, double theTolerance)
    {
        std::vector<Standard_Real> vols;
        CollisionInfo cinfo = detectCollision(shape1, shape2, vols, theTolerance);
        return cinfo.type > CollisionType::Clearance;
    }

    /// this is the second step to calcCollisionType(), after detectCollision
    CollisionType CollisionDetector::calcCollisionType(std::shared_ptr<BRepAlgoAPI_BuilderAlgo> mkGFA,
                                                       std::vector<Standard_Real> origVols,
                                                       std::vector<ItemIndexType> ij)
    {
        using namespace OccUtils;
        size_t originalShapeCount = mkGFA->Arguments().Size();
        const auto& origShapes = mkGFA->Arguments();
        std::vector<TopoDS_Shape> resultShapes; // output is limited to Solid shape type
        for (TopExp_Explorer anExp(mkGFA->Shape(), TopAbs_SOLID); anExp.More(); anExp.Next())
        {
            resultShapes.push_back(anExp.Current()); // what kind of copy,  const ref of the shape is given
        }
        if (originalShapeCount != 2)
        {
            throw std::runtime_error("this collision type detection method only works for 2 solids");
        }

        // resultShapes.size() > 0, otherwise there must be some error during general fusion
        if (resultShapes.size() == 1)
        {
            return CollisionType::Coincidence;
        }
        else if (resultShapes.size() >= originalShapeCount)
        {
            // size_t modifiedFaceCount = 0;
            // size_t modifiedEdgeCount = 0;
            if (mkGFA->HasModified()) // has modified solids after general fuse
            {
                if (origVols.size() == 0) // empty volume vector, default parameter value
                {
                    origVols.clear();
                    for (const auto& s : origShapes)
                    {
                        GProp_GProps v_props;
                        BRepGProp::VolumeProperties(s, v_props);
                        auto v = v_props.Mass();
                        origVols.push_back(v);
                    }
                }
                if (origVols.size() != originalShapeCount)
                {
                    throw std::runtime_error("volume before bool must be provided by the caller or leave it empty");
                }
                Standard_Real sumOrigVols = std::accumulate(origVols.cbegin(), origVols.cend(), 0.0);

                std::vector<Standard_Real> resVols; // result shape volumes
                Standard_Real allVol = 0;
                for (const auto& s : resultShapes)
                {
                    GProp_GProps v_props;
                    BRepGProp::VolumeProperties(s, v_props);
                    auto v = v_props.Mass();
                    resVols.push_back(v);
                    allVol += v;
                }

                auto min_result_volume = *std::min_element(resVols.cbegin(), resVols.cend());
                if (min_result_volume < 0)
                {
                    VLOG_F(LOGLEVEL_DEBUG, "result solids has minus volume, it is an error");
                    return CollisionType::Error;
                }
                auto max_original_volume = *std::max_element(origVols.cbegin(), origVols.cend());
                // auto min_original_volume = *std::min_element(origVols.cbegin(), origVols.cend());

                if (resultShapes.size() > originalShapeCount)
                {
                    /// interference Volume is the difference between original total and result total
                    /// if the ratio of interference Volume/total is too smaller, it could be a fake interference, try
                    /// bigger tolerance if ratio is between contact_vol_reltol and interference_vol_reltol, it is weak
                    /// interference. Consider: make this as a parameter and controlled in json file
                    auto contact_vol_reltol = 0.01;
                    auto interference_vol_reltol = 0.1; // above that
                    if (floatEqual(allVol, sumOrigVols, contact_vol_reltol))
                    {
                        VLOG_F(LOGLEVEL_DEBUG, "interference volume is small, deemed as face contact by auto fixing");
                        return CollisionType::WeakInterference; // merge the smaller with the interference
                    }

                    else if (floatEqual(allVol, sumOrigVols, interference_vol_reltol))
                    {
                        auto ratio = std::abs((allVol - sumOrigVols) / sumOrigVols);
                        LOG_F(WARNING, "interference ratio is %f, but set as WeakInterference to be fixed", ratio);
                        return CollisionType::WeakInterference; // merge the smaller with the interference
                    }
                    else
                        return CollisionType::Interference;
                }
                else /// i.e. resultShapes.size() == originalShapeCount
                {
                    // only surface contact is counted as Contact
                    if (floatEqual(allVol, sumOrigVols, 1e-3))
                    {
                        /* count total face, edge and vertex may also not reliable */
                        Standard_Real totalArea = 0.0;
                        for (const auto& s : origShapes)
                            totalArea += area(s);
                        Standard_Real resultArea = area(mkGFA->Shape());
                        if (floatEqual(totalArea, resultArea)) // not face contact
                        {
                            Standard_Real totalPerimeter = 0.0;
                            for (const auto& s : origShapes)
                                totalPerimeter += perimeter(s);
                            Standard_Real resultPerimeter = perimeter(mkGFA->Shape());
                            if (floatEqual(totalPerimeter, resultPerimeter))
                                return CollisionType::VertexContact;
                            else
                                return CollisionType::EdgeContact;
                        }
                        else
                        {
                            return CollisionType::FaceContact;
                        }
                    }
                    /// if result total value floatEqual to the volume of the bigger input shape, it is enclosure
                    /// this judgment also applies when input shape count > 2
                    else if (floatEqual(allVol, max_original_volume, 1e-5))
                    {
                        return CollisionType::Enclosure;
                    }
                    /// result solids count == originalShapeCount, but there is volume change
                    /// why? because TopoDS_Shape may fail in BOP check, e.g. self-interference
                    /// then 2 parts not interfered may show change volumes,
                    /// this problem is due to bad quality of input shapes.
                    else
                    {
                        if (ij.size() == 2)
                        {
                            LOG_F(WARNING, "Collision type unknown for volume change for #%lu , %lu, try BOP check",
                                  ij[0], ij[1]);
                        }
                        VLOG_F(LOGLEVEL_DEBUG, "original total vol = %f, fragments total vol = %f \n", sumOrigVols,
                               allVol);
                        VLOG_F(LOGLEVEL_DEBUG, "solids original vols %f, %f\n", origVols[0], origVols[1]);
                        VLOG_F(LOGLEVEL_DEBUG, "fragments count %lu vols %f, %f\n", resVols.size(), resVols[0],
                               resVols[1]);

                        return CollisionType::Unknown;
                    }
                }
            }
            else // no modified solid in general fuse
            {
                return CollisionType::NoCollision;
            }
        }
        else // solid count = 0
        {
            LOG_F(ERROR, "Error in general fuse, resulted solid number is zero");
            return CollisionType::Error;
        }
    }

    /// by general fusion Fragments, used by hasCollision()  mainly for unit test
    CollisionInfo CollisionDetector::detectCollision(const TopoDS_Shape& s, const TopoDS_Shape& s2,
                                                     std::vector<Standard_Real> vols, double theTolerance)
    {
        using namespace OccUtils;
        CollisionType ctype = CollisionType::Error;
        if (s == s2) // identical obj in memory, inputs are references
        {
            ctype = CollisionType::Coincidence;
            CollisionInfo cinfo = {0, 0, 0.0, ctype}; // index will be set outside by caller
            return cinfo;
        }

        if (vols.size() == 0)
        {
            GProp_GProps v_props;
            BRepGProp::VolumeProperties(s, v_props);
            vols.push_back(v_props.Mass());
            BRepGProp::VolumeProperties(s2, v_props);
            vols.push_back(v_props.Mass());
        }
        const std::vector<TopoDS_Shape> shapes = {s, s2};
        auto mkGFA = std::make_shared<BRepAlgoAPI_BuilderAlgo>();
        mkGFA->SetNonDestructive(true);
        mkGFA->SetFuzzyValue(theTolerance);
        // SetGlue() can accelerate in case of overlapping, but for the case of no interference
        // mkGFA->SetGlue(BOPAlgo_GlueShift);
        mkGFA->SetRunParallel(false);
        mkGFA->SetUseOBB(true);
        std::exception_ptr eptr;
        try
        {
            generalFuse(shapes, theTolerance, mkGFA);
        }
        catch (...)
        {
            eptr = std::current_exception();
        }
        ctype = calcCollisionType(mkGFA, vols);
        CollisionInfo cinfo = {0, 0, mkGFA->FuzzyValue(), ctype}; // index will be set outside by caller
        return cinfo;
    }

    /// call this function in serial mode, let PPP run schedule threads on item level
    bool CollisionDetector::detectCollision(const ItemIndexType i, const ItemIndexType j, bool internalMultiThreading,
                                            bool imprinting)
    {
        CollisionType ctype = CollisionType::NoCollision;

        bool completed = false;
        std::vector<Standard_Real> vols = {(*myGeometryProperties)[i].volume, (*myGeometryProperties)[j].volume};
        std::vector<TopoDS_Shape> twoShapes = {item(i), item(j)};

        std::exception_ptr eptr;
        // bool hasException = false;
        std::vector<TopoDS_Shape> pieces;
        // retry by giving bigger tolerance, so tiny interference can be ignored
        std::shared_ptr<BRepAlgoAPI_BuilderAlgo> pFuser;

        // it is tricky to whether try fuzzy tol first of zero tol
        // mastu test data is working either way.
        // PCarmon's test data has exception if try zero tolerance first
        std::vector<double> tolerances = {tolerance, 0.0};
        const size_t NB_RETRY = tolerances.size();

        // try different tolerance to correct CollisionType::Error
        for (size_t r = 0; r < NB_RETRY && !completed; r++)
        {
            try
            {
                auto mkGFA = std::make_shared<BRepAlgoAPI_BuilderAlgo>();
                mkGFA->SetNonDestructive(true);
                // mkGFA->SetGlue(BOPAlgo_GlueShift); // quick in case of overlapping only, no interference
                mkGFA->SetRunParallel(not internalMultiThreading);
                // mkGFA->SetUseOBB(true);  // only if OCCT version is high enough
                pieces = OccUtils::generalFuse(twoShapes, tolerances[r], mkGFA);

                pFuser = mkGFA;
                completed = (pieces.size() == 2);
                // no need to try again, is that possible to break inside try block?
            }
            catch (...)
            {
                eptr = std::current_exception();
                std::vector<ItemIndexType> itemIndices = {i, j};
                // dealGeneralFuseException(twoShapes, itemIndices);
                dealGeneralFuseException(pieces, itemIndices);
                LOG_F(ERROR, "Error during merge and collision type calc for item #%lu `%s`, #%lu `%s`", i,
                      itemName(i).c_str(), j, itemName(j).c_str());
                ctype = CollisionType::Error; // dump will be done at the end of this function
                // should not break, loop to see if bigger tolerance solves the GFA exception?
            }

            if (pFuser) // this is not valid if exception happened
            {
                if (pFuser->HasErrors() and r == NB_RETRY - 1)
                {
                    // todo: mkGFA->HasError(handle_type) to find out the specific error type
                    LOG_F(ERROR, "general fuse has error for the item pair #%lu `%s`, #%lu `%s`, dump", i,
                          itemName(i).c_str(), j, itemName(j).c_str());
                    auto df_original = generateDumpName("dump_fusionError", {i, j}) + myDumpFileType;
                    OccUtils::saveShape({item(i), item(j)}, df_original);
                    if (pieces.size())
                    {
                        auto df_result = generateDumpName("dump_fusionErrorResult", {i, j}) + myDumpFileType;
                        OccUtils::saveShape(pieces, df_result);
                    }
                    ctype = CollisionType::Error;
                }
                else
                {
                    bool isWeakInterferenceFixed = false;
                    ctype = calcCollisionType(pFuser, vols, {i, j});
                    // if there is volume error, then try if they are not in contact to fix the error
                    // however, it seems always give zero distance, does not help
                    if (ctype >= CollisionType::Error and r == NB_RETRY - 1)
                    {
                        auto ctype_ret = solveErrorByDistanceCheck(i, j);
                        if (ctype_ret <= CollisionType::Clearance)
                            ctype = ctype_ret;
                    }
                    if (ctype == CollisionType::WeakInterference)
                    {
                        auto results = solveWeakInterference(pieces, {}, twoShapes, vols); // modified pieces
                        if (results.size() == 2)
                        {
                            pieces = results; // boundbox, volume and uniqueId has changed after fixing
                            ctype = CollisionType::FaceContact;
                            isWeakInterferenceFixed = true;
                            // todo: save to string and report at the end of processing
                            LOG_F(INFO, "weak interference fixed as face contact for item #%lu `%s`, #%lu `%s`", i,
                                  itemName(i).c_str(), j, itemName(j).c_str());
                        }
                    }
                    // must be done after weak interference check and fixing as face contact
                    if (ctype == CollisionType::FaceContact and pieces.size() == 2)
                    {
                        if (not isWeakInterferenceFixed)
                        {
                            // if weak interference is fixed, then both boundbox and volume will change.
                            // so the boundbox change check should be disable or use a very big reltol of 0.3
                            // sequence of input and output shape has been kept, but boundbox may change
                            if (not OccUtils::isBndBoxCoincident(pieces[0], (*myShapeBoundBoxes)[i], 0.3))
                            {
                                VLOG_F(LOGLEVEL_DEBUG, "changed BoundBox for #%lu `%s` when fuse with #%lu", i,
                                       itemName(i).c_str(), j);
                                VLOG_F(LOGLEVEL_DEBUG, "face contact detected, write back item #%lu  #%lu,", i, j);
                                // volume equalness has been validated in calcCollisionType(), should be equal

                                dumpChangedBoundbox(i, pieces[0]);
                            }
                            if (not OccUtils::isBndBoxCoincident(pieces[1], (*myShapeBoundBoxes)[j], 0.3))
                            {
                                VLOG_F(LOGLEVEL_DEBUG, "changed BoundBox for #%lu `%s` when fuse with #%lu", j,
                                       itemName(j).c_str(), i);
                                // dumpChangedBoundbox(j, pieces[1]);  // it must has been dump by other operation
                            }
                        }

                        if (imprinting) // code for Imprinter only
                        {
                            setItem(i, pieces[0]); // should also update boundbox and recalc volume
                            setItem(j, pieces[1]);
                        }
                        myAdjacencyMatrix.insertAt(i, j, true);
                        myAdjacencyMatrix.insertAt(j, i, true);
                    }
                    else if (ctype == CollisionType::NoCollision)
                    {
                        calcClearance(i, j); // also write myCollisionInfos
                    }
                    else
                    {
                        // do nothing
                    }
                }
            }
            if (ctype < CollisionType::Error) // does not need to retry
                break;
        } // end of retry

        // here CollisionType::FaceContact must be saved to help resolve collision!
        if (ctype >= CollisionType::FaceContact) // vertex or edge contact is not reported
        {
            CollisionInfo info = {i, j, tolerance, ctype};
            myCollisionInfos[i].push_back(std::make_pair(j, info));
            CollisionInfo info_j = {j, i, tolerance, ctype};
            myCollisionInfos[j].push_back(std::make_pair(i, info_j));
            // report collision will be done in postProcess()
        }
        else if (ctype >= CollisionType::Error)
        {
            auto df_original = generateDumpName("dump_ctypeErrorOriginal", {i, j}) + myDumpFileType;
            OccUtils::saveShape({item(i), item(j)}, df_original);
        }
        // VLOG_F(LOGLEVEL_DEBUG, "contact type is %i solid pair #%lu `%s`, #%lu `%s`", ctype, i,
        // itemName(i).c_str(), j, itemName(j).c_str()); // disable this debug, using myCollisionInfos.json
        return completed && ctype == CollisionType::FaceContact;
    }

    bool CollisionDetector::detectBoundBoxOverlapping(const ItemIndexType i, const ItemIndexType j, double clearance)
    {
        bool overlapping = false;
        if (myShapeOrientedBoundBoxes) // todo: add clearance support for OBB
        {
            const Bnd_OBB& thisObb = (*myShapeOrientedBoundBoxes)[i];
            const Bnd_OBB& otherObb = (*myShapeOrientedBoundBoxes)[j];
            overlapping = OccUtils::isBndBoxOverlapped(thisObb, otherObb);
        }
        else // if OBB is not supported, OCCT <7.3
        {
            overlapping = OccUtils::isBndBoxOverlapped((*myShapeBoundBoxes)[i], (*myShapeBoundBoxes)[j], clearance);
        }
        return overlapping;
    }

    // call this method only if they has no interference
    CollisionType CollisionDetector::calcClearance(const ItemIndexType i, const ItemIndexType j)
    {
        double dist = 1e10; // a very big number
        try
        {
            dist = OccUtils::distance(item(i), item(j));
        }
        catch (...)
        {
            LOG_F(ERROR,
                  "exception happened during distance calculation between item %lu and %lu, "
                  "\n regarding as CollisionType::Error",
                  i, j);
            return CollisionType::Error;
        }

        if (dist > clearanceThreshold)
        {
            return CollisionType::NoCollision; // distance is big enough without contact after deformation
        }
        else if (dist <= clearanceThreshold && dist > 0) // if  contact or interference, dist == 0.0
        {
            CollisionInfo info = {i, j, dist, CollisionType::Clearance};
            myCollisionInfos[i].push_back(std::make_pair(j, info));
            CollisionInfo info_j = {j, i, dist, CollisionType::Clearance};
            myCollisionInfos[j].push_back(std::make_pair(i, info_j));
            return CollisionType::Clearance;
        }
        else
        {
            if (dist == 0.0)
            {
                VLOG_F(LOGLEVEL_DEBUG,
                       "zero distance between item %lu and %lu, "
                       "\n however this method should be called after excluding interference or contact",
                       i, j);
            }
            return CollisionType::Error;
        }
    }

    void CollisionDetector::dealGeneralFuseException(const std::vector<TopoDS_Shape> twoShapes,
                                                     const std::vector<ItemIndexType> itemIndices)
    {
        ItemIndexType it = 0;
        for (const auto& s : twoShapes)
        {
            BRepCheck_Analyzer analyzer(s); // check input geometry, it has been done
            if (!analyzer.IsValid())
            {
                // const auto& r = analyzer.Result(s);
                // r->StatusOnShape()
                LOG_F(ERROR, "input solid #%lu `%s` is invalid according to BRepCheck_Analyzer", itemIndices[it],
                      itemName(itemIndices[it]).c_str());
            }
            it++;
        }
    }

    CollisionType CollisionDetector::solveErrorByDistanceCheck(const ItemIndexType i, const ItemIndexType j)
    {
        CollisionType ctype = CollisionType::Error;
        auto dist = OccUtils::distance(item(i), item(j));
        if (dist > 0)
        {
            if (dist <= clearanceThreshold)
                ctype = CollisionType::Clearance;
            else
                ctype = CollisionType::NoCollision;
            VLOG_F(LOGLEVEL_DEBUG, "Error from calcCollisionType is recorrected for the pair #%lu  #%lu ", i, j);
        }
        else // still can not recover the error, then dump
        {
            auto dump_file_name = generateDumpName("dump_volumeError", {i, j}) + myDumpFileType;
            OccUtils::saveShape({item(i), item(j)}, dump_file_name);
        }
        return ctype;
    }

    /// if bool fragments result in more than 2 solids, there is interference
    /// if the interference is small (volume threshold), substract it from the bigger volume
    /// as marked it as solved (return true)
    std::vector<TopoDS_Shape> CollisionDetector::solveWeakInterference(const std::vector<TopoDS_Shape> resultsPieces,
                                                                       std::vector<Standard_Real> resultsVols,
                                                                       const std::vector<TopoDS_Shape>, //  origItems
                                                                       const std::vector<Standard_Real> origVols)
    {
        // std::shared_ptr<const BRepAlgoAPI_BuilderAlgo> mkGFA
        if (resultsVols.size() == 0) // empty, default parameter value
        {
            for (const auto& s : resultsPieces)
            {
                GProp_GProps v_props;
                BRepGProp::VolumeProperties(s, v_props);
                auto v = v_props.Mass();
                resultsVols.push_back(v);
            }
        }

        /// find the index for the max volume, min volume, only for 2 input shapes
        auto iBig = (origVols[0] > origVols[1]) ? 0 : 1;
        auto iSmall = (origVols[0] < origVols[1]) ? 0 : 1;
        // choose a strategy:  merge to the biggest or the smallest result fragament?
        auto iKept = iSmall;
        /// if 2 input has almost same volume, then it does not matter which is kept
        auto iKept_result = shapeMatch(origVols[iKept], resultsPieces, resultsVols);

        std::vector<TopoDS_Shape> results;
        TopoDS_Shape theKept = resultsPieces[iKept_result];
        TopoDS_Shape theOtherShape;
        auto restPieces = std::vector<TopoDS_Shape>();
        for (size_t i = 0; i < resultsPieces.size(); i++)
        {
            if (i != iKept_result)
                restPieces.push_back(resultsPieces[i]);
        }
        if (resultsPieces.size() > 2)
        {
            try
            {
                OCC_CATCH_SIGNALS
                theOtherShape = OccUtils::fuseShape(restPieces);
            }
            catch (const Standard_Failure& fail)
            {
                VLOG_F(LOGLEVEL_DEBUG, "OCCT Standard_Failure %s in shape fusion in weak inference fixing",
                       fail.GetMessageString());
                theOtherShape = restPieces[0];
            }
            catch (const std::exception& e)
            {
                VLOG_F(LOGLEVEL_DEBUG, "Exception %s in shape fusion in weak inference fixing", e.what());
                theOtherShape = restPieces[0];
            }
        }
        else
        {
            size_t iOther_result = iKept_result == 1 ? 0 : 1;
            theOtherShape = resultsPieces[iOther_result];
        }

        // merge all the other smaller, or using the original smaller volume
        if (iKept == 0)
        {
            results.push_back(theKept);
            results.push_back(theOtherShape);
        }
        else
        {
            results.push_back(theOtherShape);
            results.push_back(theKept);
        }
        return results;
    }


    size_t CollisionDetector::shapeMatch(Standard_Real volume, const std::vector<TopoDS_Shape> shapes,
                                         const std::vector<Standard_Real> vols)
    {
        size_t matched_index = 0;
        for (size_t i = 0; i < shapes.size(); i++)
        {
            // TODO:  should also compare boundbox of the input shapes to double confirm the matching
            if (OccUtils::floatEqual(volume, vols[i], 0.1)) // 0.1 should be replaced by parameter
                matched_index = i;
        }
        return matched_index;
    }

    // consisder move to GeometryProcessor class
    void CollisionDetector::dumpChangedBoundbox(const ItemIndexType eIndex, const ItemType changedShape) const
    {
        auto df_original = generateDumpName("dump_bndboxOriginal", {eIndex}) + myDumpFileType;
        OccUtils::saveShape(item(eIndex), df_original); // if exists, overwriting
        auto df = generateDumpName("dump_bndboxChanged", {eIndex}) + myDumpFileType;
        OccUtils::saveShape(changedShape, df);
        // this info does not write to log file
        std::stringstream os;

        json b1 = (*myShapeBoundBoxes)[eIndex];
        json b2 = OccUtils::calcBndBox(changedShape);
        os << "item " << eIndex << " has original boundbox " << b1 << std::endl;
        os << "item " << eIndex << " has changed boundbox " << b2;
        VLOG_F(LOGLEVEL_DEBUG, "%s", os.str().c_str());
    }

    /// the caller has checked item i has not been suppressed
    json CollisionDetector::suppressItemImpl(const ItemIndexType i, const CollisionType ctype)
    {
        suppressItem(i, to_ShapeErrorType(ctype));

        auto dump_file_name = generateDumpName("dump_suppressed", {i}) + myDumpFileType;

        std::string df;
        // saveShape() may throw SIGESGV, this signal can be converted to Exception on Linux
        try
        {
            OCC_CATCH_SIGNALS
            OccUtils::saveShape(item(i), dump_file_name);
            df = dumpRelatedItems(i);
        }
        catch (const Standard_Failure& fail)
        {
            VLOG_F(LOGLEVEL_DEBUG, "OCCT Standard_Failure %s in shape saving", fail.GetMessageString());
        }
        catch (...)
        {
            LOG_F(WARNING, "exception happened during saving shape for item #%lu", i);
        }

        json msg = {{"type", "request"}, {"data", dump_file_name}};
        msg["extraData"] = df;
        return msg;
    }

    std::string CollisionDetector::dumpRelatedItems(const ItemIndexType i)
    {
        std::vector<ItemType> relatedShapes;
        relatedShapes.push_back(item(i)); // the troublesome item must also be exported
        const auto& row = myCollisionInfos[i];
        for (const auto& p : row)
        {
            relatedShapes.push_back(item(p.second.second));
        }
        auto df = generateDumpName("dump_relatedShape", {i}) + myDumpFileType;
        // consider: dump name and color in step format

        OccUtils::saveShape(relatedShapes, df);
        return df;
    }
    ////////////////////////////////////////////////////////////////////////////////
    size_t CollisionDetector::countType(const ItemIndexType i, const CollisionType collisionType)
    {
        const auto& row = myCollisionInfos[i];
        return std::count_if(row.cbegin(), row.cend(), [=](const std::pair<ItemIndexType, CollisionInfo> p) {
            const CollisionInfo& info = p.second;
            return (info.type == collisionType);
        });
    }
    /**
     * if they have similar volume and boundbox, within relative tolerance (0.01)
     * */
    bool CollisionDetector::hasAtLeastType(const ItemIndexType i, const CollisionType collisionType)
    {
        const auto& row = myCollisionInfos[i];
        return std::any_of(row.cbegin(), row.cend(), [=](const std::pair<ItemIndexType, CollisionInfo> p) {
            const CollisionInfo& info = p.second;
            return (info.type >= collisionType);
        });
    }

    bool CollisionDetector::isAllTypeExcept(const ItemIndexType i, const CollisionType collisionType,
                                            const ItemIndexType exception)
    {
        const auto& row = myCollisionInfos[i];
        return std::all_of(row.cbegin(), row.cend(), [=](const std::pair<ItemIndexType, CollisionInfo> p) {
            const CollisionInfo& info = p.second;
            return (info.type == collisionType || i == exception);
        });
    }

    bool CollisionDetector::isNotTypeExcept(const ItemIndexType i, const CollisionType collisionType,
                                            const ItemIndexType exception)
    {
        const auto& row = myCollisionInfos[i];
        return std::all_of(row.cbegin(), row.cend(), [=](const std::pair<ItemIndexType, CollisionInfo> p) {
            const CollisionInfo& info = p.second;
            return (info.type != collisionType || i == exception);
        });
    }

    /** second round process is serial, call after `myCollisionInfos` has been built, */
    void CollisionDetector::processCollisionInfo(const ItemIndexType i)
    {
        if (itemSuppressed(i))
            return; // has been resolved by suppressing this item before call this function

        const auto& row = myCollisionInfos[i];
        if (row.size() == 0) // NoCollision is not saved
        {
            // one body without contact/interference with other bodies, suppress it
            if (suppressFloating)
            {
                suppressItem(i, ShapeErrorType::FloatingShape);
                // VLOG_F(LOGLEVEL_DEBUG, "Item #%lu `%s` suppressed as it is floating, no contact with other", i,
                //       itemName(i).c_str());
            }
            else
            {
                // suppress this debug information, too noisy, analyzeProcessedResult.py can retrieve this info
                // VLOG_F(LOGLEVEL_DEBUG, "Item #%lu `%s` in NOT suppressed, although a floating shape ", i,
                //       itemName(i).c_str());
            }
            // todo: report to the operator but does not need response;
        }
        else if (row.size() == 1)
        {
            const auto& info = row[0].second;
            const CollisionType ctype = info.type;
            const auto j = info.second;
            // remove the smaller, or keep the one with more contactCount
            if (ctype == CollisionType::Enclosure)
            {
                auto vol_i = (*myGeometryProperties)[i].volume;
                auto vol_j = (*myGeometryProperties)[j].volume;
                auto k = (vol_i < vol_j) ? i : j;
                if (not itemSuppressed(k))
                {
                    suppressItemImpl(k, CollisionType::Enclosure);
                }
            }
            else if (ctype == CollisionType::Error)
            {
                if (not itemSuppressed(i))
                {
                    suppressItemImpl(i, ctype);
                }
                // notifyCollision(info, "Error: during general fuse to detect collision type");
            }
            else if (ctype == CollisionType::Clearance)
            {
                // todo: assembly is not yet done, it should be in contact with at least one
                // notifyCollision(info, "Error: this part has a small clearance with only one neighbour");
            }
            else if (ctype == CollisionType::Interference)
            {
                // the other has no other interference, and total items > 2
                // if (myCollisionInfos[j].size() > 1 and countType(j, CollisionType::FaceContact) > 0)
                if (not itemSuppressed(i))
                {
                    suppressItemImpl(i, ctype);
                }
            }
            else if (ctype == CollisionType::Coincidence)
            {
                // item i and j are both floating, on relation with other items
                if (not itemSuppressed(i))
                {
                    suppressItemImpl(i, ctype);
                }
                // LOG_F(info, "Error: both shapes with coincidence are not in contact with other");
            }
            else
            {
                return; // do nothing
            }
        }
        else // > 1, interference, coincidence, enclosure has been dealt
        {
            // three or more body interference not checked.
            if (hasAtLeastType(i, CollisionType::Interference))
            {
                // Enclosure has been solved, here solves CollisionType::Coincidence
                bool coincidenceResolved = false;
                for (const auto& p : row)
                {
                    const auto& cinfo = p.second;
                    auto j = cinfo.second;
                    if (cinfo.type == CollisionType::Coincidence)
                    {
                        if (i < j) // remove either is fine, but avoid removing twice!
                            suppressItem(i, ShapeErrorType::Coincidence);
                        coincidenceResolved = true;
                    }
                    if (cinfo.type == CollisionType::Enclosure)
                    {
                        // the smaller volume must only 1 relationship with other,
                        // so it should have been suppressed
                        coincidenceResolved = true;
                    }
                }

                // if one body interferes/has error with the other,
                // the less total interference should be kept, disable the item with bigger Error count
                // in case of CollisionType::Unknown, just leave it
                resolveCollisionError(i, CollisionType::Interference);
                resolveCollisionError(i, CollisionType::Error);
            }
        }
    }

    /// resolve CollisionType::Interference or CollisionType::Error in serial, second round
    void CollisionDetector::resolveCollisionError(const ItemIndexType i, const CollisionType ctype)
    {
        auto thisCount = countType(i, ctype);
        if (thisCount > 0)
        {
            const auto& row = myCollisionInfos[i];
            std::vector<size_t> interferenceCounts;
            for (const auto& p : row)
            {
                const auto& info = p.second;
                const CollisionType j_ctype = info.type;
                auto j = info.second;
                interferenceCounts.push_back(countType(j, j_ctype));
            }
            auto maxOther = *std::max_element(interferenceCounts.begin(), interferenceCounts.end());

            if (thisCount > maxOther)
            {
                if (not itemSuppressed(i))
                {
                    suppressItemImpl(i, ctype);
                }
            }
            // thisCount == maxOther, this condition is too complicate, just report to operator!
        }
    }

    /// second round, to check if all interference and error has been resolved
    /// if still not resolved, just suppress
    bool CollisionDetector::checkCollisionResolution()
    {
        ItemIndexType floating = 0;
        // unsigned int floating = 0;
        std::map<ItemIndexType, CollisionType> unresolvedItems;
        const bool ignore_unknown = parameter<bool>("ignoreUnknownCollisionType");
        for (std::size_t i = 0; i < myInputData->itemCount(); i++)
        {
            const auto& row = myCollisionInfos[i];
            if (row.size() == 0)
                floating++;
            for (const auto& p : row)
            {
                const auto& cinfo = p.second;
                auto j = cinfo.second;
                // ignore CollisionType::Unknown, it should have been dumped during collisionType calc
                if (cinfo.type == CollisionType::Unknown)
                {
                    if (not(itemSuppressed(i) or itemSuppressed(j)) and (not ignore_unknown))
                    {
                        unresolvedItems.insert(std::make_pair(i, cinfo.type));
                        unresolvedItems.insert(std::make_pair(j, cinfo.type));
                    }
                }
                if (cinfo.type >= CollisionType::Interference and cinfo.type < CollisionType::Unknown)
                {
                    if (not(itemSuppressed(i) or itemSuppressed(j)) and suppressErroneous)
                    {
                        unresolvedItems.insert(std::make_pair(i, cinfo.type));
                        unresolvedItems.insert(std::make_pair(j, cinfo.type));
                    }
                }
            }
        }
        for (const auto& k : unresolvedItems)
        {
            notifyUnresolvedCollision(k.first, k.second);
        }
        if (floating > 0)
            LOG_F(WARNING, "floating item count is %lu out of total %lu", floating, itemCount());
        return unresolvedItems.size() == 0;
    }

    void CollisionDetector::notifyUnresolvedCollision(const ItemIndexType i, const CollisionType ctype)
    {
#if MAGIC_ENUM_SUPPORTED
        const char* ctype_name = std::string(enum_name<CollisionType>(ctype)).c_str();
        LOG_F(ERROR, "Item #%lu suppressed for unresolvable collision type %s", i, ctype_name);
#else
        LOG_F(ERROR, "Item #%lu suppressed for unresolvable collision type %d", i, ctype);
#endif
        auto msg = suppressItemImpl(i, CollisionType::Error);

        /// NOTE:  turn off this message to console for the time being
        // myOperator->display(msg);
        // myOperator->report(logInfo);
        /// wait for operator to select which one to keep
    }

} // namespace Geom