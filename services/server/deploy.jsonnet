local svc    = import "../../deploy/service.libsonnet";
local deploy = import "../../deploy/deploy.libsonnet";

local service_name = "server";
local db_env = {
    POSTGRES_DB: "timeseries",
    POSTGRES_USER: "user",
    POSTGRES_PASSWORD: "password"
};

svc.cpp_program(
    name     = service_name, 
    sources  = ["src/main.cpp", "src/db/Companies.cpp", "src/db/Prices.cpp"], 
    includes = ["src"],
    external = svc.registered_dependencies(
        directory = "external",
        names     = ["flatbuffers", "asio", "prometheus", "networking", "date", "schemas_cpp"]
    ) + [svc.external_package("PostgreSQL")]
) +
deploy.build_container(
    name      = service_name,
    directory = "./services/server",
    depends   = ["base", "timescaledb"],
    networks  = ["cppnet"],
    env       = {
        POSTGRES_HOST: "timescaledb"
    } + db_env
) + {
    dependent_containers: [
        deploy.image_container(
            name     = "timescaledb",
            image    = "timescale/timescaledb:latest-pg14",
            env      = db_env,
            ports    = ["5432:5432"],
            networks = ["cppnet"],
            volumes  = ["/home/mortner/quantkube/volumes/db-data:/var/lib/postgresql/data"]
        ),
        deploy.image_container(
            name     = "prometheus",
            image    = "prom/prometheus:latest",
            volumes  = ["./monitoring/prometheus.yml:/etc/prometheus/prometheus.yml"],
            command  = ["--config.file=/etc/prometheus/prometheus.yml"],
            ports    = ["9090:9090"],
            networks = ["cppnet"]
        ),
        deploy.image_container(
            image      = "grafana/grafana:latest",
            name       = "grafana",
            ports      = ["3000:3000"],
            networks   = ["cppnet"],
            depends    = ["prometheus"],
            volumes    = ["grafana-storage:/var/lib/grafana"]
        )
    ]
}