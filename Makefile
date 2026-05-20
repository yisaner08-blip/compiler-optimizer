# 编译器优化器 - Makefile
CC       = gcc
CFLAGS   = -O2 -Wall -Wextra
SRCDIR   = src
BUILDDIR = build
OUTDIR   = output
LOGDIR   = log
TARGET   = $(BUILDDIR)/optimizer.exe
SOURCES  = $(SRCDIR)/main.c $(SRCDIR)/main_c_fixed.c

.PHONY: all clean test

all: $(TARGET)

$(TARGET): $(SOURCES)
	@mkdir -p $(BUILDDIR)
	$(CC) $(CFLAGS) -o $@ $^

test: $(TARGET)
	@mkdir -p $(OUTDIR)
	@echo "========== 运行测试 =========="
	@for i in 1 2 3 4; do \
		echo "--- input$$i ---"; \
		./$(TARGET) test/input/input$$i.txt $(OUTDIR)/output$$i.txt; \
		echo "  Diff vs expected:"; \
		diff -q test/expected/output$$i.txt $(OUTDIR)/output$$i.txt && echo "  PASS" || echo "  FAIL"; \
	done
	@echo "========== 测试完成 =========="

clean:
	@rm -rf $(BUILDDIR) $(LOGDIR) $(OUTDIR)
	@echo "清理完成"
