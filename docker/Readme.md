## Readme for ppp_openmc docker image

This is a image with all open source tools for: FreeCAD + PPP + occ_faceter + gmsh + openmc  workflow

Note: this image is large, about 10 GB

For evaluation of parallel-preprocessor, use the smaller image `ppp-centos` instead.

OpenMC with MPI is not supported in this docker image, consider using singularity image to support MPI parallelization on HPC platforms.

### Get docker images

`docker pull qingfengxia/ppp_openmc`  for jupyter notebook way
`docker pull qingfengxia/ppp_openmc_ssh`  for jupyter notebook and ssh ways, it is based on image `qingfengxia/ppp_openmc`

https://hub.docker.com/repository/docker/qingfengxia/ppp_openmc

The Dockerfile is here: 

The docker image is based on `jupyter/mini-notebook` , but it can be switched to `ubuntu:focal` as base image, see instruction at the end of this document for switching base image.

## Use this image 

There are 3 ways to use these docker images, choose the one you like

### 1. The jupyter notebook way

`docker run --rm -p 8888:8888    ppp_openmc `
Jupyter can create a terminal in webbrowser, just like ssh terminal. 

`-p 8888:8888`  is for jupyter notebook web access
This docker image can be running in super user mode by giving `-e GRANT_SUDO=yes  --user root` which gives passwordless sudo capacity, to install more software.

Since nuclear material (tendl-2019-hdf5) has been built in, there is no need to map host folder to container like this. In case of some docker image is not built with material data, please use volume mapping such as ` -v /mnt/windata/MyData/openmc_data/tendl-2019-hdf5/:/mat_dir` for material cross section data. The host folder containing "cross-section.xml"  must be mapped to `/mat_dir`

For more details on options on `jupyter/mini-notebook` base docker image, see
https://jupyter-docker-stacks.readthedocs.io/en/latest/using/common.html

> -e GRANT_SUDO=yes - Instructs the startup script to grant the NB_USER user passwordless sudo capability. You do not need this option to allow the user to conda or pip install additional packages. This option is useful, however, when you wish to give $NB_USER the ability to install OS packages with apt or modify other root-owned files in the container. For this option to take effect, you must run the container with --user root. (The start-notebook.sh script will su $NB_USER after adding $NB_USER to sudoers.) You should only enable sudo if you trust the user or if the container is running on an isolated host.

### 2. X11 forwarding on local Linux host machine  (deprecated as X11 via SSH is more convenient)

to get an interactive bash shell, as ubuntu 20.04

```sh
docker run -it --rm  -v /tmp/.X11-unix:/tmp/.X11-unix -e DISPLAY=$DISPLAY -h $HOSTNAME -v $HOME/.Xauthority:/home/jovyan/.Xauthority -v $PWD/:/workspace/  ppp_openmc  bash
```

NOTE:  X11 forward is not working for `root` user, so do not add this option `-e GRANT_SUDO=yes  --user root `, perhaps caused by hardcoded absolute home path in `/home/jovyan/.Xauthority` 

Local X11 forwarding has been tested to work on Linux, it may works on Windows but with different environment variables passed in.
Local X11 forwarding can be quickly tested by
1) `xterm` is working without any xhost configuration, 

2) matplotlib.pyplot is working from terminal, tested by:  

`python3 -c "import matplotlib.pyplot as plt; plt.plot(); plt.show()" `

To understand why those arguments are needed
`-v /tmp/.X11-unix:/tmp/.X11-unix -e DISPLAY=$DISPLAY -h $HOSTNAME -v $HOME/.Xauthority:/home/jovyan/.Xauthority`
See  https://medium.com/@l10nn/running-x11-applications-with-docker-75133178d090

### 3. SSH remote shell 

SSH login is enabled by topping up the image with another thin layer, defined in `Dockerfile_ssh`

SSH has also X11 forwarding turned on after start the docker container:
`docker run --rm -p 2222:22 -p 8888:8888  -e GRANT_SUDO=yes  --user root  -it ppp_openmc_ssh bash ` then start the ssh server by `sudo service ssh start `

This argument `-p 2222:22` map container's port 22 to host 2222 port. 
To access this container by ssh  (-v means verbose,  -Y means X11 forwarding): 
either `ssh -v -Y localhost -p 2222` or  `ssh -v -Y <container_ip> -p 22`  

see also [How to setup an ssh server within a docker container](https://phoenixnap.com/kb/how-to-ssh-into-docker-container)
rebuild the image will change ssh server public key, as the ssh server installation  will generate new key pair each time.

#### user name is "jovyan" and password can be set during image building

On Ubuntu, install the `sshpass` package, then use it like this:  `sshpass -p 'YourPassword' ssh user@host` 

#### start ssh server automatically

Automatically start the ssh server is possible by `CMD ["/usr/sbin/sshd","-D"]`  in Dockerfile_ssh, if the docker container is running non-interactively (without -it option) 

In Dockerfile_ssh, the line `CMD ["/usr/sbin/sshd","-D"]` will override the jupyter’s start_notebook CMD. 

> Multiple CMD commands:  In principle, there should only be one CMD command in your Dockerfile. When CMD is used multiple times, only the last instance is executed.
   https://www.educative.io/edpresso/what-is-the-cmd-command-in-docker


### Rebuild ppp_openmc with ubuntu:focal as the base image

1) change the base image in Dockerfile before build:       `FROM   ubuntu:focal`
 or docker build with arg lie this `--build-arg IMAGE_NAME="qingfengxia/openmc_mpi" \  `

2) then build with command line, with different ARG values, 

`sudo docker build -f Dockerfile_ppp_openmc  -t  ppp_openmc . --no-cache --build-arg NB_USER=jovyan`

Note: `NB_USER=jovyan` is needed to be compatible with jupyter-notebook base image

see gull example in [build_openmc_mpi.sh](build_openmc_mpi.sh)

### MPI support for HPC cluster

Singularity will bind host HOME into the image, so python package installed into normal user home's `.local` will not visible.  One solution will be install all python module as root user into systemwide site-packages.


MPI version can be selected during image building,  therefore to match with HPC cluster's MPI version.

```
--build-arg MPI_VER=3.0 \
--build-arg MPI_VERSION=3.0.1 \
```

This image can then be converted into singularity image and bind the cluster host MPI library.

```bash
singularity pull docker://qingfengxia/openmc_mpi
# depends on singularity version, for version 2.x there is no needs to prefix mpirun inside container
mpirun -np 2 singularity exec --bind  ./openmc_mpi.simg openmc -h

```


