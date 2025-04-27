local base_container(name, depends = [], networks = [], env = {}) = {
    containers: {
        [name]: {
            config: {
                container_name: name,
                depends_on: depends,
                networks:   networks,
                environment: env
            }
        }
    }
};

local image_container(name, image, networks = [], volumes = [], ports = [], env = {}, command = [], depends = []) = {
    containers: {
        [name]: {
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
        }
    }
};

local build_container(name, directory, depends = [], networks = [], env = {}) = 
base_container(name, depends, networks, env) +
{
    containers+: {
        [name]+: {
            config+: {
                build: {
                    context: ".",
                    dockerfile: directory + "/Dockerfile"
                }
            }
        }
    }
};  

local base(services = [], containers = []) = 
{
    containers: {
        [k]: o.containers[k]
        for o in services
        for k in std.objectFields(o.containers)
    } + {
        [k]: o.containers[k]
        for o in containers
        for k in std.objectFields(o.containers)
    },
    services: {
        [k]: o.binaries[k]
        for o in services
        for k in std.objectFields(o.binaries)
    }
};

{   
    image_container:: image_container,
    build_container:: build_container,
    base:: base
}