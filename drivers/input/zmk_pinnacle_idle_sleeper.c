

#include <zephyr/init.h>
#include <zephyr/sys/util.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zmk/activity.h>
#include <zmk/event_manager.h>
#include <zmk/events/activity_state_changed.h>
#include "input_pinnacle.h"

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(pinnacle_sleeper, CONFIG_INPUT_LOG_LEVEL);

#define GET_PINNACLE(node_id) DEVICE_DT_GET(node_id),

static const struct device *pinnacle_devs[] = {
    DT_FOREACH_STATUS_OKAY(cirque_pinnacle, GET_PINNACLE)
};

static bool should_sleep(enum zmk_activity_state state) {
    /* In split keyboards, the peripheral half can enter IDLE even while the overall keyboard is
     * actively being used (e.g., typing on the central/left half). If we enable Pinnacle sleep in
     * IDLE on the peripheral, the trackpad can go to sleep and a simple tap may not wake it,
     * leading to frequent "tap not recognized" reports.
     *
     * Therefore:
     * - Central / non-split: sleep for any non-ACTIVE state (IDLE + SLEEP).
     * - Peripheral: only sleep for SLEEP.
     */
#if IS_ENABLED(CONFIG_ZMK_SPLIT) && !IS_ENABLED(CONFIG_ZMK_SPLIT_ROLE_CENTRAL)
    return state == ZMK_ACTIVITY_SLEEP;
#else
    return state != ZMK_ACTIVITY_ACTIVE;
#endif
}

static void apply_sleep_state(enum zmk_activity_state state) {
    const bool sleep = should_sleep(state);

    for (size_t i = 0; i < ARRAY_SIZE(pinnacle_devs); i++) {
        const struct device *dev = pinnacle_devs[i];

        if (!device_is_ready(dev)) {
            LOG_WRN("Pinnacle device not ready");
            continue;
        }

        const int ret = pinnacle_set_sleep(dev, sleep);
        if (ret < 0) {
            LOG_WRN("Failed to set sleep=%d: %d", sleep, ret);
        }
    }
}

static int on_activity_state(const zmk_event_t *eh) {
    const struct zmk_activity_state_changed *state_ev = as_zmk_activity_state_changed(eh);
    if (!state_ev) {
        return 0;
    }

    apply_sleep_state(state_ev->state);

    return 0;
}

static int zmk_pinnacle_idle_sleeper_init(void) {
    apply_sleep_state(zmk_activity_get_state());
    return 0;
}

SYS_INIT(zmk_pinnacle_idle_sleeper_init, APPLICATION, 99);

ZMK_LISTENER(zmk_pinnacle_idle_sleeper, on_activity_state);
ZMK_SUBSCRIPTION(zmk_pinnacle_idle_sleeper, zmk_activity_state_changed);
