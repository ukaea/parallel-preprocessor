// license

#pragma once

#ifndef PPP_COLLISION_DETECTOR_H
#define PPP_COLLISION_DETECTOR_H

#include "GeometryProcessor.h"
#include "GeometryTypes.h"

#include "OccUtils.h"
#include "PPP/SparseMatrix.h"


namespace Geom
{
    using namespace PPP;

    /// \ingroup Geom
    /**
     * collision and proximity detection by general fusion boolean operation
     *
     * NOTE: oce/Voxel_CollisionDetection is removed from OCCT 7.x
     * OpenCASCADE has non-free module for collision and proximity detection with the related points/faces
     */
    class GeomExport CollisionDetector : public GeometryProcessor
    {
        TYPESYSTEM_HEADER();

    protected:
        Standard_Real tolerance;
        double clearanceThreshold;
        bool suppressFloating;
        bool suppressErroneous;

        /// ".brep" ".step" file formats are supported, but step write has lots of print
        std::string myDumpFileType = ".brep";

        std::shared_ptr<VectorType<Bnd_Box>> myShapeBoundBoxes;
        std::shared_ptr<VectorType<GeometryProperty>> myGeometryProperties;
        std::shared_ptr<VectorType<Bnd_OBB>> myShapeOrientedBoundBoxes;

        AdjacencyMatrixType myAdjacencyMatrix;
        SparseMatrix<CollisionInfo> myCollisionInfos;
        std::unordered_map<CollisionType, ItemIndexType> myCollisionSummary;

    public:
        CollisionDetector()
        {
            // The parent's default ctor be called automatically (implicitly)
            myCharacteristics["coupled"] = true;
            myCharacteristics["indexPattern"] = IndexPattern::FilteredMatrix;
            myCharacteristics["indexDimension"] = 2;
            myCharacteristics["requiredProperties"] = {"myShapeBoundBoxes", "myGeometryProperties"};
            myCharacteristics["producedProperties"] = {"myCollisionInfos"};
        }
        ~CollisionDetector() = default;

        virtual void prepareInput() override
        {
            GeometryProcessor::prepareInput();

            double defaultTolerance = toleranceThreshold; // from parent ctor()
            tolerance = parameter<Standard_Real>("tolerance", defaultTolerance);
            // Standard_Real tol_multiplier = 10.0;
            clearanceThreshold = parameterValue<Standard_Real>("clearanceThreshold", 0.5);
            suppressFloating = parameter("suppressFloating", false);
            suppressErroneous = parameter("suppressErroneous", true);

            myShapeBoundBoxes = myInputData->get<VectorType<Bnd_Box>>("myShapeBoundBoxes");

            if (myInputData->contains("myShapeOrientedBoundBoxes"))
                myShapeOrientedBoundBoxes = myInputData->get<VectorType<Bnd_OBB>>("myShapeOrientedBoundBoxes");

            myGeometryProperties = myInputData->get<VectorType<GeometryProperty>>("myGeometryProperties");
            // resize() avoid reallocate memeroy and invalidate iterator/memory address
            myAdjacencyMatrix.resize(myInputData->itemCount());
            myCollisionInfos.resize(myInputData->itemCount());

            // this may be not the best place to call external progressor,
            // but it is the only place, if we want a progressor for a specific time-consuming processor
            if (itemCount() > 1000)
            {
                Processor::startExternalProgressor();
            }
        }

        virtual void prepareOutput() override
        {
            auto mat_file_name = dataStoragePath("myAdjacencyMatrix.mm");
            myAdjacencyMatrix.writeMatrixMarketFile(mat_file_name);
            myOutputData->emplace("myAdjacencyMatrix", std::move(myAdjacencyMatrix));

            for (std::size_t i = 0; i < myInputData->itemCount(); i++)
            {
                processCollisionInfo(i);
            }
            auto file_name = dataStoragePath(parameter<std::string>("output", "myCollisionInfos.json"));
            myCollisionInfos.toJson(file_name);
            checkCollisionResolution();
        }

        virtual bool isCoupledPair(const ItemIndexType i, const ItemIndexType j) override final
        {
            return detectBoundBoxOverlapping(i, j, clearanceThreshold); // capture item pair near each other
        }

        /// turn off OCCT internal multiple threading, PPP will schedule the job using multithreading
        virtual void processItemPair(const ItemIndexType i, const ItemIndexType j) override
        {
            if (detectBoundBoxOverlapping(i, j, toleranceThreshold))
            {
                /// NOTE: here comment out code is not removed, to reminder developer
                /// that we ignore item suppressed status

                // if (not(itemSuppressed(i) or itemSuppressed(j)))
                detectCollision(i, j, false, false); // this function will not imprint/modify shapes
            }
            else // boundbox check
            {
                if (detectBoundBoxOverlapping(i, j, clearanceThreshold))
                    calcClearance(i, j);
            }
        }

    public:
        /// @{ static method group for unit test
        /** using fusion volume to detect collision type between 2 solids, called after generalFuse() */
        static CollisionType calcCollisionType(std::shared_ptr<BRepAlgoAPI_BuilderAlgo> mkGFA,
                                               std::vector<Standard_Real> origVols = {},
                                               std::vector<ItemIndexType> ij = {});
        /** collision detection by boolean fragments, including contact, enclosure, interference */
        static bool hasCollision(const TopoDS_Shape& s, const TopoDS_Shape& s2, double theTolerance = 0.0);
        /** using fusion volume to detect collision type */
        static CollisionInfo detectCollision(const TopoDS_Shape& s, const TopoDS_Shape& s2,
                                             std::vector<Standard_Real> volumes, double theTolerance = 0.0);
        static CollisionType detectCollisionType(const TopoDS_Shape& s, const TopoDS_Shape& s2,
                                                 std::vector<Standard_Real> volumes, double theTolerance = 0.0)
        {
            return detectCollision(s, s2, volumes, theTolerance).type;
        }
        /// @}

    protected:
        /// fixing weak interference as face contact will change boundbox and volume/area
        virtual void setItem(const ItemIndexType index, const TopoDS_Shape& newShape) override
        {
            GeometryProcessor::setItem(index, newShape); // hash id, and replace item

            // update other properties
            (*myGeometryProperties)[index] = OccUtils::geometryProperty(newShape);
            (*myShapeBoundBoxes)[index] = OccUtils::calcBndBox(newShape);
            if (myInputData->contains("myShapeOrientedBoundBoxes"))
                (*myShapeOrientedBoundBoxes)[index] = OccUtils::calcBndBox(newShape);
        }

        bool detectCollision(const ItemIndexType i, const ItemIndexType j, bool internalMultiThreading = true,
                             bool imprinting = false);
        CollisionType calcClearance(const ItemIndexType i, const ItemIndexType j);
        bool detectBoundBoxOverlapping(const ItemIndexType i, const ItemIndexType j, double clearance);

    private:
        void dealGeneralFuseException(const std::vector<TopoDS_Shape> twoShapes,
                                      const std::vector<ItemIndexType> itemIndices);
        CollisionType solveErrorByDistanceCheck(const ItemIndexType i, const ItemIndexType j);

        /// if bool fragments result in more than 2 solids, there is interference
        /// if the interference is small (volume threshold), substract it from the bigger volume
        /// as marked it as solved (return true)
        std::vector<TopoDS_Shape> solveWeakInterference(const std::vector<TopoDS_Shape> resultsPieces,
                                                        std::vector<Standard_Real> resultsVols,
                                                        const std::vector<TopoDS_Shape> origItems,
                                                        const std::vector<Standard_Real> origVols);
        size_t countType(const ItemIndexType i, const CollisionType collisionType);
        /**
         * CollisionType is an enum type which can be used as integer
         * if they have similar volume and boundbox, within relative tolerance (0.01)
         * */
        bool hasAtLeastType(const ItemIndexType i, const CollisionType collisionType);
        bool isAllTypeExcept(const ItemIndexType i, const CollisionType collisionType, const ItemIndexType exception);
        bool isNotTypeExcept(const ItemIndexType i, const CollisionType collisionType, const ItemIndexType exception);
        size_t shapeMatch(Standard_Real volume, const std::vector<TopoDS_Shape> shapes,
                          const std::vector<Standard_Real> vols);
        /// consisder move to GeometryProcessor class
        void dumpChangedBoundbox(const ItemIndexType eIndex, const ItemType changedShape) const;
        json suppressItemImpl(const ItemIndexType i, const CollisionType ctype);
        /// todo: dump name and color in step format
        std::string dumpRelatedItems(const ItemIndexType i);

        /** second round process is serial, call if `myCollisionInfos` has been built, */
        void processCollisionInfo(const ItemIndexType i);
        /// resolve CollisionType::Interference or CollisionType::Error in serial, second round
        void resolveCollisionError(const ItemIndexType i, const CollisionType ctype);
        /// second round, to check if all interference and error has been dealt
        bool checkCollisionResolution();
        void notifyUnresolvedCollision(const ItemIndexType i, const CollisionType ctype);
    };

} // namespace Geom

#endif
