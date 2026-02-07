-- set_config("plat", "windows")
-- set_config("arch", "x64")
-- set_config("mode", "debug")
add_rules("mode.debug", "mode.release")

target("bboy2")
    set_kind("binary")
    set_languages("c++17")

    add_files("src/**.cpp")
    add_includedirs("src")

    add_includedirs("libs/include")
    add_linkdirs("libs/lib")
    add_links("raylib")

    set_pcxxheader("src/project_types.h")

    if is_plat("windows") then
        add_cxflags("/FIproject_types.h", {
            force = true
        })
        
        add_syslinks("user32", "gdi32", "winmm", "shell32")
    end