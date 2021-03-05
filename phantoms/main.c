#include "main.h"


/*
 *  Corner triangles
 */
const char angles[2][10] = {
    "\x0F\x0E\x0E\x0E\x0C\x0C\x0C\x08\x08\x08",
    "\xF0\x70\x70\x70\x30\x30\x30\x10\x10\x10"
};


/*
 *  Initialisation Functions
 */
void setup() {
    // Set up the game hardware

    // Set up I2C
    i2c_init(I2C_PORT, I2C_FREQUENCY);
    gpio_set_function(PIN_SDA, GPIO_FUNC_I2C);
    gpio_set_function(PIN_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(PIN_SDA);
    gpio_pull_up(PIN_SCL);

    // Set up the OLED
    ssd1306_init(128, 64);

    // Set up the speaker
    gpio_init(PIN_SPEAKER);
    gpio_set_dir(PIN_SPEAKER, GPIO_OUT);
    gpio_put(PIN_SPEAKER, false);

    // Set up the Fire button
    gpio_init(PIN_FIRE_BUTTON);
    gpio_set_dir(PIN_FIRE_BUTTON, GPIO_IN);
    gpio_pull_down(PIN_FIRE_BUTTON);

    // Set up the Teleport button
    gpio_init(PIN_TELE_BUTTON);
    gpio_set_dir(PIN_TELE_BUTTON, GPIO_IN);
    gpio_pull_down(PIN_TELE_BUTTON);

    // Set up the X- and Y-axis joystick input
    adc_init();
    adc_gpio_init(PIN_X);
    adc_gpio_init(PIN_Y);

    // Use one of the Pico's other analog inputs
    // to seed the random number generator
    adc_gpio_init(28);
    adc_select_input(2);
    srand(adc_read());

    gpio_init(PIN_LED);
    gpio_set_dir(PIN_LED, GPIO_OUT);
    gpio_put(PIN_LED, false);

    // Make the graphic frame rects
    // NOTE These are pixel values
    uint8_t coords[] = {0,0,127,63,
                        11,4,106,54,
                        24,9,80,43,
                        36,15,56,33,
                        47,20,34,24,
                        55,22,18,18,
                        61,25,6,12};

    for (uint8_t i = 0 ; i < sizeof(coords) ; i += 4) {
        Rect a_rect;
        a_rect.origin_x = coords[i];
        a_rect.origin_y = coords[i + 1];
        a_rect.width = coords[i + 2];
        a_rect.height = coords[i + 3];
        rects[i >> 2] = a_rect;
    }
}


void create_world() {
    // Generate and populate a new maze which happens
    // at the start of a new game and at the start of
    // each level

    if (game.in_play) {
        game.level++;
        game.phantoms = 0;
    } else {
        // Reset the game
        init_game();
    }

    uint8_t player_quad = irandom(0, 3);

    // Generate phantoms
    // Place one in each non-player quadrant
    game.phantoms = irandom(1, 3);;
    uint8_t inc = 0;

    for (uint8_t i = 0 ; i < game.phantoms ; ++i) {
        Phantom a_phantom;
        phantoms[i] = a_phantom;
        if (i == player_quad) inc++;
        a_phantom.hp = i + inc;
    }

    // Add the phantoms to the map
    for (uint8_t i = 0 ; i < game.phantoms ; ++i) {
        Phantom a_phantom;
        while(1) {
            uint8_t x = irandom(0, 10);
            uint8_t y = irandom(0, 10);
            if (a_phantom.hp = 1 || a_phantom.hp == 2) x += 10;
            if (a_phantom.hp = 2 || a_phantom.hp == 3) y += 10;

            if (get_square_contents(x, y) == 0xFF) {
                a_phantom.x = x;
                a_phantom.y = y;
                a_phantom.hp = irandom(1,3) + game.level;
                break;
            }
        }
    }

    // Place the player
    while(1) {
        uint8_t x = irandom(0, 10);
        uint8_t y = irandom(0, 10);
        if (player_quad = 1 || player_quad == 2) x += 10;
        if (player_quad = 2 || player_quad == 3) y += 10;

        if (get_square_contents(x, y) == 0xFF) {
            player_x = x;
            player_y = y;
            player_direction = irandom(0,4);
            break;
        }
    }

    // Initialise the current map
    map_init();
}


void init_game() {
    // Reset the main game control structure
    game.in_play = true;
    game.show_reticule = false;
    game.is_firing = false;
    game.can_teleport = false;
    game.is_joystick_centred = true;
    game.score = 0;
    game.audio_range = 4;
    game.phantoms = 0;
    game.level = 1;
    game.zap_time = 0;
    game.debounce_count_button = 0;
}


void game_loop() {
    // Read the current joystick position.
    // If it's not in the deadzone, then determine
    // which direction it's in (up, down, left or right).
    // If it's in the deadzone, check if the player is trying
    // to fire an arrow.

    while (game.in_play) {
        bool is_dead = false;
        if (game.zap_time != 0) {
            if (time_us_32() - game.zap_time > 3000000) {
                // Laser trigger held too long
                game.show_reticule = false;
                game.is_firing = true;
                game.zap_time = 0;
            }
        } else {
            // Read joystick analog output
            adc_select_input(0);
            uint16_t x = adc_read();
            adc_select_input(1);
            uint16_t y = adc_read();

            if (check_joystick(x, y)) {
                // Joystick is pointing in a direction, so
                // get the direction the player has chosen
                // 0 = forward
                // 2 = backward
                // 1 = rotate left
                // 3 = rotate right
                uint8_t dir = get_direction(x, y);

                if (dir == 0) {
                    // Move player forward if we can
                    uint8_t nx = player_x;
                    uint8_t ny = player_x;

                    if (player_direction == DIRECTION_NORTH) --ny;
                    if (player_direction == DIRECTION_SOUTH) ++ny;
                    if (player_direction == DIRECTION_EAST) ++nx;
                    if (player_direction == DIRECTION_WEST) --nx;

                    if (get_square_contents(nx, ny) != 0xFF) {
                        // Collision

                    } else {
                        player_x = nx;
                        player_y = ny;
                    }
                } else if (dir == 2) {
                    // Move player down
                    // Move player backward if we can
                    uint8_t nx = player_x;
                    uint8_t ny = player_x;

                    if (player_direction == DIRECTION_NORTH) ++ny;
                    if (player_direction == DIRECTION_SOUTH) --ny;
                    if (player_direction == DIRECTION_EAST) --nx;
                    if (player_direction == DIRECTION_WEST) ++nx;

                    if (get_square_contents(nx, ny) != 0xFF) {
                        // Collision

                    } else {
                        player_x = nx;
                        player_y = ny;
                    }
                } else if (dir == 3) {
                    // Turn player right
                    player_direction++;
                    if (player_direction > DIRECTION_WEST) player_direction = DIRECTION_NORTH;
                } else {
                    // Turn player left
                    --player_direction;
                    if (player_direction > DIRECTION_WEST) player_direction = DIRECTION_WEST;
                }

                // Check the new location for audio - is a phantom nearby?
                if (!is_dead) check_senses();
            } else {
                // Joystick is in deadzone
                if (gpio_get(PIN_FIRE_BUTTON)) {
                    if (!game.show_reticule) {
                        uint32_t now = time_us_32();
                        if (game.debounce_count_button == 0) {
                            // Set debounce timer
                            game.debounce_count_button = now;
                        } else if (now - game.debounce_count_button > DEBOUNCE_TIME_US) {
                            // Clear debounce timer
                            game.debounce_count_button == 0;
                            game.show_reticule == true;
                            game.zap_time = now;
                        }
                    }
                } else {
                    if (game.show_reticule) {
                        uint32_t now = time_us_32();
                        if (game.debounce_count_button == 0) {
                            // Set debounce timer
                            game.debounce_count_button = now;
                        } else if (now - game.debounce_count_button > DEBOUNCE_TIME_US) {
                            // Clear debounce timer
                            game.debounce_count_button == 0;
                            game.show_reticule = false;
                            game.is_firing = true;
                            game.zap_time = 0;
                        }
                    }
                }

                if (gpio_get(PIN_TELE_BUTTON)) {
                    // Player can only teleport if they have walked over the teleport square
                    // and they are not firing the laser
                    if (!game.show_reticule && game.can_teleport) {
                        uint32_t now = time_us_32();
                        if (game.debounce_count_button == 0) {
                            // Set debounce timer
                            game.debounce_count_button = now;
                        } else if (now - game.debounce_count_button > DEBOUNCE_TIME_US) {
                            // Clear debounce timer
                            game.debounce_count_button == 0;
                            game.can_teleport = false;

                            // Teleport
                            do_teleport();
                        }
                    }
                }
            }
        }

        if (!is_dead) {
            // Draw the world
            draw_world();

            if (game.is_firing) {
                // Fire!
                fire_laser();
            } else {
                // Pause between cycles
                sleep_ms(50);
            }
        } else {
            game.in_play = false;
            break;
        }
    }
}


bool check_joystick(uint16_t x, uint16_t y) {
    // Check to see if the joystick is currently
    // outside of the central deadzone, and that it
    // has returned to the centre before re-reading
    if (x > UPPER_LIMIT || x < LOWER_LIMIT || y > UPPER_LIMIT || y < LOWER_LIMIT) {
        if (game.is_joystick_centred) {
            // We're good to use the reading, but not
            game.is_joystick_centred = false;
            return true;
        } else {
            return false;
        }
    }

    // Joystick is centred
    game.is_joystick_centred = true;
    return false;
}


uint8_t get_direction(uint16_t x, uint16_t y) {
    // Get player direction from the analog input
    if (x < y) {
        if (x > (JOY_MAX - y)) {
            return DIRECTION_NORTH;     // up
        } else {
            return DIRECTION_WEST;      // right
        }
    } else {
        if (x > (JOY_MAX - y)) {
            return DIRECTION_EAST;      // left
        } else {
            return DIRECTION_SOUTH;     // down
        }
    }
}

void draw_world() {

    ssd1306_clear();
    draw_screen();
    ssd1306_draw();
}


void death() {
    // The player has died -- show the map and the score
    ssd1306_clear();
    ssd1306_text(0, 0, "Score: ", false, false);
    // TODO
    // Show the score
    char score_string[] = "00000";
    ssd1306_text(64, 0, score_string, false, false);
    show_map(15);

    sleep_ms(20000);
    // TODO
    // Exit on button press OR timer
}


/*
 * Graphics Functions
 */
void draw_screen() {

    uint8_t max_steps = get_view_distance(player_x, player_y, player_direction);
    uint8_t steps = 0;

    switch(player_direction) {
        case DIRECTION_NORTH:
            // Facing north, so left = W, right = E
            // Run through squares from current to map limit
            for (uint8_t i = player_y ; i >= 0 ; --i) {
                draw_floor_line(steps);

                // Are the walls to the left and right open or closed
                bool left_open = (get_view_distance(player_x, i, DIRECTION_WEST) > 0);
                bool right_open = (get_view_distance(player_x, i, DIRECTION_EAST) > 0);

                // Draw in the current walls
                draw_left_wall(steps, left_open);
                draw_right_wall(steps, right_open);

                // Got to the end?
                if (steps == max_steps) {
                    // Draw the facing wall (or infinity)
                    draw_end(steps);
                    break;
                } else {
                    // Step to the next most distant square
                    steps++;
                }
            }
            break;
        case DIRECTION_EAST:
            // Facing E, so left = N, right = S
            for (uint8_t i = player_x ; i < 20 ; ++i) {
                draw_floor_line(steps);

                bool left_open = (get_view_distance(i, player_y, DIRECTION_NORTH) > 0);
                bool right_open = (get_view_distance(i, player_y, DIRECTION_SOUTH) > 0);

                draw_left_wall(steps, left_open);
                draw_right_wall(steps, right_open);

                if (steps == max_steps) {
                    draw_end(steps);
                    break;
                } else {
                    steps++;
                }
            }
            break;
        case DIRECTION_SOUTH:
            // Facing S, so left = E, right = W
            for (uint8_t i = player_y ; i < 20 ; ++i) {
                draw_floor_line(steps);

                bool left_open = (get_view_distance(player_x, i, DIRECTION_EAST) > 0);
                bool right_open = (get_view_distance(player_x, i, DIRECTION_WEST) > 0);

                draw_left_wall(steps, left_open);
                draw_right_wall(steps, right_open);

                if (steps == max_steps) {
                    draw_end(steps);
                    break;
                } else {
                    steps++;
                }
            }
            break;
        default:
            // Facing W, so left = S, right = N
            for (uint8_t i = player_y ; i < 20 ; ++i) {
                draw_floor_line(steps);

                bool left_open = (get_view_distance(player_x, i, DIRECTION_SOUTH) > 0);
                bool right_open = (get_view_distance(player_x, i, DIRECTION_NORTH) > 0);

                draw_left_wall(steps, left_open);
                draw_right_wall(steps, right_open);

                if (steps == max_steps) {
                    draw_end(steps);
                    break;
                } else {
                    steps++;
                }
            }
            break;
    }
}

void draw_floor_line(uint8_t inset) {
    // Draw a grid line on the floor
    Rect r = rects[inset + 1];
    ssd1306_line(r.origin_x, r.origin_y + r.height, r.origin_x + r.width, r.origin_y + r.height, 1, 1);
    ssd1306_draw();
}

void draw_left_wall(uint8_t inset, bool is_open) {
    // Draw main left-side rect
    Rect i = rects[inset + 1];
    Rect o = rects[inset];
    ssd1306_rect(o.origin_x, i.origin_y, i.origin_x - o.origin_x - 1, i.height, 1, true);
    if (is_open) return;

    // Add upper and lower triangles for a closed wall
    for (uint8_t i = 0 ; i < 10 ; ++i) {
        uint8_t byte =  angles[0][i];
        for (uint8_t j = 0 ; j < 8 ; ++j) {
            if ((byte & (1 << j)) > 0) ssd1306_plot(o.origin_x + i, o.origin_x + j, 1);
        }

        byte = angles[1][i];
        for (uint8_t j = 0 ; j < 8 ; ++j) {
            if ((byte & (1 << j)) > 0) ssd1306_plot(o.origin_x + i, o.origin_x + o.height + j, 1);
        }
    }
    ssd1306_draw();
}

void draw_right_wall(uint8_t inset, bool is_open) {
    // Draw main right-side rect
    Rect i = rects[inset + 1];
    Rect o = rects[inset];
    uint8_t x = i.origin_x + i.width + 1;
    ssd1306_rect(x, i.origin_y, o.origin_x + o.width - x + 2, i.height, 1, true);
    if (is_open) return;

    // Add upper and lower triangles for a closed wall
    for (uint8_t i = 0 ; i < 10 ; ++i) {
        uint8_t byte =  angles[0][i];
        for (uint8_t j = 0 ; j < 8 ; ++j) {
            if ((byte & (1 << j)) > 0) ssd1306_plot(o.width - o.origin_x - i, o.origin_x + j, 1);
        }

        byte = angles[1][i];
        for (uint8_t j = 0 ; j < 8 ; ++j) {
            if ((byte & (1 << j)) > 0) ssd1306_plot(o.width - o.origin_x - i, o.origin_x + o.height + j, 1);
        }
    }
    ssd1306_draw();
}


void draw_rect(Rect *inner_rect, Rect *outer_rect, bool is_open) {
    // Draw a generic left or right wall rect, open or closed

    // Draw an open-wall rect
    ssd1306_rect(outer_rect->origin_x, inner_rect->origin_y, inner_rect->origin_x - outer_rect->origin_x, inner_rect->height, 0, true);

    // If the wall is open, we're done
    if (is_open) return;

    // TO DO
    // Draw the closed top and bottom triangles
}

void draw_end(uint8_t steps) {
    if (steps == 6) return;
    Rect i = rects[steps + 1];
    ssd1306_rect(i.origin_x + 2, i.origin_y, i.width - 2, i.height, 1, true);
    ssd1306_draw();
}


/*
 *  Actions
 */
void check_senses() {
    return;
}

void do_teleport() {
    return;
}

void fire_laser() {
    return;
}

/*
 *  Misc Functions
 */
int irandom(int start, int max) {
    // Generate a PRG between start and max
    return rand() % max + start;
}

void tone(unsigned int frequency, unsigned long duration, unsigned long post) {
    // Get the cycle period in microseconds
    // NOTE Input is in Hz
    float period = 1000000 / (float)frequency;

    // Get the microsecond timer now
    unsigned long start = time_us_64();

    // Loop until duration (ms) in microseconds has elapsed
    while (time_us_64() < start + duration * 1000) {
        gpio_put(PIN_SPEAKER, true);
        sleep_us(0.5 * period);
        gpio_put(PIN_SPEAKER, false);
        sleep_us(0.5 * period);
    };

    // Apply a post-tone delay
    sleep_ms(post);
}



/*
 * Runtime start
 *
 */
int main() {

    // Setup the hardware
    setup();

    // Play the game

    /*
    const char title[] = "\x02\x3E\x02\x00\x3E\x08\x3E\x00\x3E\x2A\x22\x00\x00\x3E\x0A\x0E\x00\x3E\x08\x3E\x00\x3C\x0A\x3C\x00\x3E\x02\x3E\x00\x02\x3E\x02\x00\x3E\x22\x3E\x00\x3E\x02\x1E\x02\x3E\x00\x00\x2E\x2A\x3A\x00\x3E\x20\x20\x00\x3C\x0A\x3C\x00\x0E\x38\x0E\x00\x3E\x2A\x22\x00\x3E\x1A\x2E";

    uint8_t x = 30;
    uint8_t y = 8;

    ssd1306_text(10, 10, "THE PHANTOM SLAYER", false, false);
    ssd1306_draw();
    sleep_ms(10000);
    ssd1306_clear();
    ssd1306_draw();


    x = 0;
    y = 0;
    int8_t dx = 2;
    int8_t dy = 1;

    uint8_t d = 99;
    */
    map_init();
    show_map(0);
    ssd1306_draw();

    sleep_ms(5000);
    ssd1306_clear();
    ssd1306_draw();
    ssd1306_inverse(true);

    /*
    for (uint8_t i = 0 ; i < 7 ; ++i) {
        Rect r = rects[i];
        ssd1306_rect(r.origin_x, r. origin_y, r.width, r.height, 1, false);
    }

    ssd1306_line(63,0,63,63,1,1);
    ssd1306_draw();
    sleep_ms(5000);
    */

    while (1) {
        // Set up a new round...
        //play_intro();

        // Set up the environment
        init_game();
        create_world();

        player_x = 0;
        player_y = 0;
        player_direction = 1;

        draw_world();
        //check_senses();

        // ...and start play
        //game_loop();
    }

    return 0;
}