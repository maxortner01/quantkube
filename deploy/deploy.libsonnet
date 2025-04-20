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

local cmake(dir, opts) = {
  docker: "RUN",
  command: "cd " + dir + " && cmake " + std.join(" ", opts) + " . && make -j"
};

local container_copy(from, to) = {
  docker: "COPY",
  command: from + " " + to
};

{
    copy:: copy,
    get:: get,
    mkdir:: mkdir,
    chain:: chain,
    cmake:: cmake,
    container_copy:: container_copy
}