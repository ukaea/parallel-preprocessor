#pragma once

#include "GeometryProcessor.h"

namespace Geom
{
    using namespace PPP;

    /// \ingroup Geom

    /// Inscribed Shape (Sphere, Box, etc) is distinguished by `std::vector` length
    /// Sphere:  Center(x, y, z), r
    /// Box: like a OBB bound box, 6 elements:  xmin, ymin, zmin, xmax, ymax, zmax
    /// Cylinder:  7 elements: start, end, r
    /// consider Geom3D base class? gp_Sphere
    /// CONSIDER: std::variant<Bnd_OBB, Bnd_Sphere, Bnd_Cylinder, gp_Cone>
    using InscribedShapeType = std::vector<double>;


    /**
     * \brief calculate the Inscribed shape for a given shape if it is shell type
     *
     * result will be saved to a sparse vector of InscribedShapeType
     * which is a std::vector<double>, could be Sphere, Cylinder, Box
     */
    class InscribedShapeBuilder : public GeometryProcessor
    {
        TYPESYSTEM_HEADER();

    private:
        /// consider return more than one Inscribed shape?
        SparseVector<InscribedShapeType> myInscribedShapes;

        std::shared_ptr<VectorType<GeometryProperty>> myGeometryProperties;
        std::shared_ptr<VectorType<Bnd_OBB>> myShapeOrientedBoundBoxes;

    public:
        InscribedShapeBuilder()
        {
            myCharacteristics["indexPattern"] = IndexPattern::Linear;
            // myCharacteristics["indexDimension"] = 1;
            myCharacteristics["requiredProperties"] = {"myGeometryProperties"};
            myCharacteristics["producedProperties"] = {"myInscribedShapes"};
        }
        ~InscribedShapeBuilder() = default;

        /**
         * \brief preparing work in serial mode
         */
        virtual void prepareInput() override final
        {
            GeometryProcessor::prepareInput();

            myGeometryProperties = myInputData->get<VectorType<GeometryProperty>>("myGeometryProperties");
            if (myInputData->contains("myShapeOrientedBoundBoxes"))
                myShapeOrientedBoundBoxes = myInputData->get<VectorType<Bnd_OBB>>("myShapeOrientedBoundBoxes");

            /// prepare private properties like `std::vector<T>.resize(myInputData->itemCount());`
            /// therefore accessing item will not cause memory reallocation and items copying
            myInscribedShapes = SparseVector<std::vector<double>>(myInputData->itemCount(), nullptr);
        }

        /**
         * \brief preparing work in serial mode, write report, move data into `myOutputData`
         */
        virtual void prepareOutput() override final
        {
            myOutputData->emplace("myInscribedShapes", std::move(myInscribedShapes));
        }

        /**
         * \brief process single data item in parallel without affecting other data items
         * @param index: index to get/set by item(index)/setItem(index, newDataItem)
         */
        virtual void processItem(const ItemIndexType index) override final
        {
            const auto gp = (*myGeometryProperties)[index];
            const auto obb = (*myShapeOrientedBoundBoxes)[index];
            if (isShell(gp, obb))
            {
                auto ret = calcInscribedShape(item(index), gp, obb);
                myInscribedShapes[index] = std::make_shared<InscribedShapeType>(std::move(ret));
            }
        }

        /// implement this virtual function, if you need to process item(i)
        // which is related to /coupled with /depending on/modifying item(j)
        // virtual void processItemPair(const ItemIndexType i, const ItemIndexType j) override final

        /// private, protected functions
    private:
        /// detect if the shape is roughly a shell or hollow shape
        /// condition 1: volume and area ratio,  the max is a sphere
        /// condition 2: volume / OBB volume,  to exclude one planar sheet
        /// resolved shape?
        bool isShell(const GeometryProperty& gp, const Bnd_OBB& obb)
        {
            const double sphere_ratio = 1.0 / 3.0 / std::sqrt(M_PI * 4);
            const double th = sphere_ratio * 0.2;
            bool b1 = ((gp.volume / gp.area) / std::sqrt(gp.area) < th);
            bool b2 = gp.volume * 5 < volume(obb);
            if (b1 && b2)
                return true;
            return false;
        }

        std::vector<double> calcAreaPerSurfaceType(const ItemType& s);
        InscribedShapeType calcInscribedShape(const ItemType& s, const GeometryProperty& gp, const Bnd_OBB& obb);

        /// shape is roughly a sphere, cylinder, box,
        SurfaceType estimateShapeType(const ItemType& s, const GeometryProperty& gp, const Bnd_OBB& obb)
        {
            auto v = calcAreaPerSurfaceType(s);
            // calc the max, get the index
            auto i = std::distance(v.cbegin(), std::max_element(v.cbegin(), v.cend()));

            return SurfaceType(i);
        }

        /// detect if the shape is roughly a planar sheet
        /// obb vol slightly bigger than the shape volume,  aspect ratios
        bool isPlanarSheet(const GeometryProperty& gp, const Bnd_OBB& obb) const
        {
            auto ar = aspectRatios(obb);
            const double ratio = 0.2;
            return ar[0] < ratio and ar[1] < ratio;
        }

        inline double volume(const Bnd_OBB& obb) const
        {
            return obb.XHSize() * obb.YHSize() * obb.ZHSize() * 8;
        }

        inline std::vector<double> aspectRatios(const Bnd_OBB& obb) const
        {
            std::vector myvector = {obb.XHSize(), obb.YHSize(), obb.ZHSize()};
            std::sort(myvector.begin(), myvector.end());
            auto max = myvector[2];
            return {myvector[0] / max, myvector[1] / max};
        }
    };
} // namespace Geom
