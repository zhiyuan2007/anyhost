bin_PROGRAMS=dns_cache

#AM_CPPFLAGS=@GLIB_CFLAGS@ @EVENT_CFLAGS@ -L$(RUBY_LIB_PATH)
AM_CPPFLAGS=@EVENT_CFLAGS@ -L$(RUBY_LIB_PATH)

#dns_cache_LDADD=@GLIB_LIBS@ @EVENT_LIBS@ $(RUBY_LINK_ARG)
dns_cache_LDADD=@EVENT_LIBS@ $(RUBY_LINK_ARG)

dns_cache_SOURCES=dns_cache.c \
				zip_dns_lib.c \
				zip_socket.c zip_addr.c	zip_socket.h zip_addr.h\
				zip_dns_server.c zip_dns_server.h \
                dig_thread_pool.c\
                dig_mem_pool.c\
                dig_rb_tree.c dig_rb_tree.h\
                dynamic_queue.c dynamic_queue.h
