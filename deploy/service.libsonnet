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

local cpp_program(name, sources, external, includes = []) = {
    binary: {
        name: name,
        language: "cpp",
        sources: sources,
        dependencies: external,
        includes: includes,
        build: [
            cmd.cmake(".")
        ]
    }
};

{
    registered_dependency:: registered_dependency,
    registered_dependencies:: registered_dependencies,
    cpp_program:: cpp_program,
    external_package:: external_package
}