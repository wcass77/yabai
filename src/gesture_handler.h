#ifndef GESTURE_HANDLER_H
#define GESTURE_HANDLER_H

struct gesture_handler
{
    void *framework_handle;
    void **device_list;
    int device_count;
    int active_fingers;
    double last_timestamp;
    float start_x;
    float start_y;
    float last_x;
    float last_y;
    bool enabled;
};

bool gesture_handler_begin(struct gesture_handler *handler);
void gesture_handler_refresh_devices(struct gesture_handler *handler);

#endif
