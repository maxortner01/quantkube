local lib = import "../deploy/deploy.libsonnet";

{
    // Preliminary commands before we do anything with the libraries
    prelim: [
        lib.mkdir("external"),
    ],
    // Commands needed to get the libraries in a working state
    libraries: [
        {
            name: "asio",
            commands: [
                lib.get("https://sourceforge.net/projects/asio/files/asio/1.30.2%20%28Stable%29/asio-1.30.2.tar.gz"),
                lib.chain([ 
                    lib.mkdir("external/asio"),
                    lib.copy("./asio-1.30.2/include", "./external/asio/include", true),
                ]),
            ],
        },
        {
            name: "flatbuffers",
            commands: [
                lib.get("https://github.com/google/flatbuffers/archive/refs/tags/v25.2.10.tar.gz"),
                lib.cmake("flatbuffers-25.2.10", ["-DFLATBUFFERS_BUILD_SHAREDLIB=ON"]),
                lib.chain([
                    lib.mkdir("external/flatbuffers"),
                    lib.copy("flatbuffers-25.2.10/flatc", "external/flatbuffers/flatc"), // copy the flatc binary
                    lib.copy("flatbuffers-25.2.10/libflatbuffers.so", "external/flatbuffers/libflatbuffers.so"), // copy the shared lib
                    lib.copy("flatbuffers-25.2.10/include", "external/flatbuffers/include", true), // copy the headers
                ]),
            ],
        },
        {
            name: "networking",
            commands: [
                lib.container_copy("./external/networking", "./external/networking"),
            ],
        },
        {
            name: "date",
            commands: [
                lib.get("https://github.com/HowardHinnant/date/archive/refs/tags/v3.0.3.tar.gz"),
                lib.chain([
                    lib.mkdir("external/date"),
                    lib.copy("./date-3.0.3/include", "./external/date/include", true)
                ])
            ]
        },
        {
            name: "prometheus",
            commands: [
                lib.get("https://github.com/jupp0r/prometheus-cpp/releases/download/v1.3.0/prometheus-cpp-with-submodules.tar.gz"),
                lib.cmake("prometheus-cpp-with-submodules", ["-DBUILD_SHARED_LIBS=ON"]),
                lib.chain([
                    lib.mkdir("external/prometheus"),
                    lib.mkdir("external/prometheus/core"),
                    lib.mkdir("external/prometheus/pull"),
                    lib.mkdir("external/prometheus/push"),
                    lib.copy("./prometheus-cpp-with-submodules/lib", "./external/prometheus/lib", true),
                    lib.copy("./prometheus-cpp-with-submodules/core/include", "./external/prometheus/core/include", true),
                    lib.copy("./prometheus-cpp-with-submodules/pull/include", "./external/prometheus/pull/include", true),
                    lib.copy("./prometheus-cpp-with-submodules/push/include", "./external/prometheus/push/include", true)
                ])
            ]
        }
    ],
}