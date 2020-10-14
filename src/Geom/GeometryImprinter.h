// license
#ifndef PPP_GEOMETRY_MERGER_H
#define PPP_GEOMETRY_MERGER_H

#include <PPP/SparseMatrix.h>

#include "GeometryData.h"
#include "GeometryProcessor.h"
#include "OccUtils.h"


namespace Geom
{
    using namespace PPP;

    /// \ingroup Geom
    /**
     * This class is derived from CollisionDector class, which does not modify input geometry data.
     * `mySolids` property is changed
     * using lock, it is possible to do in parallel, not sure about the efficiency
     * using a parallel lock free scheduler, it is also possible
     * build a AdjacentMatrix (in sparse matrix is also NOT thread-safe), could be done in this processor
     *
     *
     * boolean fragment: for simulation and also overlapping/interference check
     */
    class GeometryImprinter : public CollisionDetector
    {
        TYPESYSTEM_HEADER();

    public:
        // using CollisionDetector::CollisionDetector;
        GeometryImprinter()
        {
            myCharacteristics["modified"] = true;
            myCharacteristics["coupled"] = true;
        }

        virtual void prepareInput() override final
        {
            CollisionDetector::prepareInput();
        }

        virtual void prepareOutput() override final
        {
            CollisionDetector::prepareOutput();
        }

        /// turn off OCCT internal multiple threading, parallel externally
        virtual void processItemPair(const ItemIndexType i, const ItemIndexType j) override final
        {
            if (detectBoundBoxOverlapping(i, j, toleranceThreshold))
            {
                if (not(itemSuppressed(i) or itemSuppressed(j)))
                    detectCollision(i, j, false, true);
            }
            else // boundbox check
            {
                if (detectBoundBoxOverlapping(i, j, clearanceThreshold))
                    calcClearance(i, j);
            }
        }

    }; // class ending

} // namespace Geom

#endif