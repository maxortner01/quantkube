local svc    = import "../../deploy/service.libsonnet";
local deploy = import "../../deploy/deploy.libsonnet";

local service_name = "client";

svc.cpp_program(
    name     = service_name, 
    sources  = ["main.cpp"], 
    external = svc.registered_dependencies(
        directory = "external",
        names     = ["flatbuffers", "asio", "networking", "date", "schemas_cpp"]
    )
) +
deploy.build_container(
    name      = service_name,
    directory = "./services/client",
    depends   = ["base"],
    networks  = ["cppnet"]
)