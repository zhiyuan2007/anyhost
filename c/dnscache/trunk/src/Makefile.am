bin_PROGRAMS=dns_cache

AM_CPPFLAGS=@GLIB_CFLAGS@ @EVENT_CFLAGS@ -L$(RUBY_LIB_PATH)

dns_cache_LDADD=@GLIB_LIBS@ @EVENT_LIBS@ $(RUBY_LINK_ARG) -llua -lm -ldl -lreadline -lhistory -lncurses

dns_cache_SOURCES=dns_cache.c \
				zip_dns_lib.c \
				zip_socket.c zip_addr.c	zip_socket.h zip_addr.h\
				zip_dns_server.c zip_dns_server.h \
				dig_buffer.c dig_buffer.h \
				dig_wire_name.c dig_wire_name.h \
                dig_thread_pool.c\
                dig_mem_pool.c\
                zip_pkg_manager.c zip_pkg_manager.h\
                zip_pkg_list.c zip_pkg_list.h\
                zip_record_store.c zip_record_store.h\
                zip_lru_list.c zip_lru_list.h\
                dig_rb_tree.c dig_rb_tree.h\
                zip_ip_wblist.c zip_ip_wblist.h\
                zip_radix_tree.c zip_radix_tree.h\
                zip_domain_wblist.c zip_domain_wblist.h\
                dig_command.c dig_command.h\
                dig_command_server.c dig_command_server.h\
                zip_command_runner.c zip_command_runner.h\
                dynamic_queue.c dynamic_queue.h\
				dig_domain_rbtree.c dig_domain_rbtree.h
