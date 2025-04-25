local base_container(name, depends = [], networks = [], env = {}) = {
    config: {
        container_name: name,
        depends_on: depends,
        networks:   networks,
        environment: env
    }
};

local image_container(name, image, networks = [], volumes = [], ports = [], env = {}, command = [], depends = []) = {
    config: {
        container_name: name,
        image: image,
        networks: networks,
        volumes: volumes,
        ports: ports,
        environment: env,
        command: command,
        depends_on: depends
    }
};

local build_container(name, directory, depends = [], networks = [], env = {}) = 
base_container(name, depends, networks, env) +
{
    config+: {
        build: {
            context: ".",
            dockerfile: directory + "/Dockerfile"
        }
    }
};  

{   
    image_container:: image_container,
    build_container:: build_container
}