#pragma once

#include "Processor.h"

namespace PPP
{

    /// \ingroup PPP
    /**
     * \brief template + lambda to build a new processor inplace without declare a new processor type.
     *  this class will not be wrapped for Python, only used in C++, see exmaple in ParallelAccessorTest.cpp
     * `myResultData` is the place to save result data
     *
     * @param ProcessorType a concrete process class it inherits from, in order to share data
     * @param ResultItemType  processed result of VectorType<ResultItemType>, default to bool,
     *                    will be saved to `myOutputData` a vector<ResultItemType>
     *
     * TODO: add more template parameter to make a constructor() without parameter, not working
     *       lambda can capture the processor instance pointer or reference `[&p] () {}`
     *
     *       This template must derive from a concrete type Processor,
     *        not from a template parameter type "ProcessorType"
     */
    template <typename ProcessorType = Processor, typename ResultItemType = bool>
    class ProcessorTemplate : public Processor
    {
        /// NOTE: type system is not needed,  since it is instantiated inplace by lambda function
        // TYPESYSTEM_HEADER();

    public:
        /// function type for pre and post processors
        typedef std::function<void()> PreFuncType;
        /// function type for per item process, return a ResultType
        typedef std::function<ResultItemType(const ItemIndexType)> ItemFuncType;
        ///
        typedef std::function<ResultItemType(const ItemIndexType, const ItemIndexType)> ItemPairFuncType;

    private:
        VectorType<ResultItemType> myResultData;
        std::string myResultName;

        ItemFuncType myItemProcessor;
        ItemPairFuncType myCoupledItemProcessor;
        PreFuncType myPreprocessor;
        PreFuncType myPostprocessor;

    public:
        ProcessorTemplate(ItemFuncType&& op, const std::string name = "myProcessResult",
                          ItemPairFuncType _CoupledItemPairFunc = nullptr, PreFuncType _PreFunc = nullptr,
                          PreFuncType _PostFunc = nullptr)
                : myItemProcessor(std::move(op))
                , myResultName(name)
                , myCoupledItemProcessor(_CoupledItemPairFunc)
                , myPreprocessor(_PreFunc)
                , myPostprocessor(_PostFunc)
        {
        }
        ~ProcessorTemplate() = default;

        /* set the result data name for serialization if not given in constructor */
        void setResultName(const std::string name)
        {
            myResultName = name;
        }


        void setPreprocessor(PreFuncType&& f)
        {
            myPreprocessor = std::move(f);
        }
        void setPostprocessor(PreFuncType&& f)
        {
            myPostprocessor = std::move(f);
        }

        /// ProcessorTemplate class
        void setCharactoristics(const Config cfg)
        {
            myCharacteristics = cfg;
        }

        /// data operation is coupled, if setting this functor
        void setCoupledItemProcessor(ItemPairFuncType&& f)
        {
            myCharacteristics["coupled"] = true;
            myCoupledItemProcessor = std::move(f);
        }

        /**
         * \brief preparing work in serial mode, like reserve memory for data container
         */
        virtual void prepareInput() override final
        {
            if (myItemProcessor == nullptr)
            {
                throw std::runtime_error("function pointer to itemProcessor must NOT be nullptr or empty");
            }

            /// prepare private properties like `VectorType<T>.resize(myInputData->itemCount());`
            /// therefore accessing item will not cause memory reallocation and items copying
            /// However, it is possible inputData are not set by setInputData() in non-pipeline mode
            if (myInputData)
                myResultData.resize(myInputData->itemCount());
            if (myPreprocessor != nullptr)
                myPreprocessor();
        }

        /**
         * \brief preparing work in serial mode, write report, move data into `myOutputData`
         */
        virtual void prepareOutput() override final
        {
            if (myPostprocessor != nullptr)
                myPostprocessor();
            myOutputData->emplace(myResultName, std::move(myResultData));
        }

        /**
         * \brief process data item in parallel without affecting other data items
         * @param index: index to get/set item by `item(index)/setItem(index, newDataItem)`
         */
        virtual void processItem(const ItemIndexType index) override final
        {
            myResultData[index] = myItemProcessor(index);
        }

        /**
         * \brief process item pair by calling the function pointer `myCoupledItemProcessor(i,j)`
         * @param i: index to the first item in the coupled pair
         * @param j: index to the other item in the coupled pair
         */
        virtual void processItemPair(const std::size_t i, const std::size_t j) override final
        {
            if (myCoupledItemProcessor != nullptr)
                myCoupledItemProcessor(i, j);
        }
    };
} // namespace PPP
