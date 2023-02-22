
ALL_TARGETS=tree_to_range tree_to_range_benchmark test_assert test_sparse_index_vector test_append_to_string_literal 


all: $(ALL_TARGETS)

clean: 
	-rm $(ALL_TARGETS)

distclean: clean conan-clean

include conanbuildinfo.mak # Created by conan 'make' generator, defines CONAN_* variables used below
FMT_CXXFLAGS=$(CONAN_INCLUDE_DIRS_FMT:%=-I%) $(CONAN_DEFINES_FMT:%=-D%) $(CONAN_CXXFLAGS_FMT)
FMT_LFLAGS=$(CONAN_LIB_DIRS_FMT:%=-L%) $(CONAN_LIBS_FMT:%=-l%) $(CONAN_SYSTEM_LIBS_FMT:%=-l%)
BENCH_CXXFLAGS=$(CONAN_INCLUDE_DIRS_BENCHMARK:%=-I%) $(CONAN_DEFINES_BENCHMARK:%=-D%) $(CONAN_CXXFLAGS_BENCHMARK)
BENCH_LFLAGS=$(CONAN_LIB_DIRS_BENCHMARK:%=-L%) $(CONAN_LIBS_BENCHMARK:%=-l%) $(CONAN_SYSTEM_LIBS_BENCHMARK:%=-l%)

#read_all_conan_args=$(shell cat conanbuildinfo.args) # alternative, from conan compiler args generator

CXX?=g++
CXX+=-m64 -DNDEBUG -D_GLIBCXX_USE_CXX11_ABI=0  # Toolchain flags not generated in conanbuildinfo.mak by conan 'make' generator. (Where is make_toolchain.mak? does it still exist?)

info:
	@echo CXX=$(CXX)
	@echo FMT_CXXFLAGS=$(FMT_CXXFLAGS)
	@echo FMT_LFLAGS=$(FMT_LFLAGS)
	@echo BENCH_CXXFLAGS=$(BENCH_CXXFLAGS)
	@echo BENCH_LFLAGS=$(BENCH_LFLAGS)

help:
	@echo Targets are: all info help conan clean distclean conan-clean

conanbuildinfo.args conanbuildinfo.mak conanbuildinfo.txt &: conanfile.txt
	conan install conanfile.txt && touch $@    # conan doesn't update file timestamp if no new contents were generated (even if conanfile.txt is newer)

conan: conanbuildinfo.mak

conan-clean:
	-rm conanbuildinfo.args conanbuildinfo.mak conanbuildinfo.txt conaninfo.txt conan.lock graph_info.json

tree_to_range_benchmark: tree_to_range.cc
	$(CXX) -g -O3 -std=c++20 -Wall -Wextra -DENABLE_BENCHMARK $(FMT_CXXFLAGS) $(BENCH_CXXFLAGS) -o $@ $< $(FMT_LFLAGS) $(BENCH_LFLAGS)

%: %.cc
	$(CXX) -g -Og -std=c++20 -Wall -Wextra $(FMT_CXXFLAGS) -o $@ $< $(FMT_LFLAGS)

.PHONY: all conan clean distclean conan-clean help

