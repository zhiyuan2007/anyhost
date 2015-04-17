TESTS=test_lru_list
TESTS+=test_pkg_list
TESTS+=test_record_store
TESTS+=test_ip_wblist
TESTS+=test_domain_wblist
AM_CFLAGS=-I ../src
test_lru_list_SOURCES = ./cunit_lru_list.c\
                        ../src/zip_lru_list.c\
                        ../src/zip_pkg_list.c\
                        ../src/dig_wire_name.c\
                        ../src/dig_buffer.c
test_lru_list_LDADD = -lcunit

test_pkg_list_SOURCES = ./cunit_pkg_list.c\
                        ../src/zip_pkg_list.c\
                        ../src/dig_wire_name.c\
                        ../src/dig_buffer.c
test_pkg_list_LDADD = -lcunit

test_ip_wblist_SOURCES = ./cunit_ip_wblist.c\
                        ../src/zip_radix_tree.c\
                        ../src/zip_ip_wblist.c
test_ip_wblist_LDADD = -lcunit -lpthread

test_domain_wblist_SOURCES = ./cunit_domain_wblist.c\
                        ../src/zip_domain_wblist.c\
                        ../src/dig_domain_rbtree.c\
                        ../src/dig_rb_tree.c\
                        ../src/dig_wire_name.c\
                        ../src/dig_mem_pool.c\
                        ../src/dig_buffer.c
test_domain_wblist_LDADD = -lcunit -lpthread

test_record_store_SOURCES = ./cunit_record_store.c\
                        ../src/zip_record_store.c\
                        ../src/zip_pkg_list.c\
                        ../src/dig_domain_rbtree.c\
                        ../src/dig_wire_name.c\
                        ../src/dig_buffer.c\
                        ../src/zip_lru_list.c\
                        ../src/zip_pkg_manager.c\
                        ../src/dig_mem_pool.c\
                        ../src/dig_thread_pool.c\
                        ../src/dig_rb_tree.c\
                        ../src/dig_command.c\
                        ../src/dig_command_server.c\
                        ../src/zip_command_runner.c\
                        ../src/zip_ip_wblist.c\
                        ../src/zip_radix_tree.c\
                        ../src/zip_domain_wblist.c\
                        ../src/zip_addr.c\
                        ../src/zip_dns_lib.c\
                        ../src/dynamic_queue.c\
                        ../src/zip_socket.c

test_record_store_LDADD = @EVENT_LIBS@  -lcunit -lpthread -llua -lm -ldl -lreadline -lhistory -lncurses
noinst_PROGRAMS = $(TESTS)
