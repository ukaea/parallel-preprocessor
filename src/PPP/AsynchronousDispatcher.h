#pragma once
#include "ParallelAccessor.h"

namespace PPP
{
    /// \ingroup PPP
    /**
     * Parallel access item pairs in asynchronous without waiting other workers to complete.
     *
     * For asynchronous operation, each worker can unlock items it completed;
     * there is no needs to wait for other workers, just lock item from `myRemainedItems` and carry on work;
     * the data structure `myRemainedItems` is protected by mutex.
     *
     * single producer (myRemainedItems) and multiple consumers queue pattern (by mutex locking),
     * There is no concept of scheduler which manages the queue,
     * but any worker found the queue is empty will produce (fill the queue).
     *
     * NOTE: myRemainedItems.size()==0 does not means all items have been processed completely,
     * some may be locked by other workers, wait() all workers are still needed by the task_group
     * */
    class AppExport AsynchronousDispatcher : public ParallelAccessor
    {
    private:
        std::queue<ParallelAccessor::indexer> myQueue;

    public:
        using ParallelAccessor::ParallelAccessor;

        /// no need to wait other thread to complete
        virtual bool synchronized() const override final
        {
            return false;
        }

        /// call in main thread to produce product queue, so it is not mutex protected
        /// if you forget to call this method, next() will try to produce in locked context
        void prepare()
        {
            const ItemIndexType nProducts = myItemCount * myBatchSize * myWorkerCount;
            produce(nProducts);
        }

        /// return a sequence of indexer (multiple dim indexing)
        /// automatically unlock previous locked indexers(if any)
        virtual const indexers next(const indexers prevLocked = indexers()) override final
        {
            std::vector<indexer> a;
            try
            {
                /// NOTE: std::scoped_lock in C++17 can lock 2 locks without deadlock
                std::lock_guard<std::mutex> lock(myMutex); // this lock is exception-safe
                if (prevLocked.size() > 0)
                {
                    unlock(prevLocked); /// move this out, make this function const?
                    myProgressor->remain(remainedOperationSize());
                }

                if (myQueue.size() == 0)
                {
                    produce(myItemCount * myBatchSize * myWorkerCount);
                }
                a = consume(myBatchSize);
            }
            catch (const std::exception& e)
            {
                std::cout << e.what() << " error happened when getting next indexers\n";
            }
            return a;
        }

    private:
        const indexers consume(const ItemIndexType nProducts)
        {
            std::vector<indexer> a;
            for (ItemIndexType i = 0; i < nProducts; i++)
            {
                if (myQueue.size() > 0)
                {
                    a.push_back(myQueue.front());
                    myQueue.pop();
                }
            }
            return a;
        }

        void produce(const ItemIndexType nProducts)
        {
            auto it = myRemainedItems.cbegin();
            const auto& iend = myRemainedItems.cend();
            for (ItemIndexType i = 0; i < nProducts; i++)
            {
                while (it != iend && isLocked(*it))
                {
                    it++;
                }
                if (it != iend)
                {
                    myQueue.push(*it);
                    lockItem(*it);
                    it++;
                }
            }
        }
    };
} // namespace PPP
