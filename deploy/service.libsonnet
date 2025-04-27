local cmd = import "commands.libsonnet";

local registered_dependency(name, directory) = {
    name:      name,
    directory: directory,
    type:      "registered"
};

local registered_dependencies(directory, names) = [
    registered_dependency(name, directory) for name in names
];

local external_package(name) = {
    name: name,
    type: "external"
};

local cpp_program(name, path, sources, external, includes = []) = {
    binaries: {
        [name]: {
            name: name,
            language: "cpp",
            sources: sources,
            dependencies: external,
            includes: includes,
            path: path,
            build: [
                cmd.cmake(".")
            ]
        }
    }
};

local get_dir(path) =
  local parts = std.split(path, '/');
  std.join('/', std.slice(parts, 0, std.length(parts) - 1, 1));

{
    registered_dependency:: registered_dependency,
    registered_dependencies:: registered_dependencies,
    cpp_program:: cpp_program,
    external_package:: external_package,
    get_dir:: get_dir
}