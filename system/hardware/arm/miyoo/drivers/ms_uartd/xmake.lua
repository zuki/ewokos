target("ms_uartd")
    set_type("application")
    add_deps("libbsp")
    add_files("**.c")        
    install_dir("drivers/miyoo")
target_end()
