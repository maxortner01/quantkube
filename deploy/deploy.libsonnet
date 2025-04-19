// Define the copy function
local copy(from, to, directory = false) = {
  command: "cp" + (if directory then " -r" else "") + " " + from + " " + to
};

local get(url) = {
  local parts = std.split(url, "/"),
  local filename = parts[std.length(parts) - 1],

  command: "wget " + url + " --no-check-certificate && tar -xvzf " + filename
};

local mkdir(dir) = {
  command: "mkdir " + dir
};

local chain(commands) = {
  command: std.join(" && ", [c.command for c in commands])
};

local cmake(dir, opts) = {
  command: "cd " + dir + " && cmake " + std.join(" ", opts) + " . && make -j"
};

{
    copy:: copy,
    get:: get,
    mkdir:: mkdir,
    chain:: chain,
    cmake:: cmake
}