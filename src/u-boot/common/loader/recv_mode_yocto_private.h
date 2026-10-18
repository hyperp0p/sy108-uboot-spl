/*
 * (C) Copyright 2019
 * Stefano Babic, DENX Software Engineering, sbabic@denx.de.
 *
 * SPDX-License-Identifier:     LGPL-2.1-or-later
 */

#pragma once

#include <stdlib.h>
#include "queue.h"

typedef enum {
    TYPE_ATTR_STRING,   /* default */
    TYPE_ATTR_DECIMAL,
    TYPE_ATTR_HEX,
    TYPE_ATTR_BOOL,
    TYPE_ATTR_IP,
    TYPE_ATTR_MAC
} type_attribute;

typedef enum {
    ACCESS_ATTR_ANY,    /* default */
    ACCESS_ATTR_READ_ONLY,
    ACCESS_ATTR_WRITE_ONCE,
    ACCESS_ATTR_CHANGE_DEFAULT,
} access_attribute;

/**
 * U-Boot environment should always be redundant, but
 * for compatibility reasons a single copy must
 * be also supported. Structure is different because
 * there is no flags in the single copy
 */
struct uboot_env_noredund {
    /** computed crc32 value */
    uint32_t crc;
    /** placeholder to point to the env in flash */
    char data[];
};

/** Internal structure for an environment variable
 */
struct var_entry {
    /** Variable's name */
    char *name;
    /** Variable's value */
    char *value;
    /** Type of the variable, see access_attribute */
    type_attribute type;
    /** Permissions for the variable */
    access_attribute access;
    /** Pointer to next element in the list */
    LIST_ENTRY(var_entry) next;
};

LIST_HEAD(vars, var_entry);

/** libubootenv context
 */
struct uboot_ctx {
    /** true if the environment is redundant */
    //bool redundant;
    /** set to valid after a successful load */
    //bool valid;
    /** size of the environment */
    size_t size;
    /** devices where environment is stored */
    //struct uboot_flash_env envdevs[2];
    /** Set which device contains the current(last valid) environment */
    //int current;
    /** semaphore on the environment */
    //int lock;
    /** pointer to the internal db */
    struct vars varlist;
};
