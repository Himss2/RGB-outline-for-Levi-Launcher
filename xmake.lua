add_rules("mode.debug", "mode.release")

set_languages("cxx20")

target("SelectionOutline")
    set_kind("shared")
    set_arch("arm64-v8a")

    add_files("src/*.cpp")

    add_includedirs("include", {public = true})

    add_syslinks(
        "log",
        "dl",
        "android"
    )

    if is_plat("android") then
        add_defines("ANDROID")
    end

    set_targetdir("build")

    after_build(function (target)
        print("========================================")
        print(" SelectionOutline build complete")
        print(" Output: " .. target:targetfile())
        print("========================================")
    end)
