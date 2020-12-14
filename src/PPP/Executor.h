#pragma once

#include "Processor.h"

namespace PPP
{

    /// \ingroup PPP
    /**
     * base class for all executors: Serial, ThreadPool, Distributive, GPU, etc
     * note: C++20 has Executor API
     */
    class AppExport Executor
    {
    protected:
        std::shared_ptr<Processor> myProcessor;

        /// not in use, maybe remove later, using progressor instead
        typedef std::function<void(ItemIndexType)> ReportFunc;
        const std::function<void(ItemIndexType)> myReporter = nullptr;

        unsigned int myWorkerCount;
        bool isSynchronous;

    public:
        Executor(std::shared_ptr<Processor> gp, unsigned int workerCount = 1)
                : myProcessor(gp)
                , myWorkerCount(workerCount)
        //, myReporter([](ItemIndexType i) { VLOG_F(LOGLEVEL_PROGRESS, "process the item#%lu\n", i); })
        {
        }
        virtual ~Executor() = default;

        /**
         *  prepareInput(), process all items, prepareOutput()
         */
        virtual void process() = 0;

        inline unsigned int workerCount() const
        {
            return myWorkerCount;
        }

        /// need wait() complete until processs the next item
        inline bool synchronous() const
        {
            return isSynchronous;
        }
    };

    /// Executor implement serial(single thread) execution
    class SequentialExecutor : public Executor
    {

    public:
        using Executor::Executor;
        virtual ~SequentialExecutor() = default;

        virtual void process() override
        {
            myProcessor->prepareInput();

            const size_t NItems = myProcessor->inputData()->itemCount();
            if (NItems == 0)
            {
                LOG_F(ERROR, "no items are loaded for processing, check reading setup");
            }

            auto ip = myProcessor->attribute<IndexPattern>("indexPattern");
            auto dim = myProcessor->attribute<size_t>("indexDimension");
            if (myProcessor->isCoupledOperation())
            {
                LOG_F(ERROR, "coupled operation is not implemented for sequential executor");
            }
            else
            {
                for (size_t i = 0; i < NItems; i++)
                {
                    if (myReporter)
                        myReporter(i); // todo: replaced  by operator proxy
                    myProcessor->processItem(i);
                }
            }

            myProcessor->prepareOutput();
        }
    };

} // namespace PPP
