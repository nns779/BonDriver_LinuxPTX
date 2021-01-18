CXX := g++
INCLUDES := -Iinclude
CPPFLAGS := -MMD
CXXFLAGS := -std=c++17 $(INCLUDES) -O2 -Wall -pthread -fPIC
LDFLAGS := -shared
LDLIBS := -lm

TARGET := BonDriver_LinuxPTX.so
OBJS := src/BonDriver_LinuxPTX.o src/config.o src/char_code_conv.o src/io_queue.o src/util.o
DEPS := $(OBJS:.o=.d)

all: $(TARGET)

clean:
	$(RM) -v $(TARGET) $(OBJS) $(DEPS)

$(TARGET): $(OBJS)
	$(CXX) $(LDFLAGS) -o $@ $^ $(LDLIBS)

-include $(DEPS)

.PHONY: all clean
