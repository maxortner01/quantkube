from jinja2 import Template
import os, json, _jsonnet

# We should probably do a sub image per, then do the copy statements automatically
def generate_external_container(external_container_jsonnet):
    external = json.loads(_jsonnet.evaluate_file(external_container_jsonnet))

    content = ["FROM base-env AS external_deps"]

    for command in external["prelim"]:
        content.append("{} {}".format(command["docker"], command["command"]))

    for library in external["libraries"]:
        print("Adding {}".format(library["name"]))
        for command in library["commands"]:
            content.append("{} {}".format(command["docker"], command["command"]))

    return "\n".join(content)

def generate_deploy(service_name, rel_dir, external_container_jsonnet):
    template_context = open("./deploy/template_docker.j2", "r").read()
    template = Template(template_context)

    with open(os.path.join(rel_dir, "Dockerfile"), "w") as f:
        f.write(template.render(
            name         = service_name, 
            external_dir = "./external",
            service_dir  = rel_dir,
            external_container = generate_external_container(external_container_jsonnet)))