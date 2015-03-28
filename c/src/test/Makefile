OBJ=mem_size_test lru_test
all: ${OBJ}
mem_size_test:
	gcc -g -o mem_size_test cunit_memsize_test.c ../zip_lru_list.c ../log_name_tree.c ../dig_mem_pool.c ../log_view_stats.c ../log_view_tree.c ../dig_rb_tree.c ../log_heap.c ../statsmessage.pb-c.c -lcunit -lprotobuf-c

lru_test:
	gcc -g -o lru_test cunit_lru_test.c ../zip_lru_list.c  -lcunit
.PHONY: ${OBJ}
test:
	make
	./lru_test
	./mem_size_test
