from jinja2 import Template
import os

def generate_deploy(service_name, rel_dir):
    template_context = open("./deploy/template_docker.j2", "r").read()
    template = Template(template_context)

    with open(os.path.join(rel_dir, "Dockerfile"), "w") as f:
        f.write(template.render(
            name         = service_name, 
            external_dir = "./external",
            service_dir  = rel_dir))