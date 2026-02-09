-- set_config("plat", "windows")
-- set_config("arch", "x64")
-- set_config("mode", "debug")
add_rules("mode.debug", "mode.release")

option("profile_trace")
    set_default(false)
    set_showmenu(true)

target("tracy_client")
    set_kind("static")
    set_languages("c++17")
    
    add_files("tracy/public/TracyClient.cpp")
    add_includedirs("tracy/public")

    if get_config("profile_trace") then
        add_defines("TRACY_ENABLE", "TRACY_NO_SYSTEM_TRACING")
    end

    if is_plat("windows") then
        add_syslinks("dbghelp", "ws2_32", "advapi32")
    end

target("bboy2")
    set_kind("binary")
    set_languages("c++17")

    set_rundir("$(projectdir)")

    add_files("src/**.cpp")
    add_includedirs("src")

    add_includedirs("tracy/public")

    add_includedirs("libs/include")
    add_linkdirs("libs/lib")
    add_links("raylib")

    set_pcxxheader("src/project_types.h")

    if get_config("profile_trace") then
        add_defines("TRACY_ENABLE", "TRACY_NO_SYSTEM_TRACING")
        add_deps("tracy_client")
    end

    if is_plat("windows") then
        add_syslinks("user32", "gdi32", "winmm", "shell32")
    end