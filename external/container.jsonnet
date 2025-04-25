local lib = import "../deploy/commands.libsonnet";

local external_folder = "external";

{
    // Preliminary commands before we do anything with the libraries
    //prelim: [
    //    lib.mkdir(external_folder),
    //],
    copy_to: external_folder, // should be able to overwrite this per container
    // Commands needed to get the libraries in a working state
    libraries: [
        {
            name: "asio",
            commands: [
                lib.get("https://sourceforge.net/projects/asio/files/asio/1.30.2%20%28Stable%29/asio-1.30.2.tar.gz"),
                lib.chain([ 
                    lib.mkdir("asio"),
                    lib.copy("./asio-1.30.2/include", "./asio/include", true),
                ]),
            ],
            directories: {
                includes: ["asio/include"]
            }
        },
        {
            name: "flatbuffers",
            commands: [
                lib.get("https://github.com/google/flatbuffers/archive/refs/tags/v25.2.10.tar.gz"),
                lib.cmake("flatbuffers-25.2.10", ["-DFLATBUFFERS_BUILD_SHAREDLIB=ON"]),
                lib.chain([
                    lib.mkdir("flatbuffers"),
                    lib.copy("flatbuffers-25.2.10/flatc", "flatbuffers/flatc"), // copy the flatc binary
                    lib.copy("flatbuffers-25.2.10/libflatbuffers.so", "flatbuffers/libflatbuffers.so"), // copy the shared lib
                    lib.copy("flatbuffers-25.2.10/include", "flatbuffers/include", true), // copy the headers
                ]),
            ],
            directories: {
                includes: ["flatbuffers/include"],
                libraries: ["flatbuffers"]
            },
            binaries: ["flatbuffers"]
        },
        {
            name: "networking",
            commands: [
                lib.container_copy("./external/networking", "./networking"),
            ],
            directories: {
                includes: ["networking"]
            }
        },
        {
            name: "date",
            commands: [
                lib.get("https://github.com/HowardHinnant/date/archive/refs/tags/v3.0.3.tar.gz"),
                lib.chain([
                    lib.mkdir("date"),
                    lib.copy("./date-3.0.3/include", "./date/include", true)
                ])
            ],
            directories: {
                includes: ["date/include"]
            }
        },
        {
            name: "prometheus",
            commands: [
                lib.get("https://github.com/jupp0r/prometheus-cpp/releases/download/v1.3.0/prometheus-cpp-with-submodules.tar.gz"),
                lib.cmake("prometheus-cpp-with-submodules", ["-DBUILD_SHARED_LIBS=ON"]),
                lib.chain([
                    lib.mkdir("prometheus"),
                    lib.mkdir("prometheus/core"),
                    lib.mkdir("prometheus/pull"),
                    lib.mkdir("prometheus/push"),
                    lib.copy("./prometheus-cpp-with-submodules/lib", "./prometheus/lib", true),
                    lib.copy("./prometheus-cpp-with-submodules/core/include", "./prometheus/core/include", true),
                    lib.copy("./prometheus-cpp-with-submodules/pull/include", "./prometheus/pull/include", true),
                    lib.copy("./prometheus-cpp-with-submodules/push/include", "./prometheus/push/include", true)
                ])
            ],
            directories: {
                includes: [
                    "prometheus/core/include",
                    "prometheus/pull/include",
                    "prometheus/push/include"
                ],
                libraries: [
                    "prometheus/lib"
                ]
            },
            binaries: [
                "prometheus-cpp-core",
                "prometheus-cpp-pull",
                "prometheus-cpp-push",
            ]
        },
        {
            name: "schemas_cpp",
            commands: [
                lib.container_copy("/flatbuffers/flatc", "/flatc", ["--from=dep_flatbuffers"]),
                lib.container_copy("./api", "./api"),
                lib.run("./flatc --cpp --gen-object-api -o ./schemas ./api/*.fbs ")
            ],
            copy_from: "schemas",
            copy_to: "schemas",
            directories: {
                includes: [""]
            }
        }
    ],
}