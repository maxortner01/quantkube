// Define the copy function
local copy(from, to, directory = false) = {
  docker: "RUN",
  command: "cp" + (if directory then " -r" else "") + " " + from + " " + to
};

local get(url) = {
  local parts = std.split(url, "/"),
  local filename = parts[std.length(parts) - 1],

  docker: "RUN",
  command: "wget " + url + " --no-check-certificate && tar -xvzf " + filename
};

local mkdir(dir) = {
  docker: "RUN",
  command: "mkdir " + dir
};

local chain(commands) = {
  docker: "RUN",
  command: std.join(" && ", [c.command for c in commands])
};

local run(command) = {
  docker: "RUN",
  command: command
};

local cmake(dir, opts = []) = run("cd " + dir + " && cmake -DCMAKE_BUILD_TYPE=Release " + std.join(" ", opts) + " . && make -j");

local container_copy(from, to, opts = []) = {
  docker: "COPY",
  command: std.join(" ", opts) + " " + from + " " + to
};

{
    copy:: copy,
    get:: get,
    run:: run,
    mkdir:: mkdir,
    chain:: chain,
    cmake:: cmake,
    container_copy:: container_copy
}