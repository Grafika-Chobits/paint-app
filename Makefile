# the compiler: gcc for C program, define as g++ for C++
CC = g++

# compiler flags:
#  -g    adds debugging information to the executable file
#  -Wall turns on most, but not all, compiler warnings
CFLAGS  = -g

# the build target executable:
TARGET = main

$(TARGET): colorpicker.cpp
	$(CC) -o $(TARGET) colorpicker.cpp -lpthread
	sudo ./$(TARGET)

clean:
	$(RM) $(TARGET) 
	
run:
	sudo ./$(TARGET)
