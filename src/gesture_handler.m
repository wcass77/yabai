#include <dlfcn.h>
#include <math.h>

extern struct event_loop g_event_loop;

typedef void *MTDeviceRef;
typedef CFArrayRef (*mt_device_create_list_f)(void);
typedef int (*mt_device_start_f)(MTDeviceRef, int);
typedef int (*mt_device_stop_f)(MTDeviceRef);
typedef int (*mt_register_contact_frame_callback_f)(MTDeviceRef, int (*)(MTDeviceRef, void *, int, double, int));

struct mt_touch
{
    int frame;
    double timestamp;
    int identifier;
    int state;
    int finger_id;
    float normalized_x;
    float normalized_y;
    float size;
    int zero1;
    float angle;
    float major_axis;
    float minor_axis;
    float mm_x;
    float mm_y;
};

static mt_device_create_list_f mt_device_create_list;
static mt_device_start_f mt_device_start;
static mt_device_stop_f mt_device_stop;
static mt_register_contact_frame_callback_f mt_register_contact_frame_callback;

static struct gesture_handler *g_gesture_state;

#define GESTURE_THRESHOLD 0.12f
#define GESTURE_AXIS_LOCK 1.2f
#define GESTURE_COOLDOWN 0.18

static void gesture_handler_reset(struct gesture_handler *handler)
{
    handler->active_fingers = 0;
    handler->start_x = 0.0f;
    handler->start_y = 0.0f;
    handler->last_x = 0.0f;
    handler->last_y = 0.0f;
    handler->last_timestamp = 0.0;
}

static int gesture_contact_frame_callback(MTDeviceRef device, void *data, int finger_count, double timestamp, int frame)
{
    (void) device;
    (void) frame;

    struct gesture_handler *handler = g_gesture_state;
    if (!handler || !handler->enabled) return 0;
    if (finger_count <= 0) {
        gesture_handler_reset(handler);
        return 0;
    }

    struct mt_touch *touches = data;
    float centroid_x = 0.0f;
    float centroid_y = 0.0f;

    for (int i = 0; i < finger_count; ++i) {
        centroid_x += touches[i].normalized_x;
        centroid_y += touches[i].normalized_y;
    }

    centroid_x /= finger_count;
    centroid_y /= finger_count;

    if (handler->active_fingers != finger_count || handler->last_timestamp == 0.0) {
        handler->active_fingers = finger_count;
        handler->start_x = centroid_x;
        handler->start_y = centroid_y;
        handler->last_x = centroid_x;
        handler->last_y = centroid_y;
        handler->last_timestamp = timestamp;
        return 0;
    }

    float dx = centroid_x - handler->start_x;
    float dy = centroid_y - handler->start_y;

    if (fabsf(dx) > GESTURE_THRESHOLD &&
        fabsf(dx) > fabsf(dy) * GESTURE_AXIS_LOCK &&
        timestamp - handler->last_timestamp >= GESTURE_COOLDOWN) {
        int direction = dx < 0.0f ? DIR_EAST : DIR_WEST;

        if (finger_count == 3) {
            event_loop_post(&g_event_loop, GESTURE_SCROLL_VIEW, NULL, direction);
        } else if (finger_count == 4) {
            event_loop_post(&g_event_loop, GESTURE_SWITCH_SPACE, NULL, direction);
        }

        handler->start_x = centroid_x;
        handler->start_y = centroid_y;
        handler->last_timestamp = timestamp;
    }

    handler->last_x = centroid_x;
    handler->last_y = centroid_y;
    return 0;
}

static void gesture_handler_unregister_devices(struct gesture_handler *handler)
{
    if (!handler->device_list) return;

    for (int i = 0; i < handler->device_count; ++i) {
        if (mt_device_stop) mt_device_stop(handler->device_list[i]);
        CFRelease(handler->device_list[i]);
    }

    free(handler->device_list);
    handler->device_list = NULL;
    handler->device_count = 0;
}

void gesture_handler_refresh_devices(struct gesture_handler *handler)
{
    if (!handler->framework_handle || !mt_device_create_list || !mt_register_contact_frame_callback || !mt_device_start) return;

    gesture_handler_unregister_devices(handler);

    CFArrayRef devices = mt_device_create_list();
    if (!devices) return;

    handler->device_count = (int) CFArrayGetCount(devices);
    handler->device_list = calloc(handler->device_count, sizeof(void *));

    for (int i = 0; i < handler->device_count; ++i) {
        handler->device_list[i] = (void *) CFRetain(CFArrayGetValueAtIndex(devices, i));
        mt_register_contact_frame_callback(handler->device_list[i], gesture_contact_frame_callback);
        mt_device_start(handler->device_list[i], 0);
    }

    CFRelease(devices);
    gesture_handler_reset(handler);
}

bool gesture_handler_begin(struct gesture_handler *handler)
{
    memset(handler, 0, sizeof(*handler));
    handler->framework_handle = dlopen("/System/Library/PrivateFrameworks/MultitouchSupport.framework/Versions/Current/MultitouchSupport", RTLD_LAZY);
    if (!handler->framework_handle) {
        handler->framework_handle = dlopen("/System/Library/PrivateFrameworks/MultitouchSupport.framework/MultitouchSupport", RTLD_LAZY);
    }
    if (!handler->framework_handle) return false;

    mt_device_create_list = dlsym(handler->framework_handle, "MTDeviceCreateList");
    mt_device_start = dlsym(handler->framework_handle, "MTDeviceStart");
    mt_device_stop = dlsym(handler->framework_handle, "MTDeviceStop");
    mt_register_contact_frame_callback = dlsym(handler->framework_handle, "MTRegisterContactFrameCallback");

    if (!mt_device_create_list || !mt_device_start || !mt_register_contact_frame_callback) {
        dlclose(handler->framework_handle);
        memset(handler, 0, sizeof(*handler));
        return false;
    }

    handler->enabled = true;
    g_gesture_state = handler;
    gesture_handler_refresh_devices(handler);
    return true;
}
