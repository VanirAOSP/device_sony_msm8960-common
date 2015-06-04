/*
 * Copyright (C) 2013 Sony Mobile Communications AB
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/* ===================================================================== */
/* === Module Debug === */
#define LOG_NDEBUG 0
#define LOG_TAG "libals-as3677"

/* ===================================================================== */
/* === Module Libraries === */
#include <cutils/log.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>

/* ===================================================================== */
/* === Module Hardware === */
#include "sony_lights.h"

/* ===================================================================== */
/* === Module Variables === */
static int als_enabled = 0;

/* ===================================================================== */
/* === Module write_enable === */
static int
write_enable(char const* path, int enabled)
{
    int fd;
    fd = open(path, O_RDWR);
    if (fd >= 0) {
        char buffer[20];
        int bytes = sprintf(buffer, "%d\n", enabled ? 1 : 0);
        int amt = write(fd, buffer, bytes);
        close(fd);
        if (amt <= 0)
            return -errno;
        als_enabled = enabled;
        return 0;
    } else {
        ALOGE("write_enable failed to open %s\n", path);
        return -errno;
    }
}

/* ===================================================================== */
/* === Module write_string === */
static int
write_string(char const* path, const char* value)
{
    int fd;
    fd = open(path, O_RDWR);
    if (fd >= 0) {
        char buffer[20];
        int bytes = snprintf(buffer, sizeof(buffer), "%s\n", value);
        int amt = write(fd, buffer, bytes);
        close(fd);
        return (amt == -1 ? -errno : 0);
    } else {
        ALOGE("write_string failed to open %s\n", path);
        return -errno;
    }
}

/* ===================================================================== */
/* === Module als_result_read === */
static int
als_result_read(void)
{
    int fd;
    fd = open(AS3677_ALS_LUX_FILE, O_RDONLY);
    if (fd >= 0) {
        char buffer[12];
        int amt = read(fd, buffer, sizeof(buffer) -1);
        close(fd);
        if (amt <= 0)
            return -errno;
        buffer[amt] = 0;
        return strtol(buffer, 0, 0);
    } else {
        ALOGE("als_result_read failed to open %s\n", AS3677_ALS_LUX_FILE);
        return -errno;
    }
}

/* ===================================================================== */
/* === Module als_available === */
int
als_available(void)
{
    ALOGV("als_available");
    return 0;
}

/* ===================================================================== */
/* === Module als_enable === */
int
als_enable(void)
{
    ALOGV("als_enable");
    int err = 0;
    err |= write_enable(AS3677_ALS_ENABLE_FILE, 1);
    err |= write_enable(AS3677_ALS_LCD1_GROUP_FILE, 1);
    err |= write_enable(AS3677_ALS_LCD2_GROUP_FILE, 1);
    err |= write_string(AS3677_ALS_GROUP1_FILE, ALS_LCD_LEVELS_GROUP1);
    return err;
}

/* ===================================================================== */
/* === Module als_disable === */
int
als_disable(void)
{
    ALOGV("als_disable");
    int err = 0;
    err |= write_enable(AS3677_ALS_ENABLE_FILE, 0);
    err |= write_enable(AS3677_ALS_LCD1_GROUP_FILE, 0);
    err |= write_enable(AS3677_ALS_LCD2_GROUP_FILE, 0);
    return err;
}

/* ===================================================================== */
/* === Module als_getlux === */
int
als_getlux(float* data)
{
    int rc;
    ALOGV("als_getlux ORIGINAL : %f", *data);
    if (!als_enabled) {
        rc = als_enable();
        if (rc)
            return rc;
    }

    rc = als_result_read();
    ALOGV("Read lux light : %d", rc);
    if (rc < 0)
        return rc;
    *data = (float)rc;
    ALOGV("als_getlux RESULT : %f", *data);

    return 0;
}

/* ===================================================================== */
/* === Module als_get_filtered === */
int
als_get_filtered(unsigned long* data)
{
    ALOGV("als_get_filtered : %lu", *data);
    float datafloat;
    int err = als_getlux(&datafloat);
    if (err <= 0) {
        return err;
    }
    *data = (long)datafloat;
    return 0;
}

/* ===================================================================== */
/* === Module als_getlux_filtered === */
int
als_getlux_filtered(float* data)
{
    ALOGV("als_getlux_filtered : %f", *data);
    *data = 1;
    return als_getlux(data);
}
