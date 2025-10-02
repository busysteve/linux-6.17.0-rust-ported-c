// SPDX-License-Identifier: GPL-2.0

/*
 * Advanced Configuration and Power Interface abstractions.
 */

#include <linux/acpi.h>
#include <linux/string.h>
#include <linux/types.h>

#define ACPI_ID_LEN 16

// ACPI device id wrapper
typedef struct {
    struct acpi_device_id id;
} DeviceId;

// Create a new device id from an ACPI 'id' string.
static inline DeviceId DeviceId_new(const char *id)
{
    DeviceId dev_id;
    memset(&dev_id, 0, sizeof(dev_id));
    size_t len = strnlen(id, ACPI_ID_LEN - 1) + 1; // include nul
    if (len > ACPI_ID_LEN)
        len = ACPI_ID_LEN;
    memcpy(dev_id.id.id, id, len);
    return dev_id;
}

// Get the driver_data index from DeviceId
static inline unsigned long DeviceId_index(const DeviceId *dev_id)
{
    return dev_id->id.driver_data;
}

// Macro to create an ACPI IdTable with an alias for modpost
#define ACPI_DEVICE_TABLE(table_name, module_table_name, id_info_type, table_data) \
    static const struct acpi_device_id table_name[] = table_data; \
    MODULE_DEVICE_TABLE(acpi, module_table_name)
