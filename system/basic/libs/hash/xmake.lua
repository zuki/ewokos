target("libhash")
    set_type("library")
    add_files("src/**.c")
    add_includedirs("include", {public = true})
target_end()
