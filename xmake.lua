set_project("OutlineRGB")
set_version("0.2.0")

set_languages("cxx20")

add_rules(
    "mode.debug",
    "mode.release"
)

-- LeviLauncher/Preloader provides the Android ARM64 inline-hook backend
-- used by native Levi mods. The mod remains a single .levipack containing
-- only libOutlineRGB.so; preloader is a host-side runtime dependency.
package("preloader")
    set_homepage("https://github.com/LiteLDev/preloader-android")
    set_description("Preloader Android")
    add_urls("https://github.com/LiteLDev/preloader-android.git")
    add_versions("main", "main")
    add_deps("cmake")
    on_install("android", function(package)
        import("package.tools.cmake").install(package)
    end)
package_end()

add_requires("preloader")

target("outlinergb")
    set_kind("shared")
    set_basename("outlinergb")

    add_files("src/*.cpp")

    add_includedirs(
        "include",
        {public = true}
    )

    add_packages("preloader")

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
