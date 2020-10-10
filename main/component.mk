#
# Main component makefile.
#
# This Makefile can be left empty. By default, it will take the sources in the 
# src/ directory, compile them and link them into lib(subdirectory_name).a 
# in the build directory. This behaviour is entirely configurable,
# please read the ESP-IDF documents if you need to do this.
#

# If not unit test build, include main
# $(call compile_only_if,$(CONFIG_IS_UNIT_TEST),controlLoop.o)

CXXFLAGS += -std=c++14