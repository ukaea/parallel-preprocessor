
## Automate CAD-CAE design workflow

### Existing and future CAD-CAE design workflow

![Existing CAD and CAE design-validation workflow](https://github.com/qingfengxia/CAE_pipeline/raw/master/images/current_CAE_process.png)

If an automated workflow can achived, just like continuous integration (CI) is software engineering, the impact of any local design change on the whole design can be evulated promptly. This automated workflow is crucial for design a large appratus when multiple teams and organizations collobration involved.

Once design automation has been achieved, intelligent engineering design can be realized.
![diagram of future intelligent engineering design](https://github.com/qingfengxia/CAE_pipeline/raw/master/images/intelligent_engineering_design.png)

More details and relevant projects to enable the automated and intelligent engineering design can be found 
https://github.com/qingfengxia/CAE_pipeline

### What is geometry preprocessing on the context of exa-scale CAE

1. Geometry from design office, i.e. CAD files served for manufacturing, is full of manufacturing details. It must be simplified (**defeaturing**) and fixed, in order to be used in design validation (CAE) via physical and engineering simulation.

2. Continuous assembly validation (digital mock-up) such as **collision/interference detection**, can expose design error in early stage. 

3. For assembly file exported from CAD software, each geometry has no topology connection with other parts although they are spatially in contact. Shared surface between parts in contact must be cut out and merged into a single face (**imprinting**) that topologically shared by parts in contact. For example, heat can NOT flow from one part of the next part in contact, if geometry is not imprinted and merged. 

For large assembly (10k to 100k) as a whole, any geometry processing can be highly time-consuming, given the fact that computation time increases very fast with the total solid (part) count, estimated to be proprotional to square of the part count. Parallel computation on multi-core CPU or cluster is essential to accelerate the design and verification process.

### Why this software is needed?

1. Traditionally, CAD software tools were not designed to deal with large assembly fully utilizing modern multi-core CPU. This software design with parallelism as the first design principle. Currently this software targets on multi-threading parallelism on multi-core shared memory system, in the future, extensible to distrubutive HPC platforms such as super computer and cluster infrastructure.

2. This software is based on open source solution that can fit into digital thread (organization-wide automatic design workflow) and can be deployed in cloud computation environment.

3. There is no de-facto open source CAD preprocessing tool like VTK/paraview on the post-processing field; this software framework will try to fill the gap in the open science, by assisting parallelizing existing algorithms developed for single-thread CPU.