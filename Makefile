CC := cl
CFLAGS := /nologo /W3

SRC_DIR := .
BUILD_DIR := build

SRCS := $(wildcard $(SRC_DIR)/*.c)
OBJS := $(addprefix $(BUILD_DIR)/,$(notdir $(SRCS:.c=.obj)))

TARGET := $(BUILD_DIR)/main.exe

SUPPRESS := @<nul set /p=
DEVIDER := ------------------------------

all: $(BUILD_DIR) $(TARGET)

runc: run sub_clean

run: all
	@echo $(DEVIDER)
	@$(TARGET)
	@echo:&
	@echo Program exited with code: %ERRORLEVEL%

$(TARGET): $(OBJS)
	@$(CC) $(CFLAGS) $(OBJS) /Fe$@

$(BUILD_DIR)/%.obj: $(SRC_DIR)/%.c
	$(SUPPRESS)" >> " & @$(CC) $(CFLAGS) /c $< /Fo$@
	
$(BUILD_DIR):
	@mkdir $(BUILD_DIR)

clean:
	-@rmdir /S /Q .\$(BUILD_DIR)
	cls

sub_clean:
	-@del /Q .\build\*.obj