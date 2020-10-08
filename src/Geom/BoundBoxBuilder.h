#pragma once

#include "GeometryProcessor.h"

namespace Geom
{
    using namespace PPP;
    /// \ingroup Geom
    /**
     * calculate the axis aligned boundabox for Solids and also Face ShapeType.
     *
     * NOTE:
     * boundbox can be infinite in any direction , or can be void/empty.
     * OCCT 7.3 has oriented bound box (OBB), for OCC_VERSION_HEX >= 0x070300, OBB is also calculated.
     * OCCT 7.4 has BVH boundbox tree
     */
    class BoundBoxBuilder : public GeometryProcessor
    {
        TYPESYSTEM_HEADER();

    private:
        VectorType<Bnd_Box> myShapeBoundBoxes;
#if OCC_VERSION_HEX >= 0x070300
        VectorType<Bnd_OBB> myShapeOrientedBoundBoxes;
#endif
    public:
        BoundBoxBuilder()
        {
            /// myCharacteristics["requiredProperties"]  have been set by parent class ctor()
            myCharacteristics["producedProperties"] = {"myShapeBoundBoxes", "myShapeOrientedBoundBoxes"};
        }

        virtual void prepareInput() override final
        {
            GeometryProcessor::prepareInput();
            auto count = myInputData->itemCount();
            myShapeBoundBoxes.resize(count);
#if OCC_VERSION_HEX >= 0x070300
            myShapeOrientedBoundBoxes.resize(count);
#endif
        }

        virtual void prepareOutput() override final
        {
            myOutputData->emplace("myShapeBoundBoxes", std::move(myShapeBoundBoxes));
#if OCC_VERSION_HEX >= 0x070300
            myOutputData->emplace("myShapeOrientedBoundBoxes", std::move(myShapeOrientedBoundBoxes));
#endif
        }

        virtual void processItem(std::size_t index) override final
        {
            const TopoDS_Shape& s = item(index);

            Bnd_Box boundingBox;
            /// default zero gap, here a global tolerance is used, set in GeometryProcessor
            boundingBox.SetGap(toleranceThreshold);
            BRepBndLib::Add(s, boundingBox); // which type, simple global coordinate
            myShapeBoundBoxes[index] = std::move(boundingBox);
#if OCC_VERSION_HEX >= 0x070300
            Bnd_OBB obb;
            BRepBndLib::AddOBB(s, obb);
            myShapeOrientedBoundBoxes[index] = obb; // std::move(obb); does not work!
#endif
        }
    };
} // namespace Geom
