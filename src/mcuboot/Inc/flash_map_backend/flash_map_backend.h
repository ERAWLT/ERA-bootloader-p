/**
 * @file flash_map_backend.h
 * @brief Flash map backend header.
 */

#ifndef _FLASH_MAP_BACKEND_H_
#define _FLASH_MAP_BACKEND_H_

#include <stdbool.h>
#include <inttypes.h>

/** Structure that describing a flash area. */
struct flash_area {
    /* MCUboot-API fields */
    uint8_t  fa_id;           /*!< The slot/scratch identification */
    uint8_t  fa_device_id;    /*!< The device id (usually there's only one) */
    uint16_t pad16;           /*!< Padding */
    uint32_t fa_off;          /*!< The flash offset from the beginning */
    uint32_t fa_size;         /*!< The size of this sector */
};

/** Structure describing a sector within a flash area. */
struct flash_sector {
  /** Offset of this sector, from the start of its flash area (not device). */
  uint32_t fs_off;

  /** Size of this sector, in bytes. */
  uint32_t fs_size;
};

/**
 * @brief   Obtain the ID of the device in which a given flash area resides on.
 * 
 * @param[in] 	fa Flash area.
 * 
 * @return uint8_t. The device ID of the requested flash area.
 */
static inline uint8_t flash_area_get_device_id(const struct flash_area *fa)
{
    return (uint8_t)fa->fa_device_id;
}

/**
 * @brief   Obtain the offset, from the beginning of a device, where a given flash
 *  area starts at.
 * 
 * @param[in] 	fa Flash area.
 * 
 * @return uint32_t. The offset value of the requested flash area.
 */
static inline uint32_t flash_area_get_off(const struct flash_area *fa)
{
    return (uint32_t)fa->fa_off;
}

/**
 * @brief   Obtain the size, from the offset, of a given flash area.
 *
 * @param[in] 	fa Flash area.
 *
 * @return uint32_t. The size value of the requested flash area.
 */
static inline uint32_t flash_area_get_size(const struct flash_area *fa)
{
    return (uint32_t)fa->fa_size;
}

/**
 * @brief   Obtain the ID of a given flash area.
 *
 * @param[in] 	fa Flash area.
 *
 * @return uint8_t. The ID of the requested flash area.
 */
static inline uint8_t flash_area_get_id(const struct flash_area *fa)
{
    return fa->fa_id;
}


/**
 * @brief   Obtain the offset, from the beginning of its flash area, where a given
 *  flash sector starts at.
 *
 * @param[out]   fs Flash sector.
 *
 * @return uint32_t. The offset value of the requested flash sector.
 */
static inline uint32_t flash_sector_get_off(const struct flash_sector *fs)
{
    return fs->fs_off;
}

/**
 * @brief   Obtain the size, from the offset, of a given flash sector.
 *
 * @param[out]   fs Flash sector.
 *
 * @return uint32_t. The size in bytes of the requested flash sector.
 */
static inline uint32_t flash_sector_get_size(const struct flash_sector *fs)
{
    return fs->fs_size;
}

/**
 * @brief   Retrieve flash area from the flash map for a given partition.
 *
 * @param           id ID of the flash partition.
 * @param[out]      area_outp Pointer which will contain the reference to flash_area.
 *                            If ID is unknown, it will be NULL on output.
 *
 * @return int. Zero on success, or negative value in case of error.
 */
int flash_area_open(uint8_t id, const struct flash_area **area_outp);

/**
 * @brief Close a given flash area.
 *
 * @param[in]   fa Flash area to be closed.
 *
 * @return None.
 */
void flash_area_close(const struct flash_area *fa);

/**
 * @brief   Read data from flash area.
 *  Area readout boundaries are asserted before read request. API has the
 *  same limitation regarding read-block alignment and size as the
 *  underlying flash driver.
 *
 * @param[in] 	fa Flash area to be read.
 * @param       off Offset relative from beginning of flash area to be read.
 * @param       len Number of bytes to read.
 * @param       dst Buffer to store read data.
 *
 * @return int. Zero on success, or negative value in case of error.
 */
int flash_area_read(const struct flash_area *fa, uint32_t off,
                    void *dst, uint32_t len);
                    
/**
 * @brief   Write data to flash area.
 *  Area write boundaries are asserted before write request. API has the
 *  same limitation regarding write-block alignment and size as the
 *  underlying flash driver.
 *
 * @param[in] 	fa Flash area to be written.
 * @param       off Offset relative from beginning of flash area to be written.
 * @param       src Buffer with data to be written.
 * @param       len Number of bytes to write.
 *
 * @return int. Zero on success, or negative value in case of error.
 */
int flash_area_write(const struct flash_area *fa, uint32_t off,
                     const void *src, uint32_t len);

/**
 * @brief   Erase a given flash area range.
 *  Area boundaries are asserted before erase request. API has the same
 *  limitation regarding erase-block alignment and size as the underlying
 *  flash driver.
 *
 * @param[in] 	fa Flash area to be erased.
 * @param       off Offset relative from beginning of flash area to be erased.
 * @param       len Number of bytes to be erase.
 *
 * @return int. Zero on success, or negative value in case of error.
 */
int flash_area_erase(const struct flash_area *fa,
                     uint32_t off, uint32_t len);

/**
 * @brief   Get write block size of the flash area.
 *  Write block size might be treated as read block size, although most
 *  drivers support unaligned readout.
 *
 * @param[in] 	fa Flash area.
 *
 * @return uint32_t. Alignment restriction for flash writes in the given flash area.
 */
uint32_t flash_area_align(const struct flash_area *area);

/**
 * @brief   Get the value expected to be read when accessing any erased flash byte.
 *  This API is compatible with the MCUboot's porting layer.
 *
 * @param[in] 	fa Flash area.
 *
 * @return uint8_t. Byte value of erased memory.
 */
uint8_t flash_area_erased_val(const struct flash_area *area);

/**
 * @brief   Retrieve info about sectors within the area.
 *
 * @param 	        fa_id   ID of the flash area whose info will be retrieved.
 * @param[inout]    count   On input, represents the capacity of the sectors buffer.
 *                          On output, it shall contain the number of retrieved sectors.
 * @param[out]      sectors Buffer for sectors data.
 *
 * @return int. Zero on success, or negative value in case of error.
 */
int flash_area_get_sectors(int fa_id, uint32_t *count,
                           struct flash_sector *sectors);

/**
 * @brief   Retrieve the flash sector a given offset belongs to.
 *
 * @param[in] 	    fa Flash area structure.
 * @param           offset Offset address.
 * @param[out]      sector Sector in flash.
 *
 * @return int. Returns 0 on success, or an error code on failure.
 */
int flash_area_get_sector(const struct flash_area *fa, uint32_t off,
                          struct flash_sector *sector);

/**
 * @brief   Return the flash area ID for a given slot and a given image index
 *  (in case of a multi-image setup).
 *
 * @param   image_index Index of the image.
 * @param   slot Image slot, which may be 0 (primary) or 1 (secondary).
 *
 * @return int. Flash area ID (0 or 1), or negative value in case the requested slot
 *              is invalid.
 */
int flash_area_id_from_multi_image_slot(int image_index, int slot);

/**
 * @brief Return the flash area ID for a given slot.
 *
 * @param   slot Image slot, which may be 0 (primary) or 1 (secondary).
 *
 * @return int. Flash area ID (0 or 1), or negative value in case the requested slot
 *              is invalid.
 */
int flash_area_id_from_image_slot(int slot);

/**
 * @brief   Check ability to change flash area and change flash area offset if possible.
 *
 * @param[in] 	fa Flash area structure
 * @param       shift_in_bytes Additional offset.
 *
 * @return bool. Returns true if flash area can be changed, false otherwise.
 */
bool flash_area_move_start(const struct flash_area *fa, uint32_t shift_in_bytes);

/**
 * @brief   Reset flash area offset to initial state.
 * 
 * @param[in] 	fa Flash area structure
 * 
 * @return None
 */
void flash_area_reset_start(const struct flash_area *fa);

#endif /* _FLASH_MAP_BACKEND_H_ */
