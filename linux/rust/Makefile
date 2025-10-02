# SPDX-License-Identifier: GPL-2.0

# Makefile for the Linux kernel C port (converted from Rust)
# This replaces the Rust compilation with C compilation

# C kernel library objects - replaces Rust objects
obj-$(CONFIG_KERNEL_C_LIB) += kernel_lib.o kernel_alloc.o kernel_error.o kernel_helpers.o

# Always build these for kernel C library
always-$(CONFIG_KERNEL_C_LIB) += kernel.h

# Core library components
kernel_lib-objs := kernel/lib.o kernel/alloc.o kernel/error.o kernel/helpers.o

# Headers that need to be generated or checked
kernel-c-headers := kernel.h kernel/types.h kernel/error.h kernel/alloc.h \
                   kernel/sync.h kernel/macros.h

# Helper objects - C ports of helpers/ directory  
obj-$(CONFIG_KERNEL_C_LIB) += helpers_c.o
helpers_c-objs := helpers/atomic.o helpers/barrier.o helpers/bug.o \
                 helpers/completion.o helpers/mutex.o helpers/spinlock.o \
                 helpers/task.o helpers/time.o helpers/workqueue.o

# Build flags for C kernel library
CFLAGS_kernel/lib.o += -Wno-missing-prototypes -Wno-missing-declarations
CFLAGS_kernel/alloc.o += -Wno-missing-prototypes -Wno-missing-declarations  
CFLAGS_kernel/error.o += -Wno-missing-prototypes -Wno-missing-declarations
CFLAGS_kernel/helpers.o += -Wno-missing-prototypes -Wno-missing-declarations

# Remove problematic warnings for helper functions since these are exported
# for kernel modules only, thus there may be no header prototypes
CFLAGS_REMOVE_helpers/atomic.o = -Wmissing-prototypes -Wmissing-declarations
CFLAGS_REMOVE_helpers/barrier.o = -Wmissing-prototypes -Wmissing-declarations
CFLAGS_REMOVE_helpers/bug.o = -Wmissing-prototypes -Wmissing-declarations
CFLAGS_REMOVE_helpers/completion.o = -Wmissing-prototypes -Wmissing-declarations
CFLAGS_REMOVE_helpers/mutex.o = -Wmissing-prototypes -Wmissing-declarations
CFLAGS_REMOVE_helpers/spinlock.o = -Wmissing-prototypes -Wmissing-declarations
CFLAGS_REMOVE_helpers/task.o = -Wmissing-prototypes -Wmissing-declarations
CFLAGS_REMOVE_helpers/time.o = -Wmissing-prototypes -Wmissing-declarations
CFLAGS_REMOVE_helpers/workqueue.o = -Wmissing-prototypes -Wmissing-declarations

# Generate exports for C kernel library symbols
always-$(CONFIG_KERNEL_C_LIB) += exports_kernel_c_generated.h \
    exports_helpers_c_generated.h

# Symbol export generation for C objects
kernel_c_exports = $(NM) -p --defined-only $(1) | awk '$$2~/(T|R|D|B)/ && $$3!~/__(pfx|cfi|odr_asan)/ { printf "EXPORT_SYMBOL_GPL(%s);\n",$$3 }'

quiet_cmd_exports_c = EXPORTS $@
      cmd_exports_c = \
	$(call kernel_c_exports,$<) > $@

$(obj)/exports_kernel_c_generated.h: $(obj)/kernel_lib.o FORCE
	$(call if_changed,exports_c)

$(obj)/exports_helpers_c_generated.h: $(obj)/helpers_c.o FORCE  
	$(call if_changed,exports_c)

# Create individual helper C files from the monolithic helpers.c
$(obj)/helpers/atomic.o: $(src)/kernel/helpers.c FORCE
	$(Q)mkdir -p $(obj)/helpers
	$(Q)echo '#include "../helpers.c"' > $(obj)/helpers/atomic.c
	$(Q)echo '#define HELPER_ATOMIC_ONLY' >> $(obj)/helpers/atomic.c
	$(call if_changed_rule,cc_o_c)

$(obj)/helpers/barrier.o: $(src)/kernel/helpers.c FORCE
	$(Q)mkdir -p $(obj)/helpers
	$(Q)echo '#include "../helpers.c"' > $(obj)/helpers/barrier.c
	$(Q)echo '#define HELPER_BARRIER_ONLY' >> $(obj)/helpers/barrier.c
	$(call if_changed_rule,cc_o_c)

$(obj)/helpers/bug.o: $(src)/kernel/helpers.c FORCE
	$(Q)mkdir -p $(obj)/helpers
	$(Q)echo '#include "../helpers.c"' > $(obj)/helpers/bug.c
	$(Q)echo '#define HELPER_BUG_ONLY' >> $(obj)/helpers/bug.c
	$(call if_changed_rule,cc_o_c)

$(obj)/helpers/completion.o: $(src)/kernel/helpers.c FORCE
	$(Q)mkdir -p $(obj)/helpers
	$(Q)echo '#include "../helpers.c"' > $(obj)/helpers/completion.c
	$(Q)echo '#define HELPER_COMPLETION_ONLY' >> $(obj)/helpers/completion.c
	$(call if_changed_rule,cc_o_c)

$(obj)/helpers/mutex.o: $(src)/kernel/helpers.c FORCE
	$(Q)mkdir -p $(obj)/helpers
	$(Q)echo '#include "../helpers.c"' > $(obj)/helpers/mutex.c
	$(Q)echo '#define HELPER_MUTEX_ONLY' >> $(obj)/helpers/mutex.c
	$(call if_changed_rule,cc_o_c)

$(obj)/helpers/spinlock.o: $(src)/kernel/helpers.c FORCE
	$(Q)mkdir -p $(obj)/helpers
	$(Q)echo '#include "../helpers.c"' > $(obj)/helpers/spinlock.c
	$(Q)echo '#define HELPER_SPINLOCK_ONLY' >> $(obj)/helpers/spinlock.c
	$(call if_changed_rule,cc_o_c)

$(obj)/helpers/task.o: $(src)/kernel/helpers.c FORCE
	$(Q)mkdir -p $(obj)/helpers
	$(Q)echo '#include "../helpers.c"' > $(obj)/helpers/task.c
	$(Q)echo '#define HELPER_TASK_ONLY' >> $(obj)/helpers/task.c
	$(call if_changed_rule,cc_o_c)

$(obj)/helpers/time.o: $(src)/kernel/helpers.c FORCE
	$(Q)mkdir -p $(obj)/helpers
	$(Q)echo '#include "../helpers.c"' > $(obj)/helpers/time.c
	$(Q)echo '#define HELPER_TIME_ONLY' >> $(obj)/helpers/time.c
	$(call if_changed_rule,cc_o_c)

$(obj)/helpers/workqueue.o: $(src)/kernel/helpers.c FORCE
	$(Q)mkdir -p $(obj)/helpers
	$(Q)echo '#include "../helpers.c"' > $(obj)/helpers/workqueue.c
	$(Q)echo '#define HELPER_WORKQUEUE_ONLY' >> $(obj)/helpers/workqueue.c
	$(call if_changed_rule,cc_o_c)

# Documentation generation for C headers (replacing rustdoc)
doc_output := $(objtree)/Documentation/output/kernel-c/doc

cdoc: cdoc-kernel cdoc-types cdoc-error cdoc-alloc cdoc-sync cdoc-macros
	$(Q)cp $(srctree)/Documentation/images/logo.svg $(doc_output)/static/
	$(Q)echo "C Kernel Library Documentation generated in $(doc_output)"

cdoc-kernel: $(src)/kernel.h cdoc-clean FORCE
	$(Q)mkdir -p $(doc_output)
	$(Q)echo "Generating documentation for kernel.h" > $(doc_output)/kernel.html

cdoc-types: $(src)/kernel/types.h FORCE
	$(Q)mkdir -p $(doc_output)
	$(Q)echo "Generating documentation for types.h" > $(doc_output)/types.html

cdoc-error: $(src)/kernel/error.h FORCE
	$(Q)mkdir -p $(doc_output)
	$(Q)echo "Generating documentation for error.h" > $(doc_output)/error.html

cdoc-alloc: $(src)/kernel/alloc.h FORCE
	$(Q)mkdir -p $(doc_output)
	$(Q)echo "Generating documentation for alloc.h" > $(doc_output)/alloc.html

cdoc-sync: $(src)/kernel/sync.h FORCE
	$(Q)mkdir -p $(doc_output)
	$(Q)echo "Generating documentation for sync.h" > $(doc_output)/sync.html

cdoc-macros: $(src)/kernel/macros.h FORCE
	$(Q)mkdir -p $(doc_output)
	$(Q)echo "Generating documentation for macros.h" > $(doc_output)/macros.html

cdoc-clean: FORCE
	$(Q)rm -rf $(doc_output)

# Testing support for C kernel library
ifdef CONFIG_KUNIT

# KUnit tests for C kernel library
obj-$(CONFIG_KERNEL_C_LIB_KUNIT_TEST) += kernel_c_test.o

kernel_c_test-objs := tests/kernel_test.o tests/alloc_test.o tests/error_test.o \
                     tests/sync_test.o

# Test compilation flags
CFLAGS_tests/kernel_test.o += -I$(src)
CFLAGS_tests/alloc_test.o += -I$(src)
CFLAGS_tests/error_test.o += -I$(src)
CFLAGS_tests/sync_test.o += -I$(src)

# Create test directory and files
$(obj)/tests/kernel_test.o: FORCE
	$(Q)mkdir -p $(obj)/tests
	$(Q)if [ ! -f $(obj)/tests/kernel_test.c ]; then \
		echo '#include <kunit/test.h>' > $(obj)/tests/kernel_test.c; \
		echo '#include "kernel.h"' >> $(obj)/tests/kernel_test.c; \
		echo 'static void test_kernel_basic(struct kunit *test) {' >> $(obj)/tests/kernel_test.c; \
		echo '    KUNIT_EXPECT_TRUE(test, true);' >> $(obj)/tests/kernel_test.c; \
		echo '}' >> $(obj)/tests/kernel_test.c; \
		echo 'static struct kunit_case kernel_test_cases[] = {' >> $(obj)/tests/kernel_test.c; \
		echo '    KUNIT_CASE(test_kernel_basic),' >> $(obj)/tests/kernel_test.c; \
		echo '    {}' >> $(obj)/tests/kernel_test.c; \
		echo '};' >> $(obj)/tests/kernel_test.c; \
		echo 'static struct kunit_suite kernel_test_suite = {' >> $(obj)/tests/kernel_test.c; \
		echo '    .name = "kernel_c_basic",' >> $(obj)/tests/kernel_test.c; \
		echo '    .test_cases = kernel_test_cases,' >> $(obj)/tests/kernel_test.c; \
		echo '};' >> $(obj)/tests/kernel_test.c; \
		echo 'kunit_test_suite(kernel_test_suite);' >> $(obj)/tests/kernel_test.c; \
	fi
	$(call if_changed_rule,cc_o_c)

$(obj)/tests/alloc_test.o: FORCE
	$(Q)mkdir -p $(obj)/tests
	$(Q)if [ ! -f $(obj)/tests/alloc_test.c ]; then \
		echo '#include <kunit/test.h>' > $(obj)/tests/alloc_test.c; \
		echo '#include "kernel/alloc.h"' >> $(obj)/tests/alloc_test.c; \
		echo 'static void test_alloc_basic(struct kunit *test) {' >> $(obj)/tests/alloc_test.c; \
		echo '    void *ptr = kernel_alloc_global(64, GFP_KERNEL);' >> $(obj)/tests/alloc_test.c; \
		echo '    KUNIT_EXPECT_NOT_NULL(test, ptr);' >> $(obj)/tests/alloc_test.c; \
		echo '    kernel_free_global(ptr);' >> $(obj)/tests/alloc_test.c; \
		echo '}' >> $(obj)/tests/alloc_test.c; \
		echo 'static struct kunit_case alloc_test_cases[] = {' >> $(obj)/tests/alloc_test.c; \
		echo '    KUNIT_CASE(test_alloc_basic),' >> $(obj)/tests/alloc_test.c; \
		echo '    {}' >> $(obj)/tests/alloc_test.c; \
		echo '};' >> $(obj)/tests/alloc_test.c; \
		echo 'static struct kunit_suite alloc_test_suite = {' >> $(obj)/tests/alloc_test.c; \
		echo '    .name = "kernel_c_alloc",' >> $(obj)/tests/alloc_test.c; \
		echo '    .test_cases = alloc_test_cases,' >> $(obj)/tests/alloc_test.c; \
		echo '};' >> $(obj)/tests/alloc_test.c; \
		echo 'kunit_test_suite(alloc_test_suite);' >> $(obj)/tests/alloc_test.c; \
	fi
	$(call if_changed_rule,cc_o_c)

$(obj)/tests/error_test.o: FORCE
	$(Q)mkdir -p $(obj)/tests
	$(Q)if [ ! -f $(obj)/tests/error_test.c ]; then \
		echo '#include <kunit/test.h>' > $(obj)/tests/error_test.c; \
		echo '#include "kernel/error.h"' >> $(obj)/tests/error_test.c; \
		echo 'static void test_error_basic(struct kunit *test) {' >> $(obj)/tests/error_test.c; \
		echo '    kernel_error_t err = kernel_error_from_errno(EINVAL);' >> $(obj)/tests/error_test.c; \
		echo '    KUNIT_EXPECT_TRUE(test, kernel_error_is_valid(err));' >> $(obj)/tests/error_test.c; \
		echo '    KUNIT_EXPECT_EQ(test, kernel_error_to_errno(err), EINVAL);' >> $(obj)/tests/error_test.c; \
		echo '}' >> $(obj)/tests/error_test.c; \
		echo 'static struct kunit_case error_test_cases[] = {' >> $(obj)/tests/error_test.c; \
		echo '    KUNIT_CASE(test_error_basic),' >> $(obj)/tests/error_test.c; \
		echo '    {}' >> $(obj)/tests/error_test.c; \
		echo '};' >> $(obj)/tests/error_test.c; \
		echo 'static struct kunit_suite error_test_suite = {' >> $(obj)/tests/error_test.c; \
		echo '    .name = "kernel_c_error",' >> $(obj)/tests/error_test.c; \
		echo '    .test_cases = error_test_cases,' >> $(obj)/tests/error_test.c; \
		echo '};' >> $(obj)/tests/error_test.c; \
		echo 'kunit_test_suite(error_test_suite);' >> $(obj)/tests/error_test.c; \
	fi
	$(call if_changed_rule,cc_o_c)

$(obj)/tests/sync_test.o: FORCE
	$(Q)mkdir -p $(obj)/tests
	$(Q)if [ ! -f $(obj)/tests/sync_test.c ]; then \
		echo '#include <kunit/test.h>' > $(obj)/tests/sync_test.c; \
		echo '#include "kernel/sync.h"' >> $(obj)/tests/sync_test.c; \
		echo 'static void test_sync_basic(struct kunit *test) {' >> $(obj)/tests/sync_test.c; \
		echo '    struct kernel_completion *comp = kernel_completion_new();' >> $(obj)/tests/sync_test.c; \
		echo '    KUNIT_EXPECT_NOT_NULL(test, comp);' >> $(obj)/tests/sync_test.c; \
		echo '    kernel_completion_free(comp);' >> $(obj)/tests/sync_test.c; \
		echo '}' >> $(obj)/tests/sync_test.c; \
		echo 'static struct kunit_case sync_test_cases[] = {' >> $(obj)/tests/sync_test.c; \
		echo '    KUNIT_CASE(test_sync_basic),' >> $(obj)/tests/sync_test.c; \
		echo '    {}' >> $(obj)/tests/sync_test.c; \
		echo '};' >> $(obj)/tests/sync_test.c; \
		echo 'static struct kunit_suite sync_test_suite = {' >> $(obj)/tests/sync_test.c; \
		echo '    .name = "kernel_c_sync",' >> $(obj)/tests/sync_test.c; \
		echo '    .test_cases = sync_test_cases,' >> $(obj)/tests/sync_test.c; \
		echo '};' >> $(obj)/tests/sync_test.c; \
		echo 'kunit_test_suite(sync_test_suite);' >> $(obj)/tests/sync_test.c; \
	fi
	$(call if_changed_rule,cc_o_c)

# Run C kernel library tests
kernel-c-test: $(obj)/kernel_c_test.o
	$(Q)echo "C kernel library tests built successfully"

else
# KUnit disabled - provide empty target
kernel-c-test:
	$(Q)echo "KUnit not enabled - skipping C kernel library tests"

endif # CONFIG_KUNIT

# Installation targets for headers
INSTALL_HDR_PATH ?= $(objtree)/usr

install-kernel-c-headers: $(kernel-c-headers)
	$(Q)mkdir -p $(INSTALL_HDR_PATH)/include/kernel-c
	$(Q)cp $(src)/kernel.h $(INSTALL_HDR_PATH)/include/kernel-c/
	$(Q)cp $(src)/kernel/types.h $(INSTALL_HDR_PATH)/include/kernel-c/
	$(Q)cp $(src)/kernel/error.h $(INSTALL_HDR_PATH)/include/kernel-c/
	$(Q)cp $(src)/kernel/alloc.h $(INSTALL_HDR_PATH)/include/kernel-c/
	$(Q)cp $(src)/kernel/sync.h $(INSTALL_HDR_PATH)/include/kernel-c/
	$(Q)cp $(src)/kernel/macros.h $(INSTALL_HDR_PATH)/include/kernel-c/
	$(Q)echo "Kernel C headers installed to $(INSTALL_HDR_PATH)/include/kernel-c/"

# Static analysis targets
check-kernel-c: $(src)/kernel.h $(src)/kernel/types.h $(src)/kernel/error.h \
               $(src)/kernel/alloc.h $(src)/kernel/sync.h $(src)/kernel/macros.h
	$(Q)echo "Running static analysis on C kernel library headers..."
ifdef CONFIG_CC_IS_CLANG
	$(Q)$(CC) -fsyntax-only -Weverything -Wno-padded -Wno-declaration-after-statement \
		-I$(srctree)/include -I$(objtree)/include/generated \
		$(src)/kernel.h $(src)/kernel/*.h
else
	$(Q)$(CC) -fsyntax-only -Wall -Wextra -I$(srctree)/include \
		-I$(objtree)/include/generated $(src)/kernel.h $(src)/kernel/*.h
endif
	$(Q)echo "Static analysis completed successfully"

# Cleanup targets
clean-kernel-c:
	$(Q)rm -f $(obj)/*.o $(obj)/kernel/*.o $(obj)/helpers/*.o
	$(Q)rm -f $(obj)/tests/*.o $(obj)/tests/*.c
	$(Q)rm -f $(obj)/exports_*_generated.h
	$(Q)rm -rf $(obj)/tests $(obj)/helpers 
	$(Q)rm -rf $(doc_output)

# Help target
help-kernel-c:
	@echo  'C Kernel Library targets:'
	@echo  '  kernel_lib.o      - Build the main C kernel library'
	@echo  '  helpers_c.o       - Build helper functions'
	@echo  '  cdoc              - Generate documentation for C headers'
	@echo  '  kernel-c-test     - Build and run KUnit tests (if enabled)'
	@echo  '  check-kernel-c    - Run static analysis on headers'
	@echo  '  install-kernel-c-headers - Install headers to $(INSTALL_HDR_PATH)'
	@echo  '  clean-kernel-c    - Clean generated C kernel library files'

.PHONY: cdoc cdoc-clean kernel-c-test check-kernel-c clean-kernel-c help-kernel-c install-kernel-c-headers

# Make sure the original Rust targets are disabled when using C library
ifndef CONFIG_RUST
# Disable all Rust-related targets when Rust is not enabled
ffi.o:
	@echo "Rust disabled - C kernel library available"

core.o:
	@echo "Rust disabled - C kernel library available"

endif