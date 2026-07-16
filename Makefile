# Cross-platform entry points. Python handles paths, directory creation and
# CubeIDE toolchain discovery, including native Windows paths with spaces.
.DEFAULT_GOAL := all
PYTHON ?= python
CONFIG ?= Release

.PHONY: all test flash
all:
	$(PYTHON) tools/build.py build --config $(CONFIG)
test:
	$(PYTHON) tools/build.py test
flash:
	$(PYTHON) tools/build.py flash --config $(CONFIG)
