# SPDX-License-Identifier: MIT

zephyr_library()

zephyr_library_sources_ifdef(CONFIG_INPUT_PINNACLE ../drivers/input/input_pinnacle.c)
target_sources_ifdef(CONFIG_ZMK_INPUT_PINNACLE_IDLE_SLEEPER app PRIVATE
                     ../drivers/input/zmk_pinnacle_idle_sleeper.c)
