LLVMDIR:=/home/axel/build/llvm-orig/deb-inst/bin/
#LLVMSRCDIR:=/Users/axel/build/llvm/src

LLVMCFG:=$(LLVMDIR)/llvm-config
CLANG:=$(LLVMDIR)/clang++
LLVMLIBS:=$(shell $(LLVMCFG) --ldflags --libs orcjit runtimedyld irreader native) -lcurses -lz -pthread -ldl
LLVMCXXFLAGS:=$(shell $(LLVMCFG) --cppflags) -std=c++11 -fno-rtti -g #-I$(LLVMSRCDIR)/

ppcreloc.exe: main_orcrepl.cxx
	$(CLANG) $(LLVMCXXFLAGS) $^ $(LLVMLIBS) -o $@
