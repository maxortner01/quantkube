import _jsonnet as js
import os, json, difflib, yaml

def jsonnet_to_json(dir):
    return json.loads(js.evaluate_file(dir))

class Dependency:
    def __init__(self, config, prelim = {}):
        self.config = config
        self.prelim = prelim

    def copy(self, copy_to):
        copying_from = f"/{self.config['copy_from'] if 'copy_from' in self.config else self.config['name']}"
        copying_to   = f"/{self.config['copy_to'] if 'copy_to' in self.config else f"{copy_to}/{self.config['name']}" }"

        return copying_from, copying_to

    def prerender(self, copy_to = "."):
        """
        Render the strings for this depencency's copy command as well as the instructions needed to build its container
        """
        r = { "content": [], "copy": [] }
        
        # Header
        r["content"].append(f"# Container for library: {self.config['name']}")
        r["content"].append(f"FROM base-env AS dep_{self.config['name']}")

        copying_from, copying_to = self.copy(copy_to)

        # We want to copy from the specified copy_from to the copy_to
        r["copy"].append(f"COPY --from=dep_{self.config['name']} {copying_from} {copying_to}")

        for command in self.prelim:
            r["content"].append("{} {}".format(command["docker"], command["command"]))

        for command in self.config["commands"]:
            r["content"].append("{} {}".format(command["docker"], command["command"]))
        r["content"].append("")

        return r["content"], r["copy"]

class Dependencies:
    def __init__(self, external_jsonnet):
        # Render the external libs config
        assert os.path.exists(external_jsonnet), f"External file {external_jsonnet} does not exist!"
        self.config = jsonnet_to_json(external_jsonnet)

        # For each of these libraries construct a dependency
        self.dependencies = { config["name"]: Dependency(config, self.config.get("prelim", {})) for config in self.config["libraries"] }

    def get_dep(self, name):
        assert name in self.dependencies, f"Dependency {name} not registered! Did you mean: '{difflib.get_close_matches(name, self.dependencies.keys())[0]}'"
        return self.dependencies[name]

    def prerender(self, dependencies, copy_to=""):
        """
        Render the strings needed to copy and build all given dependencies
        """
        r = { "content": [], "copy": [] }

        for dep in dependencies:
            if dep["type"] != "registered": continue

            d = self.get_dep(dep["name"])
            content, copy = d.prerender(self.config.get("copy_to", dep["directory"]))
            r["content"] += content
            r["copy"]    += copy

        return r["content"], r["copy"]
    
    def get_directories(self, dependencies):
        includes  = []
        libraries = []

        for d in dependencies:
            if d["type"] != "registered": continue
            dep = self.get_dep(d["name"])

            _, to = dep.copy(d["directory"])
            includes  += [ f"{to.removesuffix(d['name'])}{i}" for i in dep.config.get("directories", {}).get("includes",  []) ]
            libraries += [ f"{to.removesuffix(d['name'])}{i}" for i in dep.config.get("directories", {}).get("libraries", []) ]
        
        return includes, libraries
    
    def get_binaries(self, dependencies):
        binaries = []
        for d in dependencies:
            if d["type"] != "registered": continue
            dep = self.get_dep(d["name"])

            binaries += dep.config.get("binaries", [])
        return binaries
        
class Service:
    def __init__(self, service_dir):
        # Render the service config json
        assert os.path.exists(service_dir), f"Service directory '{service_dir}' does not exist!"
        self.directory = service_dir

        deploy_jsonnet = os.path.join(service_dir, "deploy.jsonnet")
        assert os.path.exists(deploy_jsonnet), f"Service deployment '{deploy_jsonnet}' does not exist!"
        self.config = jsonnet_to_json(deploy_jsonnet)

    def prerender_build(self):
        if "build" not in self.config["binary"]: return []
        return [ f"{b['docker']} {b['command']}" for b in self.config["binary"]["build"] ]
    
    def get_containers(self):
        r = { c["config"]["container_name"]: c["config"] for c in self.config.get("dependent_containers", []) }
        r[self.config["binary"]["name"]] = self.config['config']
        return r

    def render_dockerfile(self, deps: Dependencies):
        content, dep_copy = deps.prerender(self.config["binary"]["dependencies"])

        content += ["FROM base-env", ""]

        # Copy in dependencies
        content += dep_copy

        # Copy in service files
        content += [f"COPY {self.directory} .", ""]

        # Build the service
        content += self.prerender_build()

        # Run the service
        content += [f"ENTRYPOINT [\"./{self.config["binary"]['name']}\"]"]

        return "\n".join(content)
    
    def render_cmake(self, deps: Dependencies):
        def make_str(prefix, visibility, content):
            return f"{prefix}({self.config["binary"]['name']} {visibility}\n" + "\n".join([ f"\t{c}" for c in content ]) + ")\n"

        includes, libraries = deps.get_directories(self.config["binary"]["dependencies"])
        packages = [ e['name'] for e in self.config["binary"]["dependencies"] if e["type"] == "external" ]

        content = ["cmake_minimum_required(VERSION 3.8)", "", f"project({self.config["binary"]['name']})", "", "set(CMAKE_CXX_STANDARD 20)", ""]

        for p in packages:
            content.append(f"find_package({p} REQUIRED)")
        content.append("")

        content += [make_str("add_executable", "", [ "${CMAKE_SOURCE_DIR}/" + s for s in self.config["binary"]["sources"] ]), ""]
        content += [make_str("target_include_directories", "PRIVATE", [ "${CMAKE_SOURCE_DIR}" + i for i in includes + self.config["binary"].get("includes", []) ]), ""]
        content += [make_str("target_link_directories", "PRIVATE", [ "${CMAKE_SOURCE_DIR}" + i for i in libraries ]), ""]
        content += [make_str("target_link_libraries", "PRIVATE", deps.get_binaries(self.config["binary"]["dependencies"]) + [ f"{e}::{e}" for e in packages ] + ["pthread"])] # Need to get smarter about how we add in find_package dependencies

        return "\n".join(content)


if __name__ == "__main__":
    deps = Dependencies("./external/container.jsonnet")
    
    compose = {
        "networks": {},
        "volumes": {},
        "services": {}
    }

    for container in os.listdir("containers"):
        name = container.split(".")[-1]
        compose["services"][name] = {
            "build": {
                "context": "./containers",
                "dockerfile": container
            },
            "image": name + "-env"
        }

    for service in os.listdir("./services"):
        svc = Service(os.path.join("./services", service))

        compose["services"].update(svc.get_containers())
        
        files = {
            "Dockerfile":     svc.render_dockerfile(deps),
            "CMakeLists.txt": svc.render_cmake(deps)
        }

        for fn, content in files.items():
            with open(os.path.join("./services", service, fn), "w") as f:
                f.write(content)

    for service, config in compose["services"].items():
        compose["volumes"].update({ name: {} for name in [ v.split(":")[0] for v in config.get("volumes", []) if ":" in v and "/" not in v.split(":")[0] ] })
        compose["networks"].update({ name: {} for name in config.get("networks", []) })

    with open("docker-compose.yml", "w") as f:
        yaml.dump(compose, f, sort_keys=False, default_flow_style=False)