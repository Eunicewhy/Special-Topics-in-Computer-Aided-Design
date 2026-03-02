.PHONY: all clean

CXX = g++
CXX_FLAG = -Wall -Wextra -std=c++14 -g
INCLUDE_PATH = -Iheader

OBJECT_DIRECTORY = object

SOURCE_DIRECTORY = src

SOURCE = $(wildcard $(SOURCE_DIRECTORY)/*.cpp)
OBJECT = $(patsubst $(SOURCE_DIRECTORY)/%.cpp, $(OBJECT_DIRECTORY)/%.o, $(SOURCE))
DEPENDENCY = $(patsubst $(SOURCE_DIRECTORY)/%.cpp, $(OBJECT_DIRECTORY)/%.d, $(SOURCE))

TARGET = sop

all: $(TARGET)

$(OBJECT_DIRECTORY):
	mkdir $(OBJECT_DIRECTORY)

$(TARGET): main.cpp $(OBJECT)
	$(CXX) $(CXX_FLAG) $(INCLUDE_PATH) $^ -o $@

-include $(DEPENDENCY)

$(OBJECT_DIRECTORY)/%.o: $(SOURCE_DIRECTORY)/%.cpp | $(OBJECT_DIRECTORY)
	$(CXX) $(CXX_FLAG) $(INCLUDE_PATH) -MMD -c $< -o $@

clean:
	rm -rf $(TARGET) $(OBJECT_DIRECTORY)

case0:
	./$(TARGET) case0.txt case0_out.txt

case00:
	./$(TARGET) case00.txt case00_out.txt

case1:
	./$(TARGET) case1.txt case1_out.txt

case2:
	./$(TARGET) case2.txt case2_out.txt

case4:
	./$(TARGET) case4.txt case4_out.txt

case22:
	./$(TARGET) bit22.txt case22_out.txt

case23:
	./$(TARGET) bit23.txt case23_out.txt

case24:
	./$(TARGET) bit24.txt case24_out.txt

check0:
	python3 checker.py case0.txt case0_out.txt

check00:
	python3 checker.py case00.txt case00_out.txt

check1:
	python3 checker.py case1.txt case1_out.txt

check2:
	python3 checker.py case2.txt case2_out.txt

check4:
	python3 checker.py case4.txt case4_out.txt

check22:
	python3 checker.py bit22.txt case22_out.txt

check23:
	python3 checker.py bit23.txt case23_out.txt

check24:
	python3 checker.py bit24.txt case24_out.txt
