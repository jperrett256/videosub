CXX := cl
BUILD_DIR := build/

SRC_DIR := src/

SRC_FILE := main.c

EXE_FILE = $(patsubst %.c,$(BUILD_DIR)%.exe,$(SRC_FILE))

INC_DIR := inc/

COMPILER_OPTIONS = /Fo$(BUILD_DIR) /Fe$(EXE_FILE) /Fd$(BUILD_DIR) /I$(INC_DIR) /nologo /WX /W4 /wd4100 /wd4201 /wd4245 /wd4189 /wd4459

COMPILER_OPTIONS_DEBUG := $(COMPILER_OPTIONS) /DEBUG:FULL /Zi
COMPILER_OPTIONS_RELEASE := $(COMPILER_OPTIONS)

LINKER_OPTIONS = /link user32.lib gdi32.lib wininet.lib /subsystem:windows /MANIFEST:EMBED "/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'"

# comctl32.dll uxtheme.dll
default: debug

debug: build_dir
	$(CXX) $(COMPILER_OPTIONS_DEBUG) $(SRC_DIR)$(SRC_FILE) $(LINKER_OPTIONS)

release: build_dir
	$(CXX) $(COMPILER_OPTIONS_RELEASE) $(SRC_DIR)$(SRC_FILE) $(LINKER_OPTIONS)

build_dir:
	@mkdir -p $(BUILD_DIR)

run:
	./$(EXE_FILE)

clean:
	rm -r $(BUILD_DIR)
