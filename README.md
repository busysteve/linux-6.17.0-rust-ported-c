# linux-6.17.0-rust-ported-c
A Sonnet driven port of the Linux Rust code to C.



Disk I/O bench test:
- Before: 2730.7 MB/s
- After:  2935.5 MB/s



More to come. 



# Rust to C Port - Kernel Module System

This directory contains a complete port of the Rust-for-Linux kernel module system to C. The port maintains the same API design and functionality while using standard C instead of Rust.

## Overview

The port converts:
- **Rust crates** → C libraries with headers
- **Rust traits** → C function pointer structures (vtables)
- **Rust procedural macros** → C preprocessor macros  
- **Rust modules** → C compilation units
- **Rust error handling** → C result structures and error codes
- **Rust memory management** → C allocation wrappers with RAII-style helpers

## File Structure

```
kernel.h                 - Main header (replaces kernel crate)
kernel/
├── types.h             - Core types (Arc, ARef, Opaque, etc.)
├── error.h             - Error handling and Result types
├── alloc.h             - Memory allocation (KBox, KVec, Layout)
├── sync.h              - Synchronization primitives
├── macros.h            - Preprocessor macros (module!, vtable, etc.)
├── lib.c               - Main library implementation
├── alloc.c             - Allocation implementation
├── error.c             - Error handling implementation
└── helpers.c           - Helper function implementations

Makefile.c              - Build system for C version
```

## Key Features

### 1. Module Declaration
**Rust:**
```rust
module! {
    type: MyModule,
    name: "my_module",
    author: "Author",
    description: "Description",
    license: "GPL",
}
```

**C:**
```c
KERNEL_MODULE_DECLARE(
    .name = "my_module",
    .author = "Author", 
    .description = "Description",
    .license = "GPL",
    .init_fn = my_module_init,
    .exit_fn = my_module_exit
);
```

### 2. Error Handling
**Rust:**
```rust
fn foo() -> Result<i32, Error> {
    Ok(42)
}

let result = foo()?;
```

**C:**
```c
KERNEL_RESULT(int) foo(void) {
    return KERNEL_OK_RESULT(42);
}

KERNEL_RESULT(int) result = foo();
if (IS_KERNEL_ERR(result)) {
    return KERNEL_ERR_RESULT(result.err);
}
int value = result.ok;
```

### 3. Memory Allocation  
**Rust:**
```rust
let kbox = KBox::new(data, GFP_KERNEL)?;
let vec = KVec::with_capacity(10, GFP_KERNEL)?;
```

**C:**
```c
struct kernel_kbox *kbox = kernel_kbox_new(sizeof(data), KERNEL_GFP_KERNEL);
struct kernel_kvec *vec = kernel_kvec_with_capacity(10, sizeof(elem), KERNEL_GFP_KERNEL);
```

### 4. Synchronization
**Rust:**
```rust
let mutex = Mutex::new(data);
let guard = mutex.lock();
```

**C:**
```c
struct kernel_mutex *mutex = kernel_mutex_new(data);
kernel_mutex_lock(mutex);
// ... use data ...
kernel_mutex_unlock(mutex);
```

### 5. VTable/Traits
**Rust:**
```rust
#[vtable]
trait Operations {
    fn operation(&self) -> Result<()>;
}
```

**C:**
```c
KERNEL_VTABLE_DECLARE(operations,
    int (*operation)(void *self);
);

KERNEL_VTABLE_IMPL(my_type, operations,
    .operation = my_operation_impl,
);
```

## Building

### Enable C Kernel Library
```bash
# In kernel config
CONFIG_KERNEL_C_LIB=y
CONFIG_KERNEL_C_LIB_KUNIT_TEST=y  # For tests
```

### Build Commands
```bash
# Build the C kernel library
make kernel_lib.o helpers_c.o

# Generate documentation  
make cdoc

# Run tests (if KUnit enabled)
make kernel-c-test

# Static analysis
make check-kernel-c

# Install headers
make install-kernel-c-headers

# Clean
make clean-kernel-c
```

### Using the Library
```c
#include "kernel.h"

static int my_module_init(struct module *module)
{
    pr_info("Module loaded\n");
    return 0;
}

static void my_module_exit(void)
{
    pr_info("Module unloaded\n");
}

KERNEL_MODULE_DECLARE(
    .name = "example_module",
    .author = "Developer",
    .description = "Example C kernel module",
    .license = "GPL",
    .version = "1.0",
    .init_fn = my_module_init,
    .exit_fn = my_module_exit
);
```

## API Compatibility

The C port maintains API compatibility with the Rust version:

| Rust Feature | C Equivalent | Notes |
|--------------|--------------|-------|
| `Result<T, Error>` | `KERNEL_RESULT(T)` | Struct with union and error flag |
| `Arc<T>` | `struct kernel_arc` | Reference counted pointer |
| `Mutex<T>` | `struct kernel_mutex` | Mutex with data pointer |
| `Vec<T>` | `struct kernel_kvec` | Dynamic array |
| `Box<T>` | `struct kernel_kbox` | Heap allocated box |
| `#[vtable]` | `KERNEL_VTABLE_DECLARE` | Function pointer structures |
| `module!` | `KERNEL_MODULE_DECLARE` | Module declaration macro |
| `container_of!` | `KERNEL_CONTAINER_OF` | Offset calculation |

## Testing

The port includes KUnit tests that mirror the original Rust tests:

```bash
# Run all tests
make kernel-c-test

# Individual test modules
CONFIG_KERNEL_C_LIB_KUNIT_TEST=y
```

Test categories:
- Basic kernel functionality
- Memory allocation and deallocation
- Error handling and propagation  
- Synchronization primitives
- Container operations

## Performance

The C port provides equivalent performance to the Rust version:
- No additional allocation overhead
- Direct kernel API access
- Optimized by existing kernel build system
- Compatible with all kernel debugging tools

## Migration Guide

To migrate from Rust to C:

1. Replace `use kernel::prelude::*` with `#include "kernel.h"`
2. Convert `Result<T>` returns to `KERNEL_RESULT(T)`
3. Replace `?` operator with `KERNEL_TRY` macro
4. Convert trait definitions to vtable declarations
5. Update module declarations to use `KERNEL_MODULE_DECLARE`
6. Replace Rust allocation calls with C equivalents

## Limitations

Current limitations compared to Rust:
- No compile-time memory safety guarantees
- Manual memory management required
- No automatic trait bounds checking
- Limited generic programming support
- C preprocessor instead of procedural macros

## Future Work

Planned improvements:
- Enhanced static analysis integration
- Better documentation generation
- Additional synchronization primitives
- Extended testing framework
- Performance optimizations

## Contributing

When adding new features:
1. Follow kernel C coding style
2. Add corresponding header declarations
3. Include KUnit tests
4. Update documentation
5. Maintain API compatibility


