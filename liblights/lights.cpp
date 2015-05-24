/*
 * Copyright (C) 2012-2013, The CyanogenMod Project
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

/**
* @file ligths.cpp
*
* Handle backlight in AOSP style and forward led notification
* to the Sony light HAL.
*
*/

//#define LOG_NDEBUG 0
#define LOG_TAG "lights.cm"
#include <cutils/log.h>

#include <fcntl.h>
#include <math.h>

#include <utils/threads.h>
#include <utils/String8.h>
#include <hardware/hardware.h>
#include <hardware/lights.h>
#include <cutils/properties.h>

#include "sony_lights.h"

//#include <stdint.h>
//#include <string.h>
//#include <unistd.h>
//#include <errno.h>
//#include <pthread.h>
//#include <sys/ioctl.h>
//#include <sys/types.h>

static pthread_once_t g_init = PTHREAD_ONCE_INIT;
static pthread_mutex_t g_lock = PTHREAD_MUTEX_INITIALIZER;

static android::Mutex gLightWrapperLock;
static hw_module_t *gVendorModule = 0;

static struct light_state_t g_notification;
static struct light_state_t g_battery;

static int open_lights(const hw_module_t* module, const char* name,
                       hw_device_t** device);
static int lights_device_close(hw_device_t* device);
static int lights_set_light(struct light_device_t* dev,
                            struct light_state_t const* state);

struct hw_module_methods_t lights_module_methods = {
    open: open_lights,
};

hw_module_t HAL_MODULE_INFO_SYM = {
    tag: HARDWARE_MODULE_TAG,
    version_major: 1,
    version_minor: 0,
    id: LIGHTS_HARDWARE_MODULE_ID,
    name: "Xperia Lights Wrapper",
    author: "The CyanogenMod Project",
    methods: &lights_module_methods,
    dso: NULL,
    reserved: {0},
};

typedef struct wrapper_light_device {
    light_device_t base;
    light_device_t *vendor;
} wrapper_light_device_t;

#define VENDOR_CALL(dev, func, ...) ({ \
    wrapper_light_device_t *__wrapper_dev = (wrapper_light_device_t*) dev; \
    __wrapper_dev->vendor->func(__wrapper_dev->vendor, ##__VA_ARGS__); \
})

static int check_vendor_module()
{
    int rv = 0;
    ALOGV("[%s]", __FUNCTION__);

    if (gVendorModule)
        return 0;

    rv = hw_get_module_by_class("lights", "vendor", (const hw_module_t **)&gVendorModule);
    if (rv)
        ALOGE("failed to open vendor light module");

    return rv;
}

static int lights_set_light(struct light_device_t* dev,
                     struct light_state_t const* state)
{
    ALOGV("%s->%08X->%08X", __FUNCTION__, (uintptr_t)dev, (uintptr_t)(((wrapper_light_device_t*)dev)->vendor));

    if(!dev)
        return -EINVAL;

    return VENDOR_CALL(dev, set_light, state);
}

static int lights_device_close(hw_device_t* device)
{
    int ret = 0;
    wrapper_light_device_t *wrapper_dev = NULL;

    ALOGV("[%s]", __FUNCTION__);

    android::Mutex::Autolock lock(gLightWrapperLock);

    if (!device) {
        ret = -EINVAL;
        goto done;
    }

    wrapper_dev = (wrapper_light_device_t*) device;

    wrapper_dev->vendor->common.close((hw_device_t*)wrapper_dev->vendor);
    free(wrapper_dev);

done:
    return ret;
}

static int read_int (const char *path) {
	int fd;
	char buffer[20];
	
	fd = open(path, O_RDONLY);
	if (fd < 0) {
	  return -1;
	}
	
	int cnt = read(fd, buffer, 20);
	close(fd);
	
	if(cnt > 0) {
	  return atoi(buffer);
	}
	return -1;
}

static int write_int (const char *path, int value) {
    int fd;
    static int already_warned = 0;

    fd = open(path, O_RDWR);
    if (fd < 0) {
        if (already_warned == 0) {
            ALOGE("write_int failed to open %s\n", path);
                already_warned = 1;
        }
        return -errno;
    }

    char buffer[20];
    int bytes = snprintf(buffer, sizeof(buffer), "%d\n", value);
    int written = write (fd, buffer, bytes);
    close(fd);

    return written == -1 ? -errno : 0;
}

static int write_string (const char *path, const char *value) {
    int fd;
    static int already_warned = 0;

    fd = open(path, O_RDWR);
    if (fd < 0) {
        if (already_warned == 0) {
            ALOGE("write_string failed to open %s\n", path);
                already_warned = 1;
        }
        return -errno;
    }

    char buffer[20];
    int bytes = snprintf(buffer, sizeof(buffer), "%s\n", value);
    int written = write (fd, buffer, bytes);
    close(fd);

    return written == -1 ? -errno : 0;
}

/* Color tools */
static int is_lit (struct light_state_t const* state) {
	return state->color & 0x00ffffff;
}

static int rgb_to_brightness (struct light_state_t const* state) {
    int color = state->color & 0x00ffffff;
    return ((77*((color>>16)&0x00ff))
            + (150*((color>>8)&0x00ff)) + (29*(color&0x00ff))) >> 8;
}

#ifdef ENABLE_GAMMA_CORRECTION
static int brightness_apply_gamma (int brightness) {
    double floatbrt = (double) brightness;
    floatbrt /= 255.0;
    ALOGV("%s: brightness = %d, floatbrt = %f", __FUNCTION__, brightness, floatbrt);
    floatbrt = pow(floatbrt, 2.2);
    ALOGV("%s: gamma corrected floatbrt = %f", __FUNCTION__, floatbrt);
    floatbrt *= 255.0;
    brightness = (int) floatbrt;
    ALOGV("%s: gamma corrected brightness = %d", __FUNCTION__, brightness);
    return brightness;
}
#endif

static int get_max_brightness() {
    char value[6];
    int fd, len, max_brightness;

    if ((fd = open(MAX_BRIGHTNESS_FILE, O_RDONLY)) < 0) {
        ALOGE("[%s]: Could not open max brightness file %s: %s", __FUNCTION__,
                     MAX_BRIGHTNESS_FILE, strerror(errno));
        ALOGE("[%s]: Assume max brightness 255", __FUNCTION__);
        return 255;
    }

    if ((len = read(fd, value, sizeof(value))) <= 1) {
        ALOGE("[%s]: Could not read max brightness file %s: %s", __FUNCTION__,
                     MAX_BRIGHTNESS_FILE, strerror(errno));
        ALOGE("[%s]: Assume max brightness 255", __FUNCTION__);
        close(fd);
        return 255;
    }

    max_brightness = strtol(value, NULL, 10);
    close(fd);

    return (unsigned int) max_brightness;
}

static int lights_set_light_backlight (struct light_device_t *dev, struct light_state_t const *state) {
    int err = 0;
    int brightness = rgb_to_brightness(state);
    int max_brightness = get_max_brightness();

    if (brightness > 0) {
#ifdef ENABLE_GAMMA_CORRECTION
        brightness = brightness_apply_gamma(brightness);
#endif
        brightness = max_brightness * brightness / 255;
        if (brightness < LCD_BRIGHTNESS_MIN)
            brightness = LCD_BRIGHTNESS_MIN;
    }

    ALOGV("[%s] brightness %d max_brightness %d", __FUNCTION__, brightness, max_brightness);

    pthread_mutex_lock(&g_lock);
    err |= write_int (LCD_BACKLIGHT_FILE, brightness);
    err |= write_int (LCD_BACKLIGHT2_FILE, brightness);
#ifdef DEVICE_HAYABUSA
    err |= write_int (LOGO_BACKLIGHT_FILE, brightness);
    err |= write_int (LOGO_BACKLIGHT2_FILE, brightness);
#endif
    pthread_mutex_unlock(&g_lock);

    return err;
}

static int set_light_buttons (struct light_device_t *dev, struct light_state_t const* state) {
	return 0;
}

static void set_shared_light_locked (struct light_device_t *dev, struct light_state_t *state) {
	int r, g, b;

	r = (state->color >> 16) & 0xFF;
	g = (state->color >> 8) & 0xFF;
	b = (state->color) & 0xFF;

	write_int (RED1_LED_FILE, r);
	write_int (RED2_LED_FILE, r);
	write_int (RED3_LED_FILE, r);
	write_int (RED1C_LED_FILE, r);
	write_int (RED2C_LED_FILE, r);
	write_int (RED3C_LED_FILE, r);
	write_int (GREEN1_LED_FILE, g);
	write_int (GREEN2_LED_FILE, g);
	write_int (GREEN3_LED_FILE, g);
	write_int (GREEN1C_LED_FILE, g);
	write_int (GREEN2C_LED_FILE, g);
	write_int (GREEN3C_LED_FILE, g);
	write_int (BLUE1_LED_FILE, b);
	write_int (BLUE2_LED_FILE, b);
	write_int (BLUE3_LED_FILE, b);
	write_int (BLUE1C_LED_FILE, b);
	write_int (BLUE2C_LED_FILE, b);
	write_int (BLUE3C_LED_FILE, b);
}

static void handle_shared_battery_locked (struct light_device_t *dev) {
	if (is_lit (&g_notification)) {
		set_shared_light_locked (dev, &g_notification);
	} else {
		set_shared_light_locked (dev, &g_battery);
	}
}

static int set_light_battery (struct light_device_t *dev, struct light_state_t const* state) {
	pthread_mutex_lock (&g_lock);
	g_battery = *state;
	handle_shared_battery_locked(dev);
	pthread_mutex_unlock (&g_lock);
	return 0;
}

static int set_light_notifications (struct light_device_t *dev, struct light_state_t const* state) {
	pthread_mutex_lock (&g_lock);
	g_notification = *state;
	handle_shared_battery_locked(dev);
	pthread_mutex_unlock (&g_lock);
	return 0;
}

/* Initializations */
void init_globals () {
	pthread_mutex_init (&g_lock, NULL);
}

/* Glueing boilerplate */
static int close_lights (struct light_device_t *dev) {
	if (dev)
		free(dev);

	return 0;
}

static int open_lights(const hw_module_t* module, const char* name,
						struct hw_device_t** device) {
{
    int rv = 0;
    int (*set_light)(struct light_device_t* dev,
                     struct light_state_t const* state);

    wrapper_light_device_t* light_device = NULL;

    android::Mutex::Autolock lock(gLightWrapperLock);

    ALOGV("lights device open");

    if (name != NULL) {
        if (check_vendor_module())
            return -EINVAL;

        light_device = (wrapper_light_device_t*)malloc(sizeof(*light_device));
        if (!light_device) {
            ALOGE("light_device allocation fail");
            rv = -ENOMEM;
            goto fail;
        }
        memset(light_device, 0, sizeof(*light_device));

        if (rv = gVendorModule->methods->open((const hw_module_t*)gVendorModule, name, (hw_device_t**)&(light_device->vendor))) {
            ALOGE("vendor light open fail");
            goto fail;
        }
        ALOGV("[%s]: got vendor light device 0x%08X", __FUNCTION__, (uintptr_t)(light_device->vendor));

        if (0 == strcmp(LIGHT_ID_BACKLIGHT, name))
            set_light = lights_set_light_backlight;
        else if (0 == strcmp(LIGHT_ID_BUTTONS, name))
            set_light = lights_set_light;
        else if (0 == strcmp(LIGHT_ID_BATTERY, name))
            set_light = lights_set_light;
        else if (0 == strcmp(LIGHT_ID_NOTIFICATIONS, name))
            set_light = lights_set_light;
/* LIGHT_ID_ATTENTION is not supported by Sony HAL
        else if (0 == strcmp(LIGHT_ID_ATTENTION, name))
            set_light = lights_set_light;
*/
        else
            return -EINVAL;

		pthread_once (&g_init, init_globals);
		struct light_device_t *dev = malloc(sizeof (struct light_device_t));
		memset(dev, 0, sizeof(*dev));
		
		dev->common.tag 	= HARDWARE_DEVICE_TAG;
		dev->common.version = 0;
		dev->common.module 	= (struct hw_module_t*)module;
		dev->common.close 	= (int (*)(struct hw_device_t*))close_lights;
		dev->set_light 		= set_light;
	
        light_device->base.common.tag     = HARDWARE_DEVICE_TAG;
        light_device->base.common.version = 0;
        light_device->base.common.module  = (hw_module_t *)(module);
        light_device->base.common.close   = lights_device_close;
        light_device->base.set_light      = set_light;
		
		*device = (struct hw_device_t*)dev;
		
        *device = (struct hw_device_t*)light_device;
    }

    return rv;

fail:
    if (light_device) {
        free(light_device);
        light_device = NULL;
    }
    *device = NULL;

    return rv;
}

static struct hw_module_methods_t lights_module_methods = {
	.open = open_lights,
};


struct hw_module_t HAL_MODULE_INFO_SYM = {
	.tag = HARDWARE_MODULE_TAG,
	.version_major = 1,
	.version_minor = 0,
	.id = LIGHTS_HARDWARE_MODULE_ID,
	.name = "Sony lights module",
	.author = "Diogo Ferreira <defer@cyanogenmod.com>, Andreas Makris <Andreas.Makris@gmail.com>",
	.methods = &lights_module_methods,
};
