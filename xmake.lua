set_project("OutlineRGB")
set_version("0.1.0")

set_languages("cxx20")

add_rules(
    "mode.debug",
    "mode.release"
)

target("outlinergb")
    set_kind("shared")
    set_basename("outlinergb")

    add_files("src/*.cpp")

    add_includedirs(
        "include",
        {public = true}
    )

    add_cxxflags(
        "-Wall",
        "-Wextra",
        "-Wpedantic",
        "-fvisibility=hidden",
        {force = true}
    )

    if is_plat("android") then
        add_syslinks(
            "log",
            "dl"
        )
end
