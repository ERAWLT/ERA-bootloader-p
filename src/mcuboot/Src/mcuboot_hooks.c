/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "bootutil/image.h"
#include "bootutil/bootutil.h"
#include "bootutil/fault_injection_hardening.h"
#include "flash_map_backend/flash_map_backend.h"
#include "bootutil/boot_public_hooks.h" /* should be after above headers... */
#include <mcuboot_config/mcuboot_logging.h>

#include "img_manager.h"
#include "sysflash/sysflash.h"
#include "bootloader.h"

#ifdef MCUBOOT_IMAGE_ACCESS_HOOKS

int boot_read_image_header_hook(int img_index, int slot,
                                struct image_header *img_hed)
{
    return BOOT_HOOK_REGULAR;
}

fih_ret boot_image_check_hook(int img_index, int slot)
{
    FIH_RET(FIH_BOOT_HOOK_REGULAR);
}

int boot_perform_update_hook(int img_index, struct image_header *img_head,
                             const struct flash_area *area)
{
    MCUBOOT_LOG_INF("boot... perform update hook");
    /* Downgrade prevention check passed successfully, next step before updating 
     * is to check the update trigger against unauthorized action (additional protection).*/
    if (bootloader_updateInitiated()) {
        bootloader_tryUpdateProgress(BOOTLOADER_UPDATE_OPERATION_VERIFY, 1);
        return BOOT_HOOK_REGULAR;
    }
    return 0;
}

int boot_read_swap_state_primary_slot_hook(int image_index,
                                           struct boot_swap_state *state)
{
    return BOOT_HOOK_REGULAR;
}

int boot_copy_region_post_hook(int img_index, const struct flash_area *area,
                               size_t size)
{
#ifdef MCUBOOT_OVERWRITE_ONLY
    if (img_index == IMG_ID_MAIN_APP) {
        imgManager_setCopyDone(area);
        bootloader_tryUpdateProgress(BOOTLOADER_UPDATE_OPERATION_COPY, -1);
    } else {
        return -1; /* not zero - cancel erasing for external flash memory */
    }
#endif /* MCUBOOT_OVERWRITE_ONLY */
    return 0;
}

int boot_serial_uploaded_hook(int img_index, const struct flash_area *area,
                              size_t size)
{
    return 0;
}

int boot_img_install_stat_hook(int image_index, int slot, int *img_install_stat)
{
    return BOOT_HOOK_REGULAR;
}

#endif /* MCUBOOT_IMAGE_ACCESS_HOOKS */
