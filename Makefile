CXXFLAGS = -shared -fPIC --no-gnu-unique -g -std=c++2b -O2
INCLUDES = `pkg-config --cflags pixman-1 libdrm hyprland pangocairo`
LIBS = `pkg-config --libs pangocairo`

SRC = ./src/main.cpp ./src/hyprmodoroDecoration.cpp ./src/HyprmodoroPassElement.cpp ./src/helpers.cpp ./src/renderer.cpp ./src/pomodoro.cpp
TARGET = hyprmodoro.so

all:
	@if ! pkg-config --exists hyprland; then \
		echo 'Hyprland headers not available. Run `make all && sudo make installheaders` in the root Hyprland directory.'; \
		exit 1; \
	fi
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(SRC) -o $(TARGET) $(LIBS)


clean:
	rm -f ./$(TARGET)

.PHONY: all clean