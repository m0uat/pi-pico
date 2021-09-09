/*
 * lora::main for Raspberry Pi Pico
 *
 * @version     1.0.0
 * @author      smittytone
 * @copyright   2021
 * @licence     MIT
 *
 */
#include "main.h"


int main() {
    #ifdef DEBUG
    stdio_init_all();
    #endif

    // Setup SPI
    SPI::setup();

    // Setup the LED
    setup_led();

    // Initialise the radio
    RFM9x radio = RFM9x(PIN_RESET, 433.0);

    // Check if we are good to proceed
    if (radio.state) {
        // Radio is good to use, apparently
        radio.enable_crc(true);
        radio.node = 0xFA;
        radio.destination = 0xFE;

        uint32_t counter = 1;

        while (true) {
            std::string msg = "smittytone messes with micros msg # ";
            msg += std::to_string(counter);

            #ifdef DEBUG
            printf(msg.c_str());
            #endif

            msg = base64_encode(msg);

            #ifdef DEBUG
            printf(msg.c_str());
            #endif

            radio.send((uint8_t*)msg.c_str(), msg.length() - 1);
            counter++;
            sleep_ms(SEND_INTERVAL_MS);
        }
    } else {
        // ERROR -- blink the LED five times
        blink_led(5);
        gpio_put(PIN_LED, false);
    }

    return 0;
}


void setup_led() {
    gpio_init(PIN_LED);
    gpio_set_dir(PIN_LED, GPIO_OUT);
    led_off();
}

void led_on() {
    gpio_put(PIN_LED, true);
}

void led_off() {
    gpio_put(PIN_LED, false);
}

/**
    Blink the Pico LED a specified number of times, leaving it
    on at the end.
    - Parameters:
        - blinks: The number of flashes.
 */
void blink_led(uint32_t blinks) {
    for (uint32_t i = 0 ; i < blinks ; ++i) {
        led_off();
        sleep_ms(250);
        led_on();
        sleep_ms(250);
    }
}