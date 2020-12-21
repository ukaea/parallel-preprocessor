#pragma once

#include "GeometryProcessor.h"
#include "OccUtils.h"

#include <variant>

namespace Geom
{
    using namespace PPP;

    /// \ingroup Geom

    /// Inscribed Shape (Sphere, Box, etc) is distinguished by `std::vector` length
    /// Sphere:  Center(x, y, z), r
    /// Box: like  6 elements:  xmin, ymin, zmin, xmax, ymax, zmax
    /// a OBB bound box,
    /// Cylinder:  7 elements: start, end, r, gp_Cylinder
    /// consider Geom3D base class? gp_Sphere
    /// CONSIDER: std::variant<Bnd_OBB, Bnd_Sphere, Bnd_Cylinder, gp_Cone>, then use std::visit
    ///           they
    using InscribedShapeType = std::variant<Bnd_OBB, Bnd_Sphere>;


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
            myInscribedShapes = SparseVector<InscribedShapeType>(myInputData->itemCount(), nullptr);
        }

        /**
         * \brief preparing work in serial mode, write report, move data into `myOutputData`
         */
        virtual void prepareOutput() override final
        {
            if (myConfig.contains("output"))
            {
                auto file_name = dataStoragePath(parameter<std::string>("output"));
                writeResult(file_name);
                LOG_F(INFO, "output inscribed shapes into file: %s", file_name.c_str());
            }
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
            if (true) // hasLargeVoid(gp, obb)
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
        /// condition 1: volume and area ratio,  the max is a sphere, tge smaller is sheet
        /// condition 2: volume / OBB volume,  to exclude one planar sheet
        /// resolved shape?
        bool hasLargeVoid(const GeometryProperty& gp, const Bnd_OBB& obb)
        {
            const double sphere_ratio = 1.0 / 3.0 / std::sqrt(M_PI * 4);
            const double th = sphere_ratio * 0.2;
            bool b1 = ((gp.volume / gp.area) / std::sqrt(gp.area) < th);
            const double volume_ratio = 3;
            bool b2 = gp.volume * volume_ratio < volume(obb);
            if (b1 && b2)
                return true;
            return false;
        }

        std::vector<double> calcAreaPerSurfaceType(const ItemType& s);
        InscribedShapeType calcInscribedShape(const ItemType& s, const GeometryProperty& gp, const Bnd_OBB& obb);

        /// shape is roughly a sphere, cylinder, box,
        SurfaceType estimateShapeType(const ItemType& s, const GeometryProperty&, const Bnd_OBB&)
        {
            auto v = calcAreaPerSurfaceType(s);
            // calc the max, get the index
            auto i = std::distance(v.cbegin(), std::max_element(v.cbegin(), v.cend()));

            return SurfaceType(i);
        }

        /// detect if the shape is roughly a planar sheet
        /// obb vol slightly bigger than the shape volume,  aspect ratios
        bool isPlanarSheet(const Bnd_OBB& obb) const
        {
            auto ar = aspectRatios(obb);
            const double ratio = 0.2;
            return ar[0] < ratio and ar[1] < ratio;
        }

        inline double volume(const Bnd_OBB& obb) const
        {
            return obb.XHSize() * obb.YHSize() * obb.ZHSize() * 8;
        }

        /// consider: split out into BndUtils.h
        inline std::vector<double> aspectRatios(const Bnd_OBB& obb) const
        {
            std::vector myvector = {obb.XHSize(), obb.YHSize(), obb.ZHSize()};
            std::sort(myvector.begin(), myvector.end());
            auto max = myvector[2];
            return {myvector[0] / max, myvector[1] / max};
        }

        ItemType inscribedShapeToShape(const InscribedShapeType& shape)
        {
            std::cout << "variant index = " << shape.index() << std::endl;
            // if (std::holds_alternative<Bnd_Sphere>(shape))
            //     return toShape(std::get<Bnd_Sphere>(shape));
            // else if (std::holds_alternative<Bnd_OBB>(shape))
            //     return toShape(std::get<Bnd_OBB>(shape));
            if (std::holds_alternative<Bnd_Sphere>(shape))
                return toShape(std::get<Bnd_Sphere>(shape));
            else if (std::holds_alternative<Bnd_OBB>(shape))
                return toShape(std::get<Bnd_OBB>(shape));
            else
                throw ProcessorError("BoundShape type not supported in std::variant");
        }
        // template <typename ShapeT> ItemType toShape(const ShapeT& s);
        ItemType toShape(const Bnd_Sphere& s)
        {
            return BRepPrimAPI_MakeSphere(s.Center(), s.Radius());
        }

        ItemType toShape(const Bnd_OBB& obb)
        {
            auto ax2 = gp_Ax2(obb.Center(), obb.ZDirection(), obb.YDirection()); // not sure
            ax2.Translate(gp_Vec(obb.XHSize() * -1, obb.YHSize() * -1, obb.ZHSize() * -1));
            // todo: translate the box
            return BRepPrimAPI_MakeBox(ax2, obb.XHSize() * 2, obb.YHSize() * 2, obb.ZHSize() * 2).Shape();
        }

        void writeResult(const std::string filename)
        {
            std::vector<ItemType> shapes;
            // auto converter = [&, this](const auto& s) -> ItemType { return toShape(s); }
            // std::visit(converter, *p)
            for (const auto& p : myInscribedShapes)
            {
                if (p)
                    shapes.push_back(inscribedShapeToShape(*p));
            }

            /// may save in dataStorage folder
            if (shapes.size() > 0)
                OccUtils::saveShape(OccUtils::createCompound(shapes), filename);
            else
                LOG_F(INFO, "result inscribed shapes are empty, skip writting");
        }
    };
} // namespace Geom
