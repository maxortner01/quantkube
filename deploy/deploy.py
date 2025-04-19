from jinja2 import Template

def generate_deploy(service_name, out_dir):
    template_context = open("./deploy/template_docker.j2", "r").read()
    template = Template(template_context)

    with open(out_dir + "/Dockerfile", "w") as f:
        f.write(template.render(name = service_name))

if __name__ == "__main__":
    import sys

    SERVICE_NAME = sys.argv[1]
    OUT_DIR      = sys.argv[2]

    generate_deploy(SERVICE_NAME, OUT_DIR)