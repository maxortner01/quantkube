local svc    = import "../../deploy/service.libsonnet";
local deploy = import "../../deploy/deploy.libsonnet";

local directory = svc.get_dir(std.thisFile);
local service_name = "server";
local db_env = {
    POSTGRES_DB: "timeseries",
    POSTGRES_USER: "user",
    POSTGRES_PASSWORD: "password"
};

svc.cpp_program(
    name     = service_name, 
    path     = directory,
    sources  = ["src/main.cpp", "src/db/Companies.cpp", "src/db/Prices.cpp"], 
    includes = ["src"],
    external = svc.registered_dependencies(
        directory = "external",
        names     = ["flatbuffers", "asio", "prometheus", "networking", "date", "schemas_cpp"]
    ) + [svc.external_package("PostgreSQL")]
) +
deploy.build_container(
    name      = service_name,
    directory = directory,
    depends   = ["base", "timescaledb"],
    networks  = ["cppnet"],
    env       = {
        POSTGRES_HOST: "timescaledb"
    } + db_env
)