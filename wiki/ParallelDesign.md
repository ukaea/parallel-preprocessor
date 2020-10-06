# Parallel design

### Workflow Topology

Currenlty only linear pipeline (single-way linear) is supported. 

Apache NIFI,  Tensorflow support more complicated graph dataflow. 

https://nifi.apache.org/docs/nifi-docs/html/user-guide.html#User_Interface

`Cpp-Taskflow` library is by far a faster, more expressive, and easier for drop-in integration for the single-way linear task programming 

## Executor
Currently, only CPU device is supported, althoug GPU acceleraton will be considered in the future.  It is expected the Processor should has a characteristics of  ` enum DevicePreference {CPU, GPU}`

+ multiple threading (shared memory), TBB task_group is used, but can be switched to a single-header thread pool implementation.
  synchronous multi-threading (wait until all threads complete tasks) and asynchronous dispatcher (now default) have been implemented.
  ![asynchronous dispatcher can fully utilise multi-core CPU](./PPP_asyn_multiple_threading.png)

+ MPI (distributive, NUMA)
  This is useful for distributive meshing, since memory capacity on a single multi-core node may be not sufficient to mesh large assemblies.
  [HPX](https://github.com/STEllAR-GROUP/hpx) is a library that unitify the API for multi-threading and distributve parallelization.
  libraries such as `OpenMP Tasking` and `Intel TBB FlowGraph` in handling complex parallel workloads. TensorFlow has the concept of graph simplication. 

### Thread Pool Executor
+ currently, intel TBB `task_group`, internally it could be a threadpool, is used
+ OpenCASCADE has `ThreadPool` class, can be a choice
  <https://www.opencascade.com/doc/occt-7.4.0/refman/html/class_o_s_d___thread_pool.html>
+ A simple C++11 Thread Pool implementation, license: Zlib
  <https://github.com/progschj/ThreadPool>

The class name should be identical to Python's `concurrent.futures` module like `ThreadPool`  `ProcessPoolExecutor`
https://docs.python.org/3/library/concurrent.futures.html

### Process Pool Executor

Python's `ProcessPoolExecutor` implements similar API as  `ProcessPoolExecutor`, but all processes are still on the localhost (not distributive). 
MPI distributive parallel is also on Process level, does not share the memory address.

### Parallel computation on GPU

GPU parallel is under investigation, it is expected single-source, which can fall back to CPU parallel.
GPU offloading may also be considered.

For some simple data type like image and text, it is expected computation can be done on GPU
1. GPU may be achieved by third-party library implicitly, in that case, only one thread per GPU is used
  For example, OpenMP may be offloaded to GPU, Nvidia has stdc++ lib to offload some operaton to GPU.

2. write the GPU kernel source and the `GpuExecutor` will schedule the work.

```cpp
    enum class DataType
    {
        Any = 0,  /// unknown data type
        Text,
        Image,
        Audio,
        Video,
        Geometry
    };
```
SYCL is the single source OpenCL, to write new processor in C++.

## Concurrent data access

### STL vector with parallel accessor without lock
The main thread allocate the `std::vector<>` with enough size by `resize(capacity, value)`, to avoid resize during parallel operation by worker threads.
`std::vector<>` can be read and write its own own range of a specific worker thread, without lock. Access controller is needed to make sure read and write to other thread's memory zone is valid.

`std::vector::resize(capacity, value)` will not only reserve memory but also set a initial value, if not provided then default value.
`std::vector::reserve(capacity)` reserver the contiguous memory region, but it does not really increase vector `size()`. 

 currently a template alias is used, for potential replacement for distributive  MPI parallel 
  `template <class T> using VectorType = std::vector<T>;`

### Concurrent data structure with lock underneath
<https://github.com/FEniCS/dolfin/tree/master/dolfin/la>
TBB has some concurrent data structure, C++17 standard c++ parallel library seems based on TBB.


## Distributive parallel design

The master node do the split, feeding each worker node with a collection of shapes and meta info (json) for each shape. The master will build the topology (graph, boundbox tree), which will be shared by all nodes.
Each worker works on the split dataset indepedently like shape checking and meshing  while it sends some meta information back to the master to build global information data structure.  

PPP may also write the case input files, some part recongination, boundary condition setup,  OpenMC/FEM/CFD solver are considered in this pattern. 
Natural part shared face during assembly splitting is used for `interprocessor` boundary conditions. The master node is responsible to doe boundary updating.  PreCICE may be used for boundary update. 

http://www.libgeodecomp.org/
