target("gpio_actledd")
    set_type("application")
    add_deps("libbsp")
    add_files("**.c")        
    install_dir("drivers/raspix")
target_end()
