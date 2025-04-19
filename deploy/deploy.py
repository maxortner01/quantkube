from jinja2 import Template
import os, json, _jsonnet

r"""
FROM base-env AS external_deps

# Get flatbuffers
RUN wget https://github.com/google/flatbuffers/archive/refs/tags/v25.2.10.tar.gz --no-check-certificate && tar -xvzf v25.2.10.tar.gz

# Compile flatbuffers
RUN cd flatbuffers-25.2.10 && cmake -DFLATBUFFERS_BUILD_SHAREDLIB=ON . && make -j
RUN mkdir external \ 
    && mkdir external/flatbuffers \
    && cp flatbuffers-25.2.10/flatc external/flatbuffers/flatc \
    && cp flatbuffers-25.2.10/libflatbuffers.so external/flatbuffers/libflatbuffers.so \
    && cp -r flatbuffers-25.2.10/include external/flatbuffers/include

# Copy over asio
RUN wget https://sourceforge.net/projects/asio/files/asio/1.30.2%20%28Stable%29/asio-1.30.2.tar.gz --no-check-certificate && tar -xvzf asio-1.30.2.tar.gz
RUN mkdir external/asio \
    && cp -r ./asio-1.30.2/include ./external/asio/include
"""

def generate_external_container(external_container_jsonnet):
    external = json.loads(_jsonnet.evaluate_file(external_container_jsonnet))

    content = ["FROM base-env AS external_deps"]

    for command in external["prelim"]:
        content.append("RUN {}".format(command["command"]))

    for library in external["libraries"]:
        print("Adding {}".format(library["name"]))
        for command in library["commands"]:
            content.append("RUN {}".format(command["command"]))

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