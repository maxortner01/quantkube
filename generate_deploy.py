from deploy import deploy
import os, json, yaml, re

def get_project_name(cmake_file):
    with open(cmake_file) as f:
        for line in f:
            if "project" in line:
                return re.search(r'\((.*?)\)', line).group(1)
    return None


def generate_dockerfiles():
    assert "services" in os.listdir("."), "Not in the root dir"

    for service in os.listdir("services"):
        path = "./services/" + service
        deploy.generate_deploy(get_project_name(os.path.join(path, "CMakeLists.txt")), path)
        print(os.path.join(path, "Dockerfile") + " created...")

def generate_compose_file(network_name):
    compose_dict = {
        "services": {},
        "networks": {
            network_name: {}
        }
    }

    # Get all the base containers
    for container in os.listdir("containers"):
        name = container.split(".")[-1]
        compose_dict["services"][name] = {
            "build": {
                "context": "./containers",
                "dockerfile": container
            },
            "image": name + "-env"
        }

    # Get the service containers
    for service_folder in os.listdir("services"):
        path = "./services/" + service_folder
        
        service = get_project_name(os.path.join(path, "CMakeLists.txt"))
        assert service, "Service name not found..."

        config = {
            "build": {
                "context": ".",
                "dockerfile": os.path.join(path, "Dockerfile")
            },
            "container_name": service,
            "depends_on": ["base"],
            "networks": [network_name]
        }

        print(f"Registered service: {service}", end="")

        deploy_json = os.path.join(path, "deploy.json")
        if os.path.isfile(deploy_json):
            print(" with extra deploy settings...", end="")
            with open(deploy_json) as f:
                config.update(json.load(f))

        compose_dict["services"][service] = config
        print("")

    with open("docker-compose.yml", "w") as f:
        yaml.dump(compose_dict, f, sort_keys=False, default_flow_style=False)

if __name__ == "__main__":
    print("Generating Dockerfiles...")
    generate_dockerfiles()
    print("Generating compose file...")
    generate_compose_file("cppnet")

    print("\nSuccess, now run")
    print("docker-compose build")
    print("docker-compose up -d")