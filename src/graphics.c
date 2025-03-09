#include "graphics.h"
#include "display_driver.h"
#include "typedef.h"


/// <summary>
/// Draw a line.
/// </summary>
/// <param name="line">The line that'll be drawn.</param>
/// <returns>
/// 1 if the line could be drawn inside the bounds of the screen.
/// 0, otherwise.
/// </returns>
int draw_line(P_Line line)
{
    i8 x0, y0, x1, y1;

    x0 = line->p0.x;
    y0 = line->p0.y;

    x1 = line->p1.x;
    y1 = line->p1.y;

    bool steep = abs(y1 - y0) > abs(x1 - x0);

    if (steep)
    {
        swap(&x0, &y0);
        swap(&x1, &y1);
    }

    if (x0 > x1)
    {
        swap(&x0, &x1);
        swap(&y0, &y1);
    }

    i8 delta_x = x1 - x0;
    i8 delta_y = abs(y1 - y0);

    i8 error = 0;
    i8 y     = y0;

    i8 y_step = y0 < y1 ? 1 : -1;

    for (i8 x = x0; x <= x1; x++)
    {
        if (steep) {
            // Check boundaries before drawing
            if (y >= 0 && y < 128 && x >= 0 && x < 64) {
                graphic_pixel_set(y, x);
            }
        }
        else {
            // Check boundaries before drawing
            if (x >= 0 && x < 128 && y >= 0 && y < 64) {
                graphic_pixel_set(x, y);
            }
        }

        error += delta_y;
        if (error >= delta_x)
        {
            y     += y_step;
            error -= delta_x;
        }
    }
}


/// <summary>
/// Draw a rect.
/// </summary>
/// <param name="rect">The rect that'll be drawn.</param>
/// <returns>
/// 1 if the rect could be drawn inside the bounds of the screen.
/// 0, otherwise.
/// </returns>
int draw_rect(P_Rect rect)
{
    i8 p_x    = rect->origin.x;
    i8 p_y    = rect->origin.y;
    i8 width  = rect->dimen.x;
    i8 height = rect->dimen.y;

    // Draw top line
    Point start = { p_x,         p_y };
    Point end   = { p_x + width, p_y };

    Line  line = { start, end };
    draw_line(&line);
    // Draw right line
    line.p0.x = p_x + width;
    line.p1.y = p_y + height;
    draw_line(&line);
    // Draw bottom line
    line.p0.y = p_y + height;
    line.p0.x = p_x;
    draw_line(&line);
    // Draw left line
    line.p1.x = p_x;
    line.p1.y = p_y;
    draw_line(&line);
}


/// <summary>
/// Draw a polygon.
/// </summary>
/// <param name="poly">The polygon that'll be drawn.</param>
/// <returns>
/// 1 if the polygon could be drawn inside the bounds of the screen.
/// 0, otherwise.
/// </returns>
int draw_poly(P_PolyPoint poly)
{
    Point p0 = { poly->x, poly->y };
    P_PolyPoint next = poly->next;
    while (next)
    {
        Point p1 = { next->x, next->y };
        Line  l  = { p0, p1 };
        draw_line(&l);
        p0.x = p1.x;
        p0.y = p1.y;
        next = next->next;
    }
}


/// <summary>
/// Draw an object to the screen.
/// </summary>
/// <param name="obj">
/// The object whose pixels will be drawn to the screen.
/// </param>
void draw_object(P_Object obj)
{
    const int x = obj->pos_x;
    const int y = obj->pos_y;
    P_Point   arr = obj->geo->px;

    for (char i = 0; i < obj->geo->num_points; i++) {
        int px = x + arr[i].x;
        int py = y + arr[i].y;
        // Only draw if within screen bounds
        if (px >= 0 && px < 128 && py >= 0 && py < 64) {
            graphic_pixel_set(px, py);
        }
    }
}


/// <summary>
/// Clear an object's off the screen.
/// </summary>
/// <param name="obj">The object whose pixels to delete.</param>
void clear_object(P_Object obj)
{
    const int x = obj->pos_x;
    const int y = obj->pos_y;
    P_Point   arr = obj->geo->px;

    for (int i = 0; i < obj->geo->num_points; i++) {
        int px = x + arr[i].x;
        int py = y + arr[i].y;
        // Only clear if within screen bounds
        if (px >= 0 && px < 128 && py >= 0 && py < 64) {
            graphic_pixel_clear(px, py);
        }
    }
}


/// <summary>
/// Get the absolute value of a char.
/// </summary>
/// <param name="nr">
/// The value to retrieve the absolute value from.
/// </param>
i8 abs(i8 nr)
{
    if (nr < 0) return -nr;
    return nr;
}


/// <summary>
/// Swap the values of two chars.
/// </summary>
void swap(i8 *a, i8 *b)
{
    char temp = *a;
    *a = *b;
    *b = temp;
}