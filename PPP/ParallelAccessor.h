#pragma once

#include "PreCompiled.h"
#include "Progressor.h"
#include "SparseMatrix.h"
#include "TypeDefs.h"


namespace std
{
    /// multi-dimension indexer based on `std::vector`
    typedef std::vector<PPP::ItemIndexType> indexer;

    /** Extension to c++ standard library to support std::hash on indexer type,
     * i.e. `std::vector<PPP::ItemIndexType>`.
     * NOTE: uniqueness is not guaranteed */
    template <> struct hash<indexer>
    {
        size_t operator()(const indexer& k) const
        {
            // Compute individual hash values for two data members and combine them using XOR and bit shifting
            size_t ret = hash<PPP::ItemIndexType>()(k[0]);
            for (size_t i = 1UL; i < k.size(); i++)
            {
                ret ^= (hash<PPP::ItemIndexType>()(k[i]) << i);
                ret = ret >> i;
            }
            return ret;
        }
    };

} // namespace std


namespace PPP
{
    /// \ingroup PPP

    /// abstract class for all accessor classes 1D and 2D
    template <class IndexerType> class AppExport Accessor
    {
    public:
        virtual const std::vector<IndexerType> next(const std::vector<IndexerType>) = 0;
        // virtual std::vector<IndexerType> next() = 0;
    };

    /**
     * ParallelAccessor implement a lock free parallel operation (write/modification),
     * on a multi-dimensional data structure, e.g. sparse matrix, when items are coupled together.
     * Currently, it supports binary operaton within a 2D data structure, e.g. matrix, with mutual exclusion;
     * triple operation involving 3 items in coupling is too complicated yet implemented.
     *
     * This ParallelAccess class implements the synchronous mode, while the dervied class AsynchronousDispatcher
     * implements asynchronous mode which is more efficient.
     * For the synchronous mode, the scheduler wait for all workers complete and schedule new task for all workers,
     * As only the schedular can allocate task (modify  `myRemainedItems`), there is no need for mutex.
     *
     * @param Filter: function of type `bool function(i, j)` to validate if item i and j are connected/coupled/adjacent
     * @param itemCount: the problem scale (total item number to processed)
     * @param blockCount: process blockCount operations in one batch for each thread call.
     * @param accessorCount: number of accessors (thread count in multi-thread executation mode) in parallel
     *
     */
    class ParallelAccessor : Accessor<std::vector<ItemIndexType>>
    {
    public:
        typedef std::vector<ItemIndexType> indexer;
        typedef std::vector<indexer> indexers;

    private:
        ParallelAccessor(const ParallelAccessor&) = delete;

    protected:
        const ItemIndexType myItemCount;
        const size_t myIndexDim;
        const ItemIndexType myWorkerCount;
        const ItemIndexType myBatchSize;
        std::shared_ptr<Progressor> myProgressor;

        /**
         * if filtering is time consuming, it should be done outside, but provide a sparse matrix
         */
        const bool hasFilter = false;
        /// is that item pair sequence make difference, op(i,j) has the same effect as op(j,i)
        const bool isDirected = false;
        bool isSparse = true;
        const bool isExclusive = true; // must be true for coupled data

        std::unordered_set<indexer> myRemainedItems;
        /// for synchronous mode only, the schedular wait for all workers completed then schedule next batch
        std::vector<indexers> myPreviouslyLocked;

        std::mutex myMutex;
        std::unordered_set<ItemIndexType> myLockedItems;

    public:
        ParallelAccessor(const ItemIndexType itemCount, const size_t dim, const ItemIndexType nWorker,
                         const ItemIndexType batchSize = 3)
                : myItemCount(itemCount)
                , myIndexDim(dim)
                , myWorkerCount(nWorker)
                , myBatchSize(batchSize)

        {
            /// check the setup is correct and reasonable
            if (dim > 2)
            {
                std::cout << "dimesion greater than 2 is not implemented";
            }
            myLockedItems.reserve(myWorkerCount * myBatchSize * dim);
        }

        virtual ~ParallelAccessor()
        {
            myProgressor->finish();
        }

        void finish()
        {
            myProgressor->finish();
        }

        /// this matrix can be built by filtering  isPairCoupled()
        void setCouplingMatrix(const AdjacencyMatrixType& AMax)
        {
            for (ItemIndexType i = 0; i < AMax.rowCount(); i++)
            {
                for (const auto& it : AMax[i])
                {
                    indexer ind = {i, it.first};
                    myRemainedItems.emplace(std::move(ind));
                }
            }
            /// consider: this method should moved to start(), but user may forget to call start()
            myProgressor = std::make_shared<Progressor>(remainedOperationSize());
        }

        inline size_t remainedOperationSize() const
        {
            return myRemainedItems.size();
        }

        size_t indexDimension() const
        {
            return myIndexDim;
        }

        /// has barrier to wait all threads to complete, before next
        virtual bool synchronized() const
        {
            return true;
        }

        /**
         * get (at most blockCount) indexers for a single thread, std::mutex protected
         * using AsynchronousDispatcher
         * */
        virtual const indexers next(const indexers prevLocked = indexers()) override
        {
            std::lock_guard<std::mutex> grard(myMutex); // exception safer

            unlock(prevLocked); // in case forgeting to unlock before run this function
            indexers tmp;
            auto it = myRemainedItems.cbegin();
            const auto& iend = myRemainedItems.cend();
            for (ItemIndexType i = 0; i < myBatchSize; i++)
            {
                while (it != iend && isLocked(*it)) // test it is not ending first!
                {
                    it++;
                }
                if (it != iend)
                {
                    tmp.push_back(*it);
                    lockItem(*it);
                    it++;
                }
            }
            return tmp;
        }

        /// generator indexer/accessor for all accessing workers
        /// must be called in main thread, there is no lock to protect
        /// used in barrier mode (block until all workers finish)
        /// unlockAll() before calling nextAll()
        /// check size()==0, there is no more item to be processed
        const std::vector<indexers> nextAll()
        {
            // todo: fill each thread, instead of fill the first to the full block
            unlockAll(); // erase will reset iterator of remainedItems
            myProgressor->remain(remainedOperationSize());

            std::vector<indexers> indexerSets;
            // fill with next available but not in lockedSet

            for (ItemIndexType t = 0; t < myWorkerCount; t++)
            {
                auto tmp = next();
                if (tmp.size() > 0)
                {
                    indexerSets.push_back(std::move(tmp));
                }
            }
            myPreviouslyLocked = indexerSets;
            return indexerSets;
        }

    protected:
        inline bool isLocked(const indexer& ind)
        {
            for (const auto& i : ind)
            {
                if (myLockedItems.find(i) != myLockedItems.end())
                    return true;
            }
            return false;
        }

        inline void lockItem(const indexer& inds)
        {
            for (const auto& i : inds)
                myLockedItems.insert(i);
        }

        /// no harm if call twice?, should be called after mutex lock
        void unlock(const indexers& prevLocked)
        {
            for (const auto& ind : prevLocked)
            {
                for (const auto& i : ind)
                    myLockedItems.erase(i);
                myRemainedItems.erase(ind);
            }
        }

        inline void unlockAll()
        {
            myLockedItems.clear();
            for (auto& ids : myPreviouslyLocked)
                for (auto& id : ids)
                    myRemainedItems.erase(id);
        }
    };
} // namespace PPP
