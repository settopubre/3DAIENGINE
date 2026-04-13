CXX = g++
CXXFLAGS = -Wall -Wextra -std=c++17 -march=native -O2 -pipe -flto -fopenmp
LDFLAGS = -lm -fopenmp
TARGET = 3dai_engine
SRCDIR = src
OBJDIR = obj
SOURCES = $(SRCDIR)/main.cpp
OBJECTS = $(OBJDIR)/main.o $(SRCDIR)/vector3d.o $(SRCDIR)/graph_engine.o

all: $(TARGET)
	@echo "=========================================="
	@echo "  BUILD COMPLETE: ./$(TARGET) is ready   "
	@echo "=========================================="

$(TARGET): $(OBJECTS)
	@echo "[LINK] Creating executable..."
	$(CXX) $(CXXFLAGS) $(OBJECTS) -o $(TARGET) $(LDFLAGS)

$(OBJDIR)/main.o: $(SRCDIR)/main.cpp include/graph_engine.h include/vector3d.h
	@echo "[CXX] Compiling main.cpp..."
	@mkdir -p $(OBJDIR)
	$(CXX) $(CXXFLAGS) -I./include -c $(SRCDIR)/main.cpp -o $(OBJDIR)/main.o

$(SRCDIR)/vector3d.o: $(SRCDIR)/vector3d.cpp include/vector3d.h
	@echo "[CXX] Compiling vector3d.cpp..."
	$(CXX) $(CXXFLAGS) -I./include -c $(SRCDIR)/vector3d.cpp -o $(SRCDIR)/vector3d.o

$(SRCDIR)/graph_engine.o: $(SRCDIR)/graph_engine.cpp include/graph_engine.h include/vector3d.h
	@echo "[CXX] Compiling graph_engine.cpp..."
	$(CXX) $(CXXFLAGS) -I./include -c $(SRCDIR)/graph_engine.cpp -o $(SRCDIR)/graph_engine.o

run: $(TARGET)
	@echo "[RUN] Executing 3DAI Engine..."
	@./$(TARGET)

viz: $(TARGET)
	@echo "[VIZ] Running 3DAI Engine pipeline..."
	@./$(TARGET)
	@echo "[VIZ] Rendering with R..."
	@Rscript scripts/visualize.R
	@echo "[VIZ] Complete. View: output/network_3d.png"

clean:
	@echo "[CLEAN] Removing object files and executable..."
	rm -f $(SRCDIR)/vector3d.o $(SRCDIR)/graph_engine.o
	rm -rf $(OBJDIR)
	rm -f $(TARGET)
	@echo "[CLEAN] Removing generated CSV and PNG files..."
	rm -f output/*.csv output/*.png
	@echo "[CLEAN] Done."

.PHONY: all clean run viz
