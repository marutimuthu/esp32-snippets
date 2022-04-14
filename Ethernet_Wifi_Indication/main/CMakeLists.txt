set(srcs "main_program.c")

idf_component_register(SRCS "led_strip.c" "main_program.c" "${srcs}"
                       INCLUDE_DIRS "."
			EMBED_TXTFILES ${project_dir}/server_certs/ca_cert.pem)
