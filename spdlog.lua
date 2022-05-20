project "spdlog"

dofile(_BUILD_DIR .. "/static_library.lua")

configuration { "*" }

uuid "3398ecc5-f4a2-4fda-a82f-8ada78f80a02"

includedirs {
  "include",
  _3RDPARTY_DIR .. "/fmt/include",
}

files {
  "src/spdlog.cpp",
  "src/stdout_sinks.cpp",
  "src/color_sinks.cpp",
  "src/file_sinks.cpp",
  "src/async.cpp",
  "src/cfg.cpp",
}

defines {
  "SPDLOG_COMPILED_LIB",
}

if (_PLATFORM_ANDROID) then
end

if (_PLATFORM_COCOA) then
end

if (_PLATFORM_IOS) then
end

if (_PLATFORM_LINUX) then
end

if (_PLATFORM_MACOS) then
end

if (_PLATFORM_WINDOWS) then
end

if (_PLATFORM_WINUWP) then
end
