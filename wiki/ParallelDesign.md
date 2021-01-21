# Parallel Design

## Split-Process-Merge pattern

The design of  **parallel-preprocessor** aims to enable researcher to focus on algorithms in his/her domain, no matter whether it is running on GPU or super-computer. This Split-Process-Merge pattern design share the idea with **MapReduce** programming model developed by Google. 

**MapReduce** is a simplified distributed programming model and an efficient task scheduling model for parallel operations on large-scale data sets (greater than 1TB).  The idea of ​​the MapReduce mode is to decompose the problem to be executed into Map (mapping) and Reduce (simplification). First, the data is cut into irrelevant (independent) blocks by the `Map` program, and then distributed (scheduling) to a large number of computers/threads for parallel computation. Then the result is aggregated and output through the `Reduce` program.

The design of **parallel-preprocessor** is based on the fact, a large data set, such as an assembly of a whole fusion reactor CAD model, can be viewed as a cluster of parts (part: a single self-containing 3D shape that can be design and saved). Each part is an item on which computation may be worth of running in a thread independently. For example, calculating geometrical meta data like volume and center of mass, can be run in parallel without modifying the nearby parts. Developer will only needs to write a function `processItem(index)` for each part. Multi-thread parallelization is conducted at the part level.

In case of computation on one item that may alter the state of other items, which is also called coupled operation, e.g. imprinting a part will alter the data structure of nearby parts in contact, parallel computation is still possible given the fact a part is only in contact with a very small portion of total parts in the assembly. It is possible to parallelize computation for the pair of item `(i, j)`,  and item `(x, y)` at the same time,  if both pairs do not affect each other pair, see also [Benchmarking.md](Benchmarking.md).


There are some tasks like memory allocation is not allowed to run in parallel, then there are two functions `preprocess()` and `postprocess()` will do prepare/split and cleanup/merge tasks in the serial mode, the Merge stage is corresponding to the Reduce concept in MapReduce programming model. 


## Workflow Topology

Currently only linear pipeline (single-way linear) is supported, more complicated graph dataflow as in `Apache NIFI`,  `Tensorflow`, has not been implemented. 

https://nifi.apache.org/docs/nifi-docs/html/user-guide.html#User_Interface

`Cpp-Taskflow` library is by far a faster, more expressive, and easier for drop-in integration for the single-way linear task programming.

Libraries such as `OpenMP Tasking` and `Intel TBB FlowGraph` in handling complex parallel workloads. TensorFlow has the concept of graph simplication. 


> OpenCV 4.x introduces very different programming model `G-API`, where you define pipeline of operations to be performed first, and then apply this pipeline to some actual data. In other words, whenever you call OpenCV G-API function, execution is deferred (lazily-evaluated), and deferred operation result is being returned instead of actual computation result. This concept might sound very similar to [C++ 20 Range V3](https://github.com/ericniebler/range-v3).  Source: https://blog.conan.io/2018/12/19/New-OpenCV-release-4-0.html


## Parallel Executor

Currently, only CPU device is supported, although GPU acceleration will be considered in the future.  It is expected the Processor should has a characteristics of  ` enum DevicePreference {CPU, GPU}`

+ multiple threading (shared memory)
  TBB `task_group` is used as the thread pool, but can be switched to a single-header thread pool implementation. Synchronous multi-threading (wait until all threads complete tasks) and asynchronous dispatcher (now the default) have been implemented.
  ![asynchronous dispatcher can fully utilise multi-core CPU](./PPP_asyn_multiple_threading.png)

+ MPI (distributive, NUMA)
  This is useful for distributive meshing, since memory capacity on a single multi-core node may be not sufficient to mesh large assemblies.

+ GPU (heterogenous)
  This is the preferred for simple data types such as image processing AI. 

## Concurrent data access

### STL vector with parallel accessor without lock
The main thread allocate the `std::vector<>` with enough size by `resize(capacity, value)`, to avoid resize during parallel operation by worker threads.
`std::vector<>` can be read and write its own own range of a specific worker thread, without lock. Access controller is needed to make sure read and write to other thread's memory zone is valid.

`std::vector::resize(capacity, value)` will not only reserve memory but also set a initial value, if not provided then default value.
`std::vector::reserve(capacity)` reserver the contiguous memory region, but it does not really increase vector `size()`. 

 currently a template alias is used, for potential replacement for distributive  MPI parallel 
  `template <class T> using VectorType = std::vector<T>;`

### Concurrent data structure
There are high-level concurent data structure with lock/synchronization underneath

<https://github.com/FEniCS/dolfin/tree/master/dolfin/la>

TBB has some concurrent data structure, C++17 standard c++ parallel library seems based on TBB.

[HPX](https://github.com/STEllAR-GROUP/hpx) is a library that unifying the API for multi-threading and distributive parallelization.


## Distributive parallel design

The master node do the split (a best strategy for geometry decomposition is worth of research), feeding each worker node with a collection of shapes and meta info (json) for each shape. The master will build the global topology (graph, boundbox tree), which will be shared by all nodes. Each worker works on the split dataset independently like shape checking and meshing  while it sends some meta information back to the master to build global information data structure.  


## GPU parallel design

GPU parallel is under investigation, GPU offloading from OpenMP may also be considered, reuse the current ThreadingExecutor. 

Device and technology selection
+ OpenCL/SYCL, 
+ NVIDIA's CUDA/NVCC,  
+ ADM's ROCm/HIP,  
+ Intel's oneAPI/DPC++

single source is preferred, such as SYCL. 
fall back to CPU:  it is supported by all except CUDA.
How to support more hardware platform as possible? SYCL,
multiple GPU support, CUDA is leading is this field.


### Coding

1. GPU may be achieved by third-party library implicitly, in that case, only one thread per GPU is used
    For example, OpenMP may be offloaded to GPU, Nvidia has stdc++ lib to offload some operation to GPU.

2. write the GPU kernel source and the `GpuExecutor` will schedule the work.


