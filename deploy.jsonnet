local deploy = import "./deploy/deploy.libsonnet";

local client = import "./services/client/deploy.jsonnet";
local server = import "./services/server/deploy.jsonnet";

deploy.base(
    services   = [server, client],
    containers = [
        deploy.image_container(
            name     = "timescaledb",
            image    = "timescale/timescaledb:latest-pg14",
            env      = server.containers.server.config.environment,
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
)