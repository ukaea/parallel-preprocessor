
## Automate CAD-CAE design workflow

### Existing and future CAD-CAE design workflow

![Existing CAD and CAE design-validation workflow](https://github.com/qingfengxia/CAE_pipeline/raw/master/images/current_CAE_process.png)

If an automated workflow can be achieved, just like continuous integration (CI) in software engineering, then the impact of any local design change can be evaluated promptly on the whole design. This automated workflow is crucial for the design of a large apparatus and for collaborations where multiple teams and organizations are involved.

Once design automation has been achieved, intelligent engineering design can be realized.
![diagram of future intelligent engineering design](https://github.com/qingfengxia/CAE_pipeline/raw/master/images/intelligent_engineering_design.png)

More details and relevant projects to enable automated and intelligent engineering design can be found at <https://github.com/qingfengxia/CAE_pipeline>.

### What is geometry preprocessing in the context of exascale CAE?

1. Geometry from design office, i.e. CAD files served for manufacturing, is full of manufacturing details. It must be simplified (**defeaturing**) and fixed, in order to be used in design validation (CAE) via physical and engineering simulation.

2. Continuous assembly validation (of digital mock-ups) such as **collision/interference detection**, can expose design errors at an early stage.

3. For assembly files exported from CAD software, parts (solids) which are spatially in contact with other parts have no topological relationship with each other. Shared surfaces between parts in contact must be cut out and merged into a single face (**imprinting**) which is then shared by the parts in contact. This enables, for example, the simulation of heat flowing from one part to its neighbour.

For large assemblies (with >10k parts), any geometry processing can be highly time-consuming. Computation time increases very fast with the total part count - it is approximately proportional to the square of the part count. Parallel computation on multi-core CPUs or clusters is essential to accelerate the design and verification process.

### Why this software is needed?

1. Traditionally, CAD software tools were not designed to deal with large assemblies and fully utilize modern multi-core CPUs. This software is designed with parallelism as the first design principle. Currently this software targets multi-threaded parallelism on multi-core shared memory systems.  In the future, it will be extended to target distributed memory HPC platforms.

2. This software is an open source solution that can fit into digital thread (organization-wide automatic design) workflows and can be deployed in cloud computing environments.

3. There is no de-facto open source CAD preprocessing tool like VTK/ParaView in the post-processing field; this software framework aims to fill that gap, by parallelizing existing algorithms which were developed for single-thread CPUs.
