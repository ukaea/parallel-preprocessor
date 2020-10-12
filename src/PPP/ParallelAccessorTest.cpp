//#pragma once

// for visual C++ to support not, and, or keywords
#include <iso646.h>

#include "PPP/AsynchronousDispatcher.h"
#include "PPP/ParallelAccessor.h"
#include "PPP/Processor.h"
#include "PPP/SparseMatrix.h"

#include "PPP/ProcessorTemplate.h"
#include "PPP/ThreadPoolExecutor.h"


namespace PPP
{
    /// manually build up a pipeline, instead of from config file
    bool test_ThreadPoolExecutor(bool isCoupled, bool asynchronous = false)
    {
        const int Nitems = 1000;
        size_t avgNumberOfCoupledItems = 10;
        // auto A = AdjacencyMatrixType::random(Nitems, avgNumberOfCoupledItems * Nitems);
        // A.writeMatrixMarketFile("random_matrix.mm");
        auto A = AdjacencyMatrixType::readMatrixMarketFile("../data/sampleCoupledMatrix.mm");

        auto data = std::make_shared<DataObject>();
        data->setItemCount(Nitems);
        std::vector<size_t> myData;
        myData.resize(Nitems, 0); // set all value as 0
        // myData is not moved into data object, it is ref in lambda function

        /// it also tests the ProcessorTemplate
        auto p = std::make_shared<ProcessorTemplate<Processor, bool>>(
            [&](ItemIndexType i) {
                myData[i] += 1; // it must be calc once and only once
                return true;
            },
            "myResultData");
        p->setInputData(data);
        // p->setPostprocessor([&](){          });  // extra pre/post-processing is not needed

        auto nCores = std::thread::hardware_concurrency();
        auto batchSize = 2;

        /// NOTE: tbb::task_group does not support std::make_shared<>() on clang
        auto threadPool = std::shared_ptr<ThreadPoolType>(new ThreadPoolType());
        auto te = std::make_shared<ThreadPoolExecutor>(p, nCores, threadPool);
        if (isCoupled)
        {
            size_t dim = 2;
            std::shared_ptr<ParallelAccessor> pa;
            if (asynchronous)
            {
                pa = std::make_shared<AsynchronousDispatcher>(p->inputData()->itemCount(), dim, nCores, batchSize);
            }
            else
            {
                pa = std::make_shared<ParallelAccessor>(p->inputData()->itemCount(), dim, nCores, batchSize);
            }

            pa->setCouplingMatrix(A);
            te->setParallelAccessor(pa); // if this line is commented out

            Config cfg = {{"indexPattern", IndexPattern::SparseMatrix}, {"coupled", true}};
            p->setCharactoristics(cfg);
            p->setCoupledItemProcessor([&A](ItemIndexType i, ItemIndexType j) {
                // remove item from A,  A[i] is an unique pointer to std::vector
                A.removeAt(i, j);
                return true;
            });
        }
        te->process();

        // get data back and check
        bool result;
        if (isCoupled and asynchronous)
        {
            // total size of A should be zero
            result = (A.elementSize() == 0);
            // A.writeMatrixMarketFile("test_matrix.mm");  // for debug only
            if (!result)
            {
                throw std::runtime_error(std::string("Failed: not all items are processed for coupled data\n"));
            }
            else
            {
                std::cout << "Processing on coupled data seems correct\n";
            }
        }
        else
        {
            bool isAllDone = std::all_of(myData.cbegin(), myData.cend(), [](size_t it) { return it != 0; });
            std::cout << std::boolalpha << "has set zero all done = " << isAllDone << std::endl;

            auto outputData = p->outputData();
            std::cout << "contains(myTestData) = " << outputData->contains("myResultData") << std::endl;
            auto results = outputData->get<VectorType<bool>>("myResultData");
            std::cout << "results->size() = " << results->size() << std::endl;
            result = std::all_of(results->cbegin(), results->cend(), [](bool it) { return it == true; });
        }
        return result;
    }

} // namespace PPP

/* test as a normal program, debugging */
int main()
{
    PPP::test_ThreadPoolExecutor(false);
    PPP::test_ThreadPoolExecutor(true);
    PPP::test_ThreadPoolExecutor(true, true);
}
