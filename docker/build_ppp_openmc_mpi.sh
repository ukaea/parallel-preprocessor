# see Dockerfile_ppp_openmc for default values for ARG
# base image seems can not be setup from ARG, and all ARG must be defined after FROM base_image
# manually set base image  Dockerfile_ppp_openmc between ubuntu and jupyter

# provide MPI short and full version as arg, to build MPI from source
# to match target cluster MPI version, to enable singularity image. 

# --build-arg NB_USER=jovyan,  if ubuntu:focal base image is used 
# to be compatible with jupyter notebook base image

# Docker's 17.05 release, allows: set FROM via ARG
#                     --build-arg IMAGE_NAME="ubuntu:focal" \

docker build . -f Dockerfile_ppp_openmc -t  qingfengxia/ppp_openmc_mpi   \
                    --build-arg IMAGE_NAME="ubuntu:focal" \
                    --build-arg include_preprocessor="true" \
                    --build-arg include_jupyter="false" \
                    --build-arg ENABLE_MPI="true" \
                    --build-arg BUILD_MPI="true" \
                    --build-arg MPI_VER=3.0 \
                    --build-arg MPI_VERSION=3.0.1 \
                    --build-arg include_material_data="false" \
                    --build-arg include_embree="true" \
                    --build-arg NB_USER="jovyan" \


# both system MPI packages and compile MPI from source are working for openmc compiling


####################### to use this docker image ########################
# if material not included, map the folder into the container, e.g. 
#  give full path to material data folder containing the xml

# export MAT_DIR=/mnt/windata/MyData/openmc_data/tendl-2019-hdf5/
# docker run --rm -it -v $MAT_DIR:/mat_dir qingfengxia/ppp_openmc_mpi   bash
# mpirun --allow-run-as-root -np 2 openmc -h

