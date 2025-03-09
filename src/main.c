// main.c - Modified version for Breakout with ball repositioning on wall collision

#include "typedef.h"
#include "memreg.h"
#include "delay.h"
#include "display_driver.h"
#include "graphics.h"
#include "keyb.h"
#include "ascii_game.h"
#include "ascii.h"

// Registers
static gpio_t    *gpiod   = (gpio_t*)   GPIOD;
static gpio_t    *gpioe   = (gpio_t*)   GPIOE;
static systick_t *systick = (systick_t*)SYSTICK;

// Constants
#define PADDLE_MOVE_LEFT  1
#define PADDLE_MOVE_RIGHT 3
#define SPEED             8  // Increased from 2 to 4 for faster paddle movement
#define MAX_BRICKS        30
#define BRICK_ROWS        4
#define BRICK_COLS        8
#define BRICK_HEIGHT      4
// BRICK_WIDTH is now calculated dynamically in init_bricks() to prevent overlap
#define SCREEN_WIDTH      128
#define SCREEN_HEIGHT     64
#define MAX_LIVES         3

// Helper functions for min and max
int min(int a, int b) {
    return a < b ? a : b;
}

int max(int a, int b) {
    return a > b ? a : b;
}

// Setup
void app_init(void)
{
    // Start clocks for port D and port E.
    *(ulong*)0x40023830 = 0x18;
    // Start clocks for SYSCFG
    *(ulong*)0x40023844 |= 0x4000;

    gpiod->MODER_LOW  =     0x5555;
    gpiod->MODER_HIGH =     0x5500;
    gpiod->OSPEEDR    = 0x55555555;
    gpiod->OTYPER    &=     0x00FF;
    gpiod->PUPDR     &= 0x0000FFFF;
    gpiod->PUPDR     |= 0x00AA0000;

    gpioe->MODER   = 0x00005555;
    gpioe->OSPEEDR = 0x55555555;
}

// Helper functions
void wait_for_start_press()
{
    bool start = false;
    while (true)
    {
        Input *keyb_input = keyb();

        for (u8 i = 0; i < keyb_input->n_presses; i++)
        {
            u8 current_input = keyb_input->buffer[i];
            if (current_input == 5) {
                start = true;
                break;
            }
        }

        if (start) break;
    }
}

/**
* @brief Checks if the ball collides with the paddle.
*/
bool colliding_with_paddle(P_Object ball, P_Object paddle)
{
    i8 ball_min_x = ball->pos_x;
    i8 ball_max_x = ball->pos_x + ball->geo->size_x;
    i8 ball_min_y = ball->pos_y;
    i8 ball_max_y = ball->pos_y + ball->geo->size_y;

    i8 paddle_min_x = paddle->pos_x;
    i8 paddle_max_x = paddle->pos_x + paddle->geo->size_x;
    i8 paddle_min_y = paddle->pos_y;
    i8 paddle_max_y = paddle->pos_y + paddle->geo->size_y;

    return
        ball_min_x <= paddle_max_x
        &&
        ball_max_x >= paddle_min_x
        &&
        ball_min_y <= paddle_max_y
        &&
        ball_max_y >= paddle_min_y;
}

/**
* @brief Moves an object one "tick" by updating its coordinates with its speed.
*/
void move_object(P_Object object)
{
    // Clear the object temporarily from the screen
    object->clear(object);

    // Update the position of the object
    object->pos_x += object->dir_x;
    object->pos_y += object->dir_y;

    // Draw the object with its new positions
    draw_object(object);
}

/**
* @brief Sets the speed of the given object
*/
void set_object_speed(P_Object object, int speed_x, int speed_y)
{
    object->dir_x = speed_x;
    object->dir_y = speed_y;
}

/**
* @brief Checks if the ball collides with a wall.
*/
WallCollision check_wall_collision(P_Object ball)
{
    i16 ball_min_x = ball->pos_x;
    i16 ball_max_x = ball->pos_x + ball->geo->size_x;
    i16 ball_min_y = ball->pos_y;
    i16 ball_max_y = ball->pos_y + ball->geo->size_y;
    WallCollision result;

    // Check left wall collision
    if (ball_min_x < 1) {
        result = (WallCollision) { .is_colliding = true, .which = 'l'};
        return result;
    }
    // Check right wall collision
    if (ball_max_x > SCREEN_WIDTH) {
        result = (WallCollision) { .is_colliding = true, .which = 'r'};
        return result;
    }
    // Check upper wall collision
    if (ball_min_y < 1) {
        result = (WallCollision) { .is_colliding = true, .which = 'u'};
        return result;
    }
    // Check lower wall collision (game over)
    if (ball_max_y > SCREEN_HEIGHT) {
        result = (WallCollision) { .is_colliding = true, .which = 'd'};
        return result;
    }
    // When no wall collision occurs
    result = (WallCollision){ false };
    return result;
}

/**
* @brief Reset the ball and paddle to initial positions
*/
void reset_game_objects(P_Object ball, P_Object paddle)
{
    // Reset ball - start from the middle of the paddle with upward trajectory
    ball->dir_x = 4;  // Increased to 4 for faster horizontal movement
    ball->dir_y = -5; // Increased to -5 for faster vertical movement
    ball->pos_x = SCREEN_WIDTH / 2 - ball->geo->size_x / 2;
    ball->pos_y = SCREEN_HEIGHT - 12; // Start a bit higher for better visibility

    // Reset paddle
    paddle->dir_x = 0;
    paddle->dir_y = 0;
    paddle->pos_x = SCREEN_WIDTH / 2 - paddle->geo->size_x / 2;
    paddle->pos_y = SCREEN_HEIGHT - 6;
}

/**
* @brief Checks if ball collides with a brick
*/
bool colliding_with_brick(P_Object ball, Rect brick)
{
    i8 ball_min_x = ball->pos_x;
    i8 ball_max_x = ball->pos_x + ball->geo->size_x;
    i8 ball_min_y = ball->pos_y;
    i8 ball_max_y = ball->pos_y + ball->geo->size_y;

    i8 brick_min_x = brick.origin.x;
    i8 brick_max_x = brick.origin.x + brick.dimen.x;
    i8 brick_min_y = brick.origin.y;
    i8 brick_max_y = brick.origin.y + brick.dimen.y;

    return
        ball_min_x <= brick_max_x
        &&
        ball_max_x >= brick_min_x
        &&
        ball_min_y <= brick_max_y
        &&
        ball_max_y >= brick_min_y;
}

/**
* @brief Display game over screen with final score
*/
void game_over(int score, bool win)
{
    graphic_clear_screen();
    ascii_command(0b00000001, delay_milli, 2); // Clear display
    
    char* game_result = win ? "You Win!" : "Game Over!";
    char score_text[] = "Score: ";
    char* s;
    
    ascii_goto(1, 1);
    s = game_result;
    while (*s)
        ascii_write_char(*s++);
        
    ascii_goto(1, 2);
    s = score_text;
    while (*s)
        ascii_write_char(*s++);
        
    // Convert score to string and display
    if (score >= 100)
        ascii_write_char('0' + score / 100);
    if (score >= 10)
        ascii_write_char('0' + (score % 100) / 10);
    ascii_write_char('0' + (score % 10));
    
    delay_milli(5000);
}

/**
* @brief Show welcome screen
*/
void breakout_start_screen(void)
{
    ascii_command(0b00000001, delay_milli, 2);

    char welcome[] = "Welcome to Breakout!";
    char press[]   = "Press 5 to start.";

    ascii_goto(1, 1);
    
    char *s = welcome;
    while (*s)
        ascii_write_char(*s++);

    ascii_goto(1, 2);

    s = press;
    while (*s)
        ascii_write_char(*s++);
}

/**
* @brief Display score and lives on ascii display
*/
void display_game_info(int score, int lives)
{
    // Display score
    ascii_goto(1, 2);
    char score_text[] = "Score: ";
    char* s = score_text;
    while (*s)
        ascii_write_char(*s++);
    
    // Convert score to string and display
    if (score >= 100)
        ascii_write_char('0' + score / 100);
    if (score >= 10)
        ascii_write_char('0' + (score % 100) / 10);
    ascii_write_char('0' + (score % 10));
    
    // Display lives
    ascii_goto(12, 2);
    char lives_text[] = "Lives: ";
    s = lives_text;
    while (*s)
        ascii_write_char(*s++);
    ascii_write_char('0' + lives);
}

/**
* @brief Initialize brick layout for the game
*/
int init_bricks(Rect bricks[], bool brick_active[])
{
    int brick_count = 0;
    int brick_margin_x = 3;  // Increased horizontal margin between bricks
    int brick_margin_y = 3;  // Increased vertical margin between bricks
    int start_x = 4;         // Starting x position for the first brick
    int start_y = 4;         // Starting y position for the first brick
    
    // Calculate actual brick width to fit screen properly
    int actual_brick_width = (SCREEN_WIDTH - 2*start_x - (BRICK_COLS-1)*brick_margin_x) / BRICK_COLS;
    
    for (int row = 0; row < BRICK_ROWS; row++) {
        for (int col = 0; col < BRICK_COLS; col++) {
            int index = row * BRICK_COLS + col;
            Point origin = {
                start_x + col * (actual_brick_width + brick_margin_x),
                start_y + row * (BRICK_HEIGHT + brick_margin_y)
            };
            Point dimen = {actual_brick_width, BRICK_HEIGHT};
            bricks[index] = (Rect){origin, dimen};
            brick_active[index] = true;
            draw_rect(&bricks[index]);
            brick_count++;
        }
    }
    
    return brick_count;
}

// Geometries
Geometry ball_geometry = {
    12,     // numpoints
    4,      // size_x
    4,      // size_y
    {
        {0,1},{0,2},{1,0},{1,1},{1,2},{1,3},{2,0},{2,1},{2,2},{2,3},{3,1},{3,2}
    }
};

Geometry paddle_geometry = {
    10, // numpoints
    10,  // size_x
    2,  // size_y
    {
        {0,0},{1,0},{2,0},{3,0},{4,0},{5,0},{6,0},{7,0},{8,0},{9,0},  // Top row
    }
};

// Game objects
static Object ball = {
    &ball_geometry,
    0, 0,            // Initial direction
    1, 1,            // Initial startposition
    draw_object,
    clear_object,
    move_object,
    set_object_speed
};

static Object paddle = {
    &paddle_geometry,
    0, 0,                // Initial direction
    59, 58,              // Start position
    draw_object,
    clear_object,
    move_object,
    set_object_speed
};

int main(void)
{
    // Initialize application
    app_init();
    graphic_initialize();
    ascii_init();

    // Define brick array and state
    Rect bricks[MAX_BRICKS];
    bool brick_active[MAX_BRICKS] = {0};
    
    // Game variables
    int score = 0;
    int lives = MAX_LIVES;
    int brick_count = 0;
    int level = 1;
    int game_speed = 12; // milliseconds per frame - reduced to 12 for much faster gameplay

init_game:
    graphic_clear_screen();
    breakout_start_screen();
    wait_for_start_press();
    
    // Reset game state
    score = 0;
    lives = MAX_LIVES;
    level = 1;
    game_speed = 12; // Keep consistent with initial value
    
    // Initialize bricks for first level
    brick_count = init_bricks(bricks, brick_active);

new_life:
    // Clear entire screen to start fresh
    graphic_clear_screen();
    
    // Redraw active bricks completely from scratch
    for (int i = 0; i < BRICK_ROWS * BRICK_COLS; i++) {
        if (brick_active[i]) {
            // Create a fresh brick rect with the original parameters
            int row = i / BRICK_COLS;
            int col = i % BRICK_COLS;
            
            int brick_margin_x = 3;
            int brick_margin_y = 3;
            int start_x = 4;
            int start_y = 4;
            
            // Calculate brick width like in init_bricks
            int actual_brick_width = (SCREEN_WIDTH - 2*start_x - (BRICK_COLS-1)*brick_margin_x) / BRICK_COLS;
            
            // Recreate the brick with exact original dimensions
            Point origin = {
                start_x + col * (actual_brick_width + brick_margin_x),
                start_y + row * (BRICK_HEIGHT + brick_margin_y)
            };
            Point dimen = {actual_brick_width, BRICK_HEIGHT};
            
            // Update the brick in the array and redraw it
            bricks[i].origin = origin;
            bricks[i].dimen = dimen;
            draw_rect(&bricks[i]);
        }
    }
    
    // Make sure ball and paddle are properly cleared
    ball.clear(&ball);
    paddle.clear(&paddle);
    reset_game_objects(&ball, &paddle);
    
    // Display game info
    ascii_command(0b00000001, delay_milli, 2); // Clear display
    
    char breakout[] = "Breakout - Lvl ";
    
    ascii_goto(1, 1);
    char *s = breakout;
    while (*s)
        ascii_write_char(*s++);
    
    // Display level number
    ascii_write_char('0' + level);
    
    // Display score and lives
    display_game_info(score, lives);

    // Gameplay-loop
    while (true)
    {
        // Read input
        Input *keyb_input = keyb();
        i8 paddle_dx = 0;

        if (keyb_input->n_presses > 0)
        {
            for (u8 i = 0; i < keyb_input->n_presses; i++)
            {
                u8 current_input = keyb_input->buffer[i];
                switch (current_input)
                {
                case PADDLE_MOVE_LEFT:
                    paddle_dx--; break;
                case PADDLE_MOVE_RIGHT:
                    paddle_dx++; break;
                default:
                    break;
                }
            }
        }

        // Set paddle speed from input
        paddle.set_speed(&paddle, paddle_dx * SPEED, 0);

        // Only move the paddle if it stays inside the screen
        if ((paddle.pos_x + paddle.dir_x >= 1) && 
            (paddle.pos_x + paddle.dir_x + paddle.geo->size_x <= SCREEN_WIDTH)) {
            paddle.move(&paddle);
        }

        // Move ball
        ball.move(&ball);

        // Check for ball collision with paddle
        if (colliding_with_paddle(&ball, &paddle)) {
            // Bounce ball based on where it hit the paddle
            ball.dir_y = -abs(ball.dir_y); // Always bounce up
            
            // Adjust x direction based on where ball hit paddle
            int hit_pos = ball.pos_x - paddle.pos_x;
            int paddle_width = paddle.geo->size_x;
            
            // Divide paddle into 5 segments for more precise control
            if (hit_pos < paddle_width / 5) {
                // Far left - sharp angle
                ball.dir_x = -3;
            } else if (hit_pos < paddle_width * 2/5) {
                // Left side - medium angle
                ball.dir_x = -2;
            } else if (hit_pos < paddle_width * 3/5) {
                // Center - keep current x direction but ensure it's not 0
                if (ball.dir_x == 0) ball.dir_x = 1;
            } else if (hit_pos < paddle_width * 4/5) {
                // Right side - medium angle
                ball.dir_x = 2;
            } else {
                // Far right - sharp angle
                ball.dir_x = 3;
            }
        }
        
        // Check for ball collision with bricks
        bool brick_hit = false;
        for (int i = 0; i < BRICK_ROWS * BRICK_COLS; i++) {
            if (brick_active[i]) {
                if (colliding_with_brick(&ball, bricks[i])) {
                    // Determine which side of the brick was hit
                    i8 ball_cx = ball.pos_x + ball.geo->size_x / 2;
                    i8 ball_cy = ball.pos_y + ball.geo->size_y / 2;
                    i8 brick_cx = bricks[i].origin.x + bricks[i].dimen.x / 2;
                    i8 brick_cy = bricks[i].origin.y + bricks[i].dimen.y / 2;
                    
                    // Calculate distance from centers in each direction
                    i8 dx = ball_cx - brick_cx;
                    i8 dy = ball_cy - brick_cy;
                    
                    // Calculate overlap in each direction
                    i8 overlap_x = (ball.geo->size_x + bricks[i].dimen.x) / 2 - abs(dx);
                    i8 overlap_y = (ball.geo->size_y + bricks[i].dimen.y) / 2 - abs(dy);
                    
                    // Change direction based on smaller overlap (more likely collision side)
                    if (overlap_x < overlap_y) {
                        ball.dir_x *= -1;
                    } else {
                        ball.dir_y *= -1;
                    }
                    
                    // Remove the brick and increment score
                    brick_active[i] = false;
                    score += 10;
                    display_game_info(score, lives);
                    
                    // Clear the entire brick area properly
                    // First clear a slightly larger area to ensure all pixels are removed
                    int clear_x_start = max(0, bricks[i].origin.x - 1);
                    int clear_y_start = max(0, bricks[i].origin.y - 1);
                    int clear_x_end = min(SCREEN_WIDTH - 1, bricks[i].origin.x + bricks[i].dimen.x + 1);
                    int clear_y_end = min(SCREEN_HEIGHT - 1, bricks[i].origin.y + bricks[i].dimen.y + 1);
                    
                    for (int j = clear_x_start; j <= clear_x_end; j++) {
                        for (int k = clear_y_start; k <= clear_y_end; k++) {
                            graphic_pixel_clear(j, k);
                        }
                    }
                    
                    brick_count--;
                    brick_hit = true;
                    
                    // Check if all bricks are cleared
                    if (brick_count <= 0) {
                        // Level complete
                        level++;
                        graphic_clear_screen();
                        
                        // Display level complete message
                        ascii_command(0b00000001, delay_milli, 2);
                        char level_complete[] = "Level Complete!";
                        ascii_goto(1, 1);
                        s = level_complete;
                        while (*s)
                            ascii_write_char(*s++);
                            
                        char next_level[] = "Next Level...";
                        ascii_goto(1, 2);
                        s = next_level;
                        while (*s)
                            ascii_write_char(*s++);
                            
                        delay_milli(2000);
                        
                        // Increase difficulty
                        game_speed = game_speed > 10 ? game_speed - 5 : 10;
                        
                        // Reset bricks for next level
                        brick_count = init_bricks(bricks, brick_active);
                        goto new_life;
                    }
                    
                    // Only handle one brick collision per frame
                    break;
                }
            }
        }

        // Checks for ball collision with walls with repositioning fixes
        WallCollision wc = check_wall_collision(&ball);
        if (wc.is_colliding)
        {
            switch (wc.which)
            {
                case 'u': // Ball hit upper wall
                    ball.pos_y = 1;
                    ball.dir_y *= -1;
                    break;
                case 'l': // Ball hit left wall
                    ball.pos_x = 1;
                    ball.dir_x *= -1;
                    break;
                case 'r': // Ball hit right wall
                    ball.pos_x = SCREEN_WIDTH - ball.geo->size_x - 1;
                    ball.dir_x *= -1;
                    break;
                case 'd': // Ball hit bottom wall (lose a life)
                    lives--;
                    display_game_info(score, lives);
                    
                    if (lives <= 0) {
                        game_over(score, false);
                        goto init_game;
                    }
                    
                    // Small pause before continuing
                    delay_milli(1000);
                    goto new_life;
                default:
                    break;
            }
        }
        
        // If player has reached level 3 and cleared all bricks, they win!
        if (level >= 3 && brick_count <= 0) {
            game_over(score, true);
            goto init_game;
        }
        
        // Add a small delay to control game speed
        delay_milli(game_speed);
    }
    
    return 0;
}
