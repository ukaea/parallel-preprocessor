# delete image without name
docker rmi -f $(docker images  | awk ' /^<none>/ {print $3}')
docker container ls -a >all_docker_container.csv
# delete line (with container to keep) in nano by shortcut key: ctrl+K
nano all_docker_container.csv
docker container rm $(cat all_docker_container.csv | awk  'BEGIN { FS = OFS = "    "}  {print $1}')