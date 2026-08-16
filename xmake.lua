set_project("OutlineRGB")
set_version("0.2.0")

set_languages("cxx20")

add_rules(
    "mode.debug",
    "mode.release"
)

package("preloader")
    set_homepage(
        "https://github.com/LiteLDev/preloader-android"
    )

    set_description(
        "Preloader Android"
    )

    add_urls(
        "https://github.com/LiteLDev/preloader-android.git"
    )

    add_versions(
        "main",
        "main"
    )

    add_deps("cmake")

    on_install(
        "android",
        function(package)
            import("package.tools.cmake")
                .install(package)
        end
    )
package_end()

add_requires("preloader")

target("outlinergb")
    set_kind("shared")

    set_basename("outlinergb")

    add_files(
        "src/*.cpp"
    )

    add_includedirs(
        "include",
        {public = true}
    )

    add_packages(
        "preloader"
    )

    add_cxxflags(
        "-Wall",
        "-Wextra",
        "-Wpedantic",
        "-fvisibility=hidden",
        {force = true}
    )

    if is_plat("android") then

        add_cxflags(
            "-fPIC",
            "-Oz",
            "-ffunction-sections",
            "-fdata-sections",
            "-fvisibility=hidden",
            {force = true}
        )

        add_shflags(
            "-Wl,--gc-sections",
            "-Wl,--hash-style=gnu",
            "-Wl,-z,max-page-size=16384",
            {force = true}
        )

        add_syslinks(
            "android",
            "log",
            "dl"
        )

end
