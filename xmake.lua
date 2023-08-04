-- xmake.lua
add_rules("mode.debug", "mode.release") -- xmake f -m debug/release来切换编译模式

set_languages("cxx17")

add_includedirs("$(projectdir)")

-- 设置默认的输出目录
set_targetdir("$(projectdir)/bin")

-- Required packages
add_requires("boost", "yaml-cpp")

-- Define the static library
target("hiper")
    set_kind("static")
    add_files(
        "hiper/base/**.cc"
    )
    add_headerfiles(
        "hiper/base/**.h"
    )
    add_packages("boost", "yaml-cpp")
    add_syslinks("pthread")
    set_targetdir("$(projectdir)/lib")

-- Define the executable targets
for _, name in ipairs({"mutex_test", "log_test", "config_test", "try"}) do
    target(name)
        set_kind("binary")
        add_files("tests/" .. name .. ".cc")
        add_packages("boost", "yaml-cpp")
        add_deps("hiper")
        add_syslinks("pthread")
end

-- The default rule
if is_mode("release") then
    set_optimize("fastest")
else
    add_cxflags("-O0", "-ggdb")
end