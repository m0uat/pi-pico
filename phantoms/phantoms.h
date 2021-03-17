/*
 * Phantoms
 *
 * @version     1.0.0
 * @author      smittytone
 * @copyright   2021, Tony Smith
 * @licence     MIT
 *
 */
#ifndef _PHANTOMS_HEADER_
#define _PHANTOMS_HEADER_


/*
 * PROTOTYPES
 */
void move_phantoms();
uint8_t get_facing_phantom(uint8_t range);
uint8_t locate_phantom(uint8_t x, uint8_t y);
void manage_phantoms();
void roll_new_phantom(uint8_t index);


// _PHANTOMS_HEADER_
#endif