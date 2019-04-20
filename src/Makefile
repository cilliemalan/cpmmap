

CC = gcc
CXX = g++
BUILD_DIR = build

INCLUDES = \
-I../include \
-I../googletest/googletest/include \
-I../googletest/googletest

DEFINES =

CFLAGS = -Wall -g $(INCLUDES) $(DEFINES)
CFLAGS += -MMD -MP -MF"$(@:%.o=%.d)"
LDFLAGS = -pthread -g -lexplain

CPP_SOURCES = \
basictests.cpp
CC_SOURCES = \
../googletest/googletest/src/gtest-all.cc \
../googletest/googletest/src/gtest_main.cc

OBJECTS += $(addprefix $(BUILD_DIR)/,$(notdir $(CPP_SOURCES:.cpp=.o)))
OBJECTS += $(addprefix $(BUILD_DIR)/,$(notdir $(CC_SOURCES:.cc=.o)))
vpath %.cpp $(sort $(dir $(CPP_SOURCES)))
vpath %.cc $(sort $(dir $(CC_SOURCES)))

all: $(BUILD_DIR)/runtests

clean:
	rm -rf $(BUILD_DIR)


$(BUILD_DIR)/%.o: %.cc Makefile | $(BUILD_DIR)
	@echo "CC $<"
	@$(CXX) -c $(CFLAGS) $< -o $@

$(BUILD_DIR)/%.o: %.cpp Makefile | $(BUILD_DIR)
	@echo "CC $<"
	@$(CXX) -c $(CFLAGS) $< -o $@

$(BUILD_DIR):
	mkdir $(BUILD_DIR)

$(BUILD_DIR)/runtests: $(OBJECTS) Makefile
	@echo "LN $@"
	@$(CXX) $(OBJECTS) $(LDFLAGS) -o $@


-include $(wildcard $(BUILD_DIR)/*.d)