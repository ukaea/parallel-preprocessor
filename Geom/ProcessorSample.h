#pragma once

/// todo list after copy from this template:
/// replace class name,  correct the header path for the base processor
/// replace myDataProperty type and name, or delete it if not needed
/// register parameter from processor config
/// get all `requiredProperties` from myInputData in `prepareInput()`
#include "GeometryProcessor.h"

namespace Geom
{
    using namespace PPP;

    /// \ingroup Geom
    /**
     * \brief a brief description on the process/feature/functions
     */
    class ProcessorSample : public GeometryProcessor
    {
        TYPESYSTEM_HEADER();

    private:
        /// declare your new data properties here
        VectorType<std::size_t> myDataProperty;

    public:
        ProcessorSample()
        {
            // myCharacteristics["requiredProperties"] = {"myAdjacencyMatrix"};
            // myCharacteristics["producedProperties"] = {"myCollisionInfos"};
        }
        ~ProcessorSample() = default;

        /**
         * \brief preparing work in serial mode
         */
        virtual void prepareInput() override final
        {
            GeometryProcessor::prepareInput();
            /// prepare private properties like `std::vector<T>.resize(myInputData->itemCount());`
            /// therefore accessing item will not cause memory reallocation and items copying
            myDataProperty.resize(myInputData->itemCount());
        }

        /**
         * \brief preparing work in serial mode, write report, move data into `myOutputData`
         */
        virtual void prepareOutput() override final
        {
            myOutputData->emplace<decltype(myDataProperty)>("myDataProperty", std::move(myDataProperty));
        }

        /**
         * \brief process single data item in parallel without affecting other data items
         * @param index: index to get/set by item(index)/setItem(index, newDataItem)
         */
        virtual void processItem(const ItemIndexType index) override final
        {
        }

        /// implement this virtual function, if you need to process item(i)
        // which is related to /coupled with /depending on/modifying item(j)
        // virtual void processItemPair(const ItemIndexType i, const ItemIndexType j) override final

        /// private, protected functions
    private:
    };
} // namespace Geom
