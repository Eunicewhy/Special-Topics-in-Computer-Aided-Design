# compiler settings 
CXX = g++
CXXFLAGS = -I ./inc -std=c++14 -DPRINTTIME=true -DNDEBUG 
OPTFLAGS = -flto -funroll-loops -finline-functions -O3 -ffast-math
WARNINGS = -g -Wall

# Source files and object files
SRCDIR = src
OBJDIR = obj
INCDIR = inc
SRCS = $(wildcard $(SRCDIR)/*.cpp)
OBJS = $(patsubst $(SRCDIR)/%.cpp,$(OBJDIR)/%.o,$(SRCS))

# testcase file

CASENAME = c17# change here
CASEDIR = testcase/$(CASENAME)

NETLIST = $(CASENAME).v
LIBRARY = testcase/test_lib.lib
PATTERN = $(CASENAME).pat

TARGET = sta

all: $(TARGET)

$(OBJDIR):
	@mkdir $(OBJDIR)

$(TARGET): main.cpp $(OBJS)
	$(CXX) $(WARNINGS) $(CXXFLAGS) $(OPTFLAGS) $^ -o $@

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp | $(OBJDIR)
	$(CXX) $(WARNINGS) $(CXXFLAGS) $(OPTFLAGS) -c $< -o $@

run:
	time ./$(TARGET) $(CASEDIR)/$(NETLIST) -l $(LIBRARY) -i $(CASEDIR)/$(PATTERN)

clean:
	rm -rf $(OBJDIR) $(TARGET) *.txt

case0:
	./$(TARGET) c17.v -l test_lib.lib -i c17.pat
case1:
	./$(TARGET) c432.v -l test_lib.lib -i c432.pat
case2:
	./$(TARGET) c54321.v -l test_lib.lib -i c54321.pat
case3:
	./$(TARGET) c123456.v -l test_lib.lib -i c123456.pat
