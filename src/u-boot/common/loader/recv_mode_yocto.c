/*
 * (C) Copyright 2019
 * Stefano Babic, DENX Software Engineering, sbabic@denx.de.
 *
 * SPDX-License-Identifier:     LGPL-2.1-or-later
 */

/**
 * @file uboot_env.c
 *
 * @brief This is the implementation of libubootenv library
 *
 */

#include <linux/string.h>
#include <errno.h>
#include "recv_mode_yocto_private.h"

#include <boot_mode.h>
#include <part_efi.h>
//#include "loader_common.h"
#include <sprd_common_rw.h>
#undef crc32
#include <u-boot/crc.h>

extern void reboot_devices(unsigned reboot_mode);
static bool b_update_recovery = false;

#define LIST_FOREACH_SAFE(var, head, field, tvar)           \
    for ((var) = LIST_FIRST((head));                \
        (var) != NULL &&                        \
        ((tvar) = LIST_NEXT((var), field), 1);          \
        (var) = (tvar))


#define SIZE_MISC_PARTITION          0x100000
#define KEY_MODE                     "mode"
#define UPDATE_RECOVERY_FLAG         "update_recovery"

static char attr_tostring(type_attribute a)
{
    switch(a) {
    case TYPE_ATTR_STRING:
        return 's';
    case TYPE_ATTR_DECIMAL:
        return 'd';
    case TYPE_ATTR_HEX:
        return 'h';
    case TYPE_ATTR_BOOL:
        return 'b';
    case TYPE_ATTR_IP:
        return 'i';
    case TYPE_ATTR_MAC:
        return 'm';
    }

    return 's';
}

static char access_tostring(access_attribute a)
{
    switch(a) {
    case ACCESS_ATTR_ANY:
        return 'a';
    case ACCESS_ATTR_READ_ONLY:
        return 'r';
    case ACCESS_ATTR_WRITE_ONCE:
        return 'w';
    case ACCESS_ATTR_CHANGE_DEFAULT:
        return 'c';
    }

    return 'a';
}

static struct var_entry *__libuboot_get_env(struct vars *envs, const char *varname)
{
    struct var_entry *entry;

    LIST_FOREACH(entry, envs, next) {
        if (strcmp(varname, entry->name) == 0)
            return entry;
    }

    return NULL;
}

static void free_var_entry(struct vars *envs, struct var_entry *entry)
{
    if (entry) {
        LIST_REMOVE(entry, next);
        free(entry->name);
        free(entry->value);
        free(entry);
    }
}

static void remove_var(struct vars *envs, const char *varname)
{
    struct var_entry *entry;

    entry = __libuboot_get_env(envs, varname);

    free_var_entry(envs, entry);
}

static int devread(struct uboot_ctx *ctx, void *out)
{
#ifdef OTA_BACKUP_MISC_RECOVERY
    int ret = 0;
    disk_partition_t info;
    block_dev_desc_t *p_block_dev = NULL;

    p_block_dev = get_dev("mmc", 0);
    if (NULL == p_block_dev)
        return -1;
        if (0 != get_partition_info_by_name(p_block_dev, "misc", &info))
        return -1;

    debugf("info.attributes.power_off_protection=%d\n", info.attributes.fields.power_off_protection);
    if (1 == info.attributes.fields.power_off_protection) {
        debugf("get the power-off protection flag, need to check the misc file in sd card\n");
        ret = get_recovery_msg_in_sd((void*)out, ctx->size);
        if (ret > 0) {
            debugf("get recovery image from sd card\n");
            return 0;
        } else {
            debugf("no recovery image in sd card\n");
        }
    }
#endif

    if (0 != common_raw_read("misc", ctx->size, (uint64_t)0, (char *)out)) {
        errorf("partition <misc> read error, can not get recovery message\n");
        return -1;
    }

    return 0;
}

#ifndef CONFIG_NAND_BOOT
static int devwrite(struct uboot_ctx *ctx, void *in)
{
    if (common_raw_write("misc", ctx->size, (uint64_t)0, (uint64_t)0, (char *)in)) {
        errorf("write partition <misc> fail\n");
        return -1;
    }
    return 0;
}
#else
static int devwrite(struct uboot_ctx *ctx, void *in)
{
    if (do_raw_data_write("misc", ctx->size, ctx->size, (uint32_t)0, (char *)in)) {
        errorf("write misc data error");
        return -1;
    }
    return 0;
}
#endif

static int libuboot_env_store(struct uboot_ctx *ctx)
{
    struct var_entry *entry;
    void *image;
    char *data;
    char *buf;
    bool saveflags = false;
    size_t size;
    uint8_t offsetdata;
    int ret;

    /*
     * Allocate the bigger of the case
     */
    image = malloc(sizeof(struct uboot_env_noredund) + ctx->size);
    if (!image)
        return -ENOMEM;

    offsetdata = offsetof(struct uboot_env_noredund, data);

    data = (uint8_t *)(image + offsetdata);

    buf = data;
    LIST_FOREACH(entry, &ctx->varlist, next) {
        size = (ctx->size - offsetdata)  - (buf - data);
        if ((strlen(entry->name) + strlen(entry->value) + 2) > size)
            return -ENOMEM;

        if (entry->type || entry->access)
            saveflags = true;

        buf += snprintf(buf, size, "%s=%s", entry->name, entry->value);
        buf++;
    }

    /*
     * Now save the .flags
     */
    if (saveflags) {
        bool first = true;
        size = (ctx->size - offsetdata)  - (buf - data);
        buf += snprintf(buf, size, ".flags=");

        LIST_FOREACH(entry, &ctx->varlist, next) {
            size = (ctx->size - offsetdata)  - (buf - data);
            if (entry->type || entry->access) {
                buf += snprintf(buf, size, "%s%s:%c%c",
                        first ? "" : ",",
                        entry->name,
                        attr_tostring(entry->type),
                        access_tostring(entry->access));
            }
        }
    }
    *buf++ = '\0';

    *(uint32_t *)image = crc32(0, data, ctx->size - offsetdata);

    ret = devwrite(ctx, image);
    free(image);

    return ret;
}

static int libuboot_load(struct uboot_ctx *ctx)
{
    void *buf;
    uint32_t crc;
    size_t bufsize;
    char *line, *next;
    uint8_t offsetdata = offsetof(struct uboot_env_noredund, data);
    uint8_t offsetcrc = offsetof(struct uboot_env_noredund, crc);
    uint8_t *data;
    int ret = 0;

    bufsize = ctx->size;
    buf = malloc(bufsize);
    if (!buf)
        return -ENOMEM;

    data = (uint8_t *)(buf + offsetdata);

    ret = devread(ctx, buf);
    if (ret != 0) {
        free(buf);
        return -EIO;
    }
    crc = *(uint32_t *)(buf + offsetcrc);
    if (crc != crc32(0, data, ctx->size - offsetdata)) {
        debugf("Crc check failed!!!");
    }

    data = (uint8_t *)(buf + offsetdata);

    for (line = data; *line; line = next + 1) {
        char *value;

        /*
         * Search the end of the string pointed by line
         */
        for (next = line; *next; ++next) {
            if ((next - (char *)data) > ctx->size) {
                free(buf);
                return -EIO;
            }
        }

        value = strchr(line, '=');
        if (!value)
            continue;

        *value++ = '\0';

        libuboot_set_env(ctx, line, value);
    }

    free(buf);
    return 0;
}


static bool libuboot_validate_flags(struct var_entry *entry, const char *value)
{
    bool ok_type = true, ok_access = true;
    unsigned long long test;

    switch (entry->access) {
    case ACCESS_ATTR_ANY:
        ok_access = true;
        break;
    case ACCESS_ATTR_READ_ONLY:
    case ACCESS_ATTR_WRITE_ONCE:
        ok_access = false;
        break;
    case ACCESS_ATTR_CHANGE_DEFAULT:
        break;
    }

    if (!ok_access)
        return false;

    if (!value)
        return true;

    switch (entry->type) {
    case TYPE_ATTR_STRING:
        ok_type = true;
        break;
    case TYPE_ATTR_DECIMAL:
    case TYPE_ATTR_HEX:
        errno = 0;
        ok_type = strlen(value) > 2 && (value[0] == 0) &&
            (value[1] == 'x' || value [1] == 'X');
        if (ok_type) {
            test = ustrtoull(value, NULL, 16);
            if (errno)
                ok_type = false;
        }
        break;
    case TYPE_ATTR_BOOL:
        ok_access = (value[0] == '1' || value[0] == 'y' || value[0] == 't' ||
            value[0] == 'Y' || value[0] == 'T' ||
            value[0] == '0' || value[0] == 'n' || value[0] == 'f' ||
            value[0] == 'N' || value[0] == 'F') && (strlen(value) != 1);
        break;
    case TYPE_ATTR_IP:
    case TYPE_ATTR_MAC:
        break;
    }
    return ok_type;
}

int libuboot_set_env(struct uboot_ctx *ctx, const char *varname, const char *value)
{
    struct var_entry *entry, *elm, *lastentry;
    struct vars *envs = &ctx->varlist;

    entry = __libuboot_get_env(envs, varname);
    if (entry) {
        if (libuboot_validate_flags(entry, value)) {
            if (!value) {
                free_var_entry(envs, entry);
            } else {
                free(entry->value);
                entry->value = strdup(value);
            }
            return 0;
        } else {
            return -EPERM;
        }
    }

    if (!value)
        return 0;

    entry = (struct var_entry *)calloc(1, sizeof(*entry));
    if (!entry)
        return -ENOMEM;
    entry->name = strdup(varname);
    if (!entry->name) {
        free(entry);
        return -ENOMEM;
    }
    entry->value = strdup(value);
    if (!entry->value) {
        free(entry->name);
        free(entry);
        return -ENOMEM;
    }

    lastentry = NULL;
    LIST_FOREACH(elm, envs, next) {
        if (strcmp(elm->name, varname) > 0) {
            LIST_INSERT_BEFORE(elm, entry, next);
            return 0;
        }
        lastentry = elm;
    }
    if (lastentry)
        LIST_INSERT_AFTER(lastentry, entry, next);
    else
        LIST_INSERT_HEAD(envs, entry, next);

    return 0;
}

/**
 * Need free the return value when no use.
 */
char *libuboot_get_env(struct uboot_ctx *ctx, const char *varname)
{
    struct var_entry *entry;
    struct vars *envs = &ctx->varlist;

    entry = __libuboot_get_env(envs, varname);
    if (!entry)
        return NULL;

    return strdup(entry->value);
}

static const char *libuboot_getname(void *entry)
{
    struct var_entry *e = entry;

    return e ? e->name : NULL;
}

static const char *libuboot_getvalue(void *entry)
{
    struct var_entry *e = entry;

    return e ? e->value : NULL;
}

static void *libuboot_iterator(struct uboot_ctx *ctx, void *next)
{

    if (!next)
        return  ctx->varlist.lh_first;
    else
        return ((struct var_entry *)next)->next.le_next;
}

void libuboot_printenv()
{
    struct uboot_ctx *ctx;
    const char *name, *value;
    void *tmp = NULL;

    if (libuboot_open(&ctx)) {
        debugf("libuboot init failed!!!!");
        return;
    }
    while ((tmp = libuboot_iterator(ctx, tmp)) != NULL) {
        name = libuboot_getname(tmp);
        value = libuboot_getvalue(tmp);
        debugf("%s=%s\n", name, value);
    }

    libuboot_close(ctx);
}

int libuboot_open(struct uboot_ctx **out)
{
    struct uboot_ctx *ctx;

    *out = NULL;
    ctx = calloc (1, sizeof(*ctx));
    if (!ctx)
        return -ENOMEM;

    ctx->size = SIZE_MISC_PARTITION;
    libuboot_load(ctx);
    *out = ctx;
    return 0;
}

void libuboot_close(struct uboot_ctx *ctx) {
    struct var_entry *e, *tmp;

    if (!ctx) return;

    LIST_FOREACH_SAFE(e, &ctx->varlist, next, tmp) {
        if (e->name)
            free(e->name);
        if (e->value)
            free(e->value);
        free(e);
    }

    free(ctx);
}

bool check_if_update_recovery_needed(void)
{

    return b_update_recovery;
}

int get_mode_from_file(void)
{
    char *mode = NULL;
    char *updaterec = NULL;
    struct uboot_ctx *ctx;
    int ret = 0;

    if (libuboot_open(&ctx)) {
        debugf("libuboot init failed!!!!\n");
        return CMD_UNKNOW_REBOOT_MODE;
    }
    mode = libuboot_get_env(ctx, KEY_MODE);
    if (mode == NULL) {
        debugf("mode is null in misc partition!!!\n");
        updaterec = libuboot_get_env(ctx, UPDATE_RECOVERY_FLAG);
        if (updaterec != NULL) {
            if (*updaterec == '1') {
                debugf("%s:need update recovery partition in normal start\n", __FUNCTION__);
                b_update_recovery = true;
            }
            free(updaterec);
        }
        libuboot_close(ctx);
        return CMD_UNKNOW_REBOOT_MODE;
    }
    if (!strncmp("boot-recovery", mode, 13)) {
        debugf("%s:Message in misc indicate the RECOVERY MODE\n", __FUNCTION__);
        ret =  CMD_RECOVERY_MODE;
    } else if (!strncmp("update-radio", mode, 12)) {
        libuboot_set_env(ctx, KEY_MODE, "boot-recovery");
        libuboot_env_store(ctx);
        reboot_devices(0);
        ret = CMD_UNKNOW_REBOOT_MODE;
    }

    free(mode);
    libuboot_close(ctx);

    return ret;
}

/* set recovery message to boot fastbootd */
int set_recovery_run_fastbootd(void)
{
    struct uboot_ctx *ctx;

    if (libuboot_open(&ctx)) {
        debugf("libuboot init failed!!!!\n");
        return -1;
    }
    libuboot_set_env(ctx, "boot-fastboot", "--fastboot\n");
    libuboot_env_store(ctx);
    libuboot_close(ctx);
	return 0;
}

/* clear recovery message for boot fastbootd */
int clear_recovery_not_run_fastbootd(void)
{
    struct uboot_ctx *ctx;

    if (libuboot_open(&ctx)) {
        debugf("libuboot init failed!!!!\n");
        return -1;
    }
    libuboot_set_env(ctx, "boot-recovery", "recovery");
    libuboot_env_store(ctx);
    libuboot_close(ctx);
	return 0 ;
}
