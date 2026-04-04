CFLAGS=-march=native -std=c++23 -Wall -Wextra -Wshadow -pedantic

HEADERS=include/static_search.hpp include/internal.hpp include/dynamic_search.hpp

TEST_HEADERS=test/test.cpp test/test_static_set.hpp test/test_static_map.hpp \
             test/test_dynamic_set.hpp test/test_dynamic_multiset.hpp \
			 test/test_dynamic_map.hpp $(HEADERS) test/util.hpp test/test_runs.hpp

CL=$(shell getconf LEVEL1_DCACHE_LINESIZE)

.PHONY: test cover

bf_testing: bf_testing.cpp $(HEADERS) test/util.hpp
	g++ $(CFLAGS) -Ofast -DCACHE_LINE=$(CL) bf_testing.cpp -o bf_testing

test/test: googletest/build/lib/libgtest_main.a $(TEST_HEADERS)
	g++ $(CFLAGS) -g -DCACHE_LINE=$(CL) test/test.cpp -o test/test -lgtest_main -lgtest

test/cover: googletest/build/lib/libgtest_main.a $(TEST_HEADERS)
	g++ $(CFLAGS) --coverage -O0 -g -DCACHE_LINE=$(CL) test/test.cpp -o test/cover -lgtest_main -lgtest

test: test/test
	test/test $(ARG)

googletest/build/lib/libgtest_main.a: | googletest/googletest
	(mkdir -p googletest/build && cd googletest/build && cmake .. && make)

googletest/googletest:
	git submodule update --init

cover: googletest/build/lib/libgtest_main.a $(TEST_HEADERS) test/cover | coverage
	test/cover
	gcov test/cover-test.gcno
	lcov -c -d . --no-external --ignore-errors mismatch,inconsistent,inconsistent -o coverage/index.info 
	genhtml coverage/index.info -o coverage
	rm ./*.gcov
	rm ./test/*.gcda
	rm ./test/*.gcno

coverage:
	mkdir coverage