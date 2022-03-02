#pragma once

#include "Processor.h"
#include "SparseMatrix.h"

namespace PPP
{

    /**
     * \brief  detect item pair coupling in the target expensive processor and build a coupling matrix
     *
     * the coupling matrix is more general than Geometry's adjacency matrix.
     * this processor can be used to detect/filter potential coupling and skip item pair definitely not in
     * coupling. parallel accessor need this information to coordinate coupling operation in parallel
     *
     */
    class AppExport CouplingMatrixBuilder : public Processor
    {
        TYPESYSTEM_HEADER();

    private:
        /// declare your new data properties here
        SparseMatrix<bool> myCouplingMatrix;

        std::shared_ptr<Processor> myTargetProcessor;

    public:
        CouplingMatrixBuilder()
        {
            myCharacteristics["coupled"] = false;
            myCharacteristics["indexPattern"] = IndexPattern::UpperTriangle;
            myCharacteristics["producedProperties"] = {"myCouplingMatrix"};
            myCharacteristics["requiredProperties"] = json::array();
        }
        ~CouplingMatrixBuilder() = default;

        void setTargetProcessor(std::shared_ptr<Processor> p)
        {
            myTargetProcessor = p;
        }

        /**
         * this function is used instead of outputData()/prepareOutput()
         *  if it is not part of pipeline, and will not modified data in pipeline
         * but inside ThreadedExectutor.generateParallelAccessor
         */
        const SparseMatrix<bool>& couplingMatrix() const
        {
            return myCouplingMatrix;
        }

        /**
         * \brief preparing work in serial mode
         */
        virtual void prepareInput() override final
        {
            /// prepare private properties like `std::vector<T>.resize(myInputData->itemCount());`
            /// therefore accessing item will not cause memory reallocation and items copying
            myCouplingMatrix.resize(myInputData->itemCount());
        }

        /**
         * \brief preparing work in serial mode, write report, move data into `myOutputData`
         */
        virtual void prepareOutput() override final
        {
            // todo: if called in pipeline explicitly, replace the filename the defined in Config
            // myCouplingMatrix.writeMatrixMarketFile("couplingMatrix.mm");
            myOutputData->emplace("myCouplingMatrix", std::move(myCouplingMatrix));
        }

        /**
         * \brief process data item in parallel without affecting other data items
         * @param index: index to get/set by item(index)/setItem(index, newDataItem)
         *  assuming an undirected graph `isCoupled(i, j) == isCoupled(j, i)`
         */
        virtual void processItem(const std::size_t index) override final
        {
            const std::size_t NItems = myInputData->itemCount();
            /// upper triangle for the matrix, also skip the pair (i, i)
            for (std::size_t j = index + 1; j < NItems; j++)
            {
                processItemPair(index, j);
            }
        }

        virtual void process() override final
        {
            const std::size_t NItems = myInputData->itemCount();
            for (std::size_t index = 0; index < NItems; index++)
            {
                processItem(index);
            }
        }

        /// @{ API for coupled data item processing
        virtual bool isCoupledPair(const ItemIndexType, const ItemIndexType) override
        {
            return false;
        }

        /// run in parallelism with the assistance of ParallelAccessor on coupled data
        virtual void processItemPair(const ItemIndexType i, const ItemIndexType j) override
        {
            if (myTargetProcessor->isCoupledPair(i, j)) // todo: get the filter function?
            {
                myCouplingMatrix[i].push_back(std::make_pair(j, true));
            }
        }
        /// @}

    };
} // namespace PPP
