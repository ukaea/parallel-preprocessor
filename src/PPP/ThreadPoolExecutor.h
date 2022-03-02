#pragma once

#include "AsynchronousDispatcher.h"
#include "CouplingMatrixBuilder.h"
#include "Executor.h"
#include "ParallelAccessor.h"

#define PPP_HAS_TBB 1
#if PPP_HAS_TBB
#include "tbb/task_group.h"
typedef tbb::task_group ThreadPoolType;
#else
#include "third-party/ThreadPool.h"
typedef ThreadPool ThreadPoolType;
#endif


namespace PPP
{
    /// \ingroup PPP
    /**
     * It is essential to use thread pool to start parallel task
     * otherwise, thread creation (a time-consuming operation in kernel) slow down the process
     * */
    class AppExport ThreadPoolExecutor : public Executor
    {
    protected:
        std::shared_ptr<ParallelAccessor> myParallelAccessor;
        std::shared_ptr<ThreadPoolType> myThreadPool;

    public:
        ThreadPoolExecutor(std::shared_ptr<Processor> gp, unsigned int threadCount,
                           std::shared_ptr<ThreadPoolType> threadPool)
                : Executor(gp, threadCount)
                , myThreadPool(threadPool)
        {
        }
        virtual ~ThreadPoolExecutor() = default;

        void setParallelAccessor(const std::shared_ptr<ParallelAccessor>& pa)
        {
            myParallelAccessor = pa;
        }

        /**
         *  this algorithm needs only the length of data
         */
        // @param applier_t: a boolean operation function typdef applier_t
        // @param filter: a shape filter const std::function<bool (const TopoDS_Shape&)>& filter
        // @param reporter:
        //
        virtual void process() override
        {
            myProcessor->prepareInput();
            const size_t NItems = myProcessor->inputData()->itemCount();
            if (NItems == 0)
            {
                LOG_F(ERROR, "item count is zero in dataObject, check");
            }

            auto ip = myProcessor->attribute<IndexPattern>("indexPattern");
            auto dim = myProcessor->attribute<size_t>("indexDimension");
            // VLOG_F(LOGLEVEL_DEBUG, "debug info: indexPattern = %d", ip);
            if (ip == IndexPattern::PartitionIdVector)
            {
                VLOG_F(LOGLEVEL_DEBUG, " processing item in parallel using partitioned Id");
                runParallelOnPartitionedData();
            }
            else if (myParallelAccessor || myProcessor->isCoupledOperation())
            {
                VLOG_F(LOGLEVEL_DEBUG, "parallel processing on coupled data for index dimension = %lu", dim);
                if (!myParallelAccessor)
                {
                    generateParallelAccessor(dim); // ip == FilteredMatrix, SparseMatrix
                }

                if (myParallelAccessor->synchronized())
                {
                    runParallelOnCoupledData(myParallelAccessor);
                }
                else
                {
                    runAsynchronouslyOnCoupledData(myParallelAccessor);
                }
            }
            else
            {
                runParallelInBlock(NItems);
                /// NOTE: asyn mode could be more efficient, if processItem() time is not constant
            }

            myProcessor->prepareOutput();
        }

    private:
        void generateParallelAccessor(const size_t dim)
        {
            // AsynchronousDispatcher is the preferred one
            // since the default ParallelAccessor need synchronization (lead to low cpu utilization ratio)
            std::shared_ptr<ParallelAccessor> pa =
                std::make_shared<AsynchronousDispatcher>(myProcessor->inputData()->itemCount(), dim, myWorkerCount, 2);
            if (myProcessor->inputData()->contains("myCouplingMatrix")) // SparseMatrix
            {                                                           // first test if `myCouplingMatrix` exists
                std::shared_ptr<const AdjacencyMatrixType> m =
                    myProcessor->inputData()->getConst<AdjacencyMatrixType>("myCouplingMatrix");
                pa->setCouplingMatrix(*m);
                // for debug propose, write into file
                m->writeMatrixMarketFile(myProcessor->generateDumpName("couplingMatrix.mm", {}));
            }
            else
            {
                // run filter and generate a coupling matrix/ potential overlapping matrix
                auto b = std::make_shared<CouplingMatrixBuilder>();
                // it is not possible to non-const getInputData(),
                // assume output is share_ptr of the input data, done in the target processor's preparaInput()
                b->setInputData(myProcessor->outputData());
                b->setTargetProcessor(myProcessor);
                b->prepareInput();
                b->process(); // it is serial model, but it is possible to run in parallel,
                // as the coupling-test is a cheap op, multi-threading may not be worth of doing.
                // if it is really needed, then build the CouplingMatrixBuilder into the pipeline
                // ThreadPoolExecutor te(b, myWorkerCount, myThreadPool);
                // te.process();  // this will move/empty the couplingMatrix()

                // note: preparaOutput is not called, if called, "myCouplingMatrix" will be inserted into data
                auto cmat = b->couplingMatrix();
                pa->setCouplingMatrix(cmat);
                cmat.writeMatrixMarketFile(myProcessor->generateDumpName("myFilteredMatrix.mm", {})); // debugging
            }
            myParallelAccessor = pa;
        }


        void runParallelInBlock(const size_t NItems)
        {
            // decide on the processor's IndexPattern
            // for dense matrix, an linear pattern, split into equally spaced row groups
            // for Triangle, 2D traverse, AsynchronousDispatcher is the best choice no matter of coupling
            size_t itemPerThread = size_t(NItems / myWorkerCount);

            // in case: itemPerThread  = 0 when Nitems < myWorkerCount
            if (NItems > 0 && NItems % myWorkerCount)
            {
                itemPerThread += 1;
            }

            for (unsigned int t = 0; t < myWorkerCount; t++)
            {
                size_t i_start = t * itemPerThread;
                size_t i_end = (t + 1) * itemPerThread;
                // if NItems < myWorkerCount and itemPerThread is set to 1, check range
                if (i_start >= NItems)
                {
                    break;
                }
                // the last thread process all the remained, as i_end can be smaller than NItems
                if (i_end > NItems)
                {
                    i_end = NItems;
                }
                // it is important to capture the index by value
                myThreadPool->run([&, i_start = i_start, i_end = i_end]() {
                    for (size_t i = i_start; i < i_end && i < NItems; i++)
                    {
                        if (this->myReporter)
                            this->myReporter(i);
                        this->myProcessor->processItem(i);
                    }
                });
            }
            myThreadPool->wait(); // tbb::task_group API, otherwise, the main thread will run prepareOutput()
        }

        /**
         * allocate item to thread according to partitionIDs
         * */
        void runParallelOnPartitionedData()
        {
            size_t itemCount = 0;
            ItemIndexType NItems = myProcessor->inputData()->itemCount();
            std::shared_ptr<const std::vector<ItemIndexType>> myPartitionIds =
                myProcessor->inputData()->getConst<std::vector<ItemIndexType>>("myPartitionIds");
            for (unsigned int t = 0; t < myWorkerCount; t++)
            {
                std::vector<ItemIndexType> ids;
                for (ItemIndexType i = 0; i < NItems; i++)
                {
                    if ((*myPartitionIds)[i] == t)
                        ids.push_back(i);
                }
                itemCount += ids.size();
                // it is fine to capture ids by reference, but t must not!
                if (ids.size())
                {
                    myThreadPool->run([=]() {
                        for (auto index : ids) // must pass ids as value into this lambda function
                        {
                            // LOG_F(INFO, "partition id %d", index);
                            myProcessor->processItem(index);
                        }
                    });
                }
            }
            assert(itemCount == NItems);
            myThreadPool->wait();
        }

        /**
         * build a adjacencyMatrix to identify the items have mutual influence before run this function
         * */
        void runParallelOnCoupledData(std::shared_ptr<ParallelAccessor> pa)
        {
            const auto dim = pa->indexDimension();
            int i_loop = 0;
            auto ids = pa->nextAll();
            while (ids.size() > 0) // after wait(), nothing is locked as nextAll() will unclockAll()
            {
                // VLOG_F(LOGLEVEL_DEBUG, "processing %d batch in synchronous parallel on couppled data", i_loop);
                for (unsigned int workerId = 0; workerId < myWorkerCount && workerId < ids.size(); workerId++)
                {
                    /// NOTE: it is fine to capture ids by reference, but workerId must be passed by value copy
                    myThreadPool->run([&, t = workerId]() {
                        for (auto indexer : ids[t]) // if there is no indexer, this worker will do nothing
                        {
                            if (dim == 2)
                                myProcessor->processItemPair(indexer[0], indexer[1]);
                            else
                                myProcessor->processItem(indexer[0]);
                        }
                    });
                }
                myThreadPool->wait(); /// tbb::task_group API to synchronize all tasks
                ids = pa->nextAll();
                i_loop += 1;
            }
        }

        void runAsynchronouslyOnCoupledData(std::shared_ptr<ParallelAccessor> pa)
        {
            const auto dim = pa->indexDimension();
            VLOG_F(LOGLEVEL_DEBUG, "processing items Asynchronously"); // loglevel 2 for debug info

            for (unsigned int t = 0; t < myWorkerCount; t++)
            {
                myThreadPool->run([&]() {
                    auto ids = pa->next(); /// protected by std::mutex
                    while (ids.size() > 0)
                    {
                        for (auto indexer : ids) // if there is no indexer, this worker will do nothing
                        {
                            if (dim == 2)
                                myProcessor->processItemPair(indexer[0], indexer[1]);
                            else
                                myProcessor->processItem(indexer[0]);
                            // loglevel 1 means PROGRESS,  disable this info since progress bar is available
                            // VLOG_F(PROGRESS, "processing pair (%lu, %lu) asynchronously", indexer[0], indexer[1]);
                        }
                        ids = pa->next(ids);
                    }
                });
            }
            myThreadPool->wait(); // wait until all operation completed, then post-processing
            // pa->finish();
        }
    };

} // namespace PPP
