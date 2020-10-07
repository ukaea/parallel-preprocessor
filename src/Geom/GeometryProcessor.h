// license
#ifndef PPP_GEOMETRY_PROCESSOR_H
#define PPP_GEOMETRY_PROCESSOR_H

#include "GeometryData.h"
#include "PPP/Processor.h"
#include "PPP/Utilities.h"


namespace Geom
{
    using namespace PPP;

    /// \ingroup Geom

    /** \brief Base class for all GeometryData processors
     * all virtual functions as default implementation, probably empty.
     * processor classes are not easy to fit into unit test, so functions are declared in OccUtils namespace
     * */
    class GeomExport GeometryProcessor : public Processor
    {
        TYPESYSTEM_HEADER();

    protected:
        Standard_Real toleranceThreshold;
        ShapeType myShapeType;
        std::shared_ptr<VectorType<ItemHashType>> myShapeIDs;
        std::shared_ptr<ItemContainerType> myShapes;
        std::shared_ptr<MapType<ItemHashType, std::string>> myNameMap;
        std::shared_ptr<MapType<ItemHashType, Quantity_Color>> myColorMap;
        std::shared_ptr<MapType<ItemHashType, Material>> myMaterialMap;
        std::shared_ptr<MapType<ItemHashType, ShapeErrorType>> myShapeErrors;

    public:
        /// consider: disable copy but enable move constructors and assigners
        GeometryProcessor()
        {
            myCharacteristics["modified"] = false; // will not modify item() by this processor
            myCharacteristics["indexPattern"] = IndexPattern::Linear;
            myCharacteristics["indexDimension"] = 1U; // indepent item processing, processing pair(i,j) for dim=2
            myCharacteristics["requiredProperties"] = {"mySolids", "myShapeType"}; // matching reader
        }
        virtual ~GeometryProcessor() = default;


        /// \defgroup ItemOperation
        /// @{
        inline ItemIndexType itemCount() const
        {
            return myInputData->itemCount();
        }
        /// shape item is stored in a map instead of vector, for easy insertion
        inline const TopoDS_Shape& item(const ItemIndexType index) const
        {
            return (*myShapes)[(*myShapeIDs)[index]];
        }

    protected: /// only derived classes can modify status
        /// derived class should override, if simply resignment is not sufficient
        /// item should have locked this item by unique write access to it.
        virtual void setItem(const ItemIndexType index, const TopoDS_Shape& newShape)
        {
            // updateShapeHash(index, newShape); //

            // const TopoDS_Shape& oldShape = item(index);
            // BRepTools_ReShape reshape;
            // reshape.Replace(oldShape, newShape);
            // return reshape.Apply(mothershape, TopAbs_SHAPE);

            (*myShapes)[(*myShapeIDs)[index]] = newShape;
            // currently, subtopology item of solids are not in used, so not needed to update
        }

        /// the linear index will increase by 1, so it will not affect previous data
        void insertItem()
        {
            // todo: all shared_ptr<> needs insertion, inputData should be updated automatically
        }

        /// some processor will modify shape item, so shapeIDs  and all other keys will outdate
        void updateShapeHash(const ItemIndexType index, const TopoDS_Shape& newShape)
        {
            (*myShapeIDs)[index] = newShape.HashCode(ItemHashMax);
            // todo: all other hash key must be updated
        }

        /// kind of remove item from downstream processing
        inline void suppressItem(const ItemIndexType index, const ShapeErrorType etype = ShapeErrorType::UnknownError)
        {
            if (myShapeErrors)
                (*myShapeErrors)[(*myShapeIDs)[index]] = etype;
        }

    public:
        /// used in distributive parallel after decomposition
        inline ItemIndexType itemGlobalIndex(const ItemIndexType) const
        {
            return 0UL; // todo:
        }

        inline bool itemSuppressed(const ItemIndexType index) const
        {
            if (myShapeErrors && myShapeErrors->find((*myShapeIDs)[index]) != myShapeErrors->end())
                return (*myShapeErrors)[(*myShapeIDs)[index]] != ShapeErrorType::NoError;
            else
                return false;
        }
        /// if there no name for the item, return empty string
        inline const std::string itemName(const ItemIndexType index) const
        {
            if (myNameMap->find((*myShapeIDs)[index]) != myNameMap->end())
                return (*myNameMap)[(*myShapeIDs)[index]];
            else
                return "item" + std::to_string(index); // do NOT return reference of local variable!
        }

        ShapeErrorType itemError(const ItemIndexType index) const
        {
            if (myShapeErrors->find((*myShapeIDs)[index]) != myShapeErrors->end())
                return (*myShapeErrors)[(*myShapeIDs)[index]];
            else
                return ShapeErrorType::NoError;
        }

        ///  test before use the value of the returned type `std::optional<Quantity_Color>`
        inline const std::optional<Quantity_Color> itemColor(const ItemIndexType index) const
        {
            if (myColorMap->find((*myShapeIDs)[index]) != myColorMap->end())
                return (*myColorMap)[(*myShapeIDs)[index]];
            else
                return std::nullopt;
        }
        ///
        inline const std::optional<Material> itemMaterial(const ItemIndexType index) const
        {
            if (myMaterialMap->find((*myShapeIDs)[index]) != myMaterialMap->end())
                return (*myMaterialMap)[(*myShapeIDs)[index]];
            else
                return std::nullopt;
        }
        /// @}

        /**
         * it must be done in serial, not in parallel
         * be default, get shared_ptr to "mySolids" as name it as `myShapes`
         */
        virtual void prepareInput() override
        {
            Processor::prepareInput();
            toleranceThreshold = Processor::globalParameter<Standard_Real>("toleranceThreshold");
            // cast from DataObject to GeometryData at runtime is costly, so do this
            if (myInputData->contains("myShapeType"))
                myShapeType = *(myInputData->get<ShapeType>("myShapeType"));
            else
                myShapeType = ShapeType::Solid;

            if (myShapeType == ShapeType::Solid and myInputData->contains("mySolids"))
            {
                myShapes = myInputData->get<ItemContainerType>("mySolids");
                myShapeIDs = myInputData->get<VectorType<ItemHashType>>("mySolidIDs");
                myShapeErrors = myInputData->get<MapType<ItemHashType, ShapeErrorType>>("myShapeErrors");
                myNameMap = myInputData->get<MapType<ItemHashType, std::string>>("myNameMap");
                myColorMap = myInputData->get<MapType<ItemHashType, Quantity_Color>>("myColorMap");
                myMaterialMap = myInputData->get<MapType<ItemHashType, Material>>("myMaterialMap");
            }
            else
            {
                LOG_F(ERROR, "this should be overridden by more concrete processors ");
            }
        }

        /**
         * it must be done in serial, not in parallel, default implementation do nothing
         */
        virtual void prepareOutput() override
        {
            Processor::prepareOutput();
        }

        /**
         * serial non parallel processing, e.g readers and writers
         */
        virtual void process() override
        {
            LOG_F(ERROR, "you should not use this function, call processItem as possible");
        }

        /// this should be overridden by more concrete processors
        virtual void processItem(const ItemIndexType i) override
        {
            const ItemIndexType NShapes = myInputData->itemCount();
            // uppper triangle traversal for the matrix, also skip i==j
            for (ItemIndexType j = i + 1; j < NShapes; j++)
            {
                if (isCoupledPair(i, j))
                    processItemPair(i, j);
            }
        }

        /// must implement the pure virtual function of the parent type, default to false
        virtual bool isCoupledPair(const ItemIndexType, const ItemIndexType) override
        {
            return false;
        }

        virtual void processItemPair(const ItemIndexType, const ItemIndexType) override
        {
            LOG_F(ERROR, "this should be overridden by more concrete processors");
        }
    };


} // namespace Geom

#endif // include file firewall