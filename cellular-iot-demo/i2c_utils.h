/*
 * cellular::i2c_utils for Raspberry Pi Pico
 *
 * @version     1.0.1
 * @author      smittytone
 * @copyright   2021
 * @licence     MIT
 *
 */
#ifndef _I2C_HEADER_
#define _I2C_HEADER_


/*
 * CONSTANTS
 */
#define I2C_PORT                i2c1
#define I2C_FREQUENCY           400000
#define SDA_GPIO                2
#define SCL_GPIO                3


/*
 * PROTOTYPES
 */
namespace I2C {
    void        setup();
    void        write_byte(uint8_t address, uint8_t byte);
    void        write_block(uint8_t address, uint8_t *data, uint8_t count);
    void        read_block(uint8_t address, uint8_t *data, uint8_t count);
}


#endif  // _I2C_HEADER_