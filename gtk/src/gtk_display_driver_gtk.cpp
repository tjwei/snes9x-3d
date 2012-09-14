#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <cairo.h>
#include "gtk_display.h"
#include "gtk_display_driver_gtk.h"

S9xGTKDisplayDriver::S9xGTKDisplayDriver (Snes9xWindow *window,
                                          Snes9xConfig *config)
{
    this->window = window;
    this->config = config;
    this->drawing_area = GTK_WIDGET (window->drawing_area);
    this->pixbuf = NULL;

    return;
}
//SSS

void
S9xGTKDisplayDriver::update (int width, int height)
{
    int           x, y, w, h;
    int           c_width, c_height, final_pitch, width2, height2;
    uint8         *final_buffer1, *final_buffer2;
    GtkAllocation allocation;
    width2=width;
    height2=height;
    gtk_widget_get_allocation (drawing_area, &allocation);
    c_width = allocation.width;
    c_height = allocation.height;

    if (width == SIZE_FLAG_DIRTY)
    {
        this->clear ();
        return;
    }

    if (width <= 0)
        return;
    
    if (config->scale_method > 0)
    {
        uint8 *src_buffer1 = (uint8 *) padded_buffer1[0];
        uint8 *dst_buffer1 = (uint8 *) padded_buffer1[1];
        uint8 *src_buffer2 = (uint8 *) padded_buffer2[0];
        uint8 *dst_buffer2 = (uint8 *) padded_buffer2[1];
        int   src_pitch = image_width * image_bpp;
        int   dst_pitch = scaled_max_width * image_bpp;

        S9xFilter (src_buffer1,
                   src_pitch,
                   dst_buffer1,
                   dst_pitch,
                   width,
                   height);
       S9xFilter (src_buffer2,
                   src_pitch,
                   dst_buffer2,
                   dst_pitch,
                   width2,
                   height2);
        final_buffer1 = (uint8 *) padded_buffer1[1];
        final_buffer2 = (uint8 *) padded_buffer2[1];
        final_pitch = dst_pitch;
    }
    else
    {
        final_buffer1 = (uint8 *) padded_buffer1[0];
        final_buffer2 = (uint8 *) padded_buffer2[0];
        final_pitch = image_width * image_bpp;
    }
    x = width; y = height; w = c_width; h = c_height;
    S9xApplyAspect (x, y, w, h);
    output (final_buffer1, final_buffer2, final_pitch, x, y, width, height, w, h);

    return;
}
//SSSSSSS
void
S9xGTKDisplayDriver::output (void *src1,
                             void *src2,
                             int  src_pitch,
                             int  x,
                             int  y,
                             int  width,
                             int  height,
                             int  dst_width,
                             int  dst_height)
{
    if (width != gdk_buffer_width || height != gdk_buffer_height)
    {
        gdk_buffer_width = width;
        gdk_buffer_height = height;

        gdk_pixbuf_unref (pixbuf);

        padded_buffer1[2] = realloc (padded_buffer1[2],
                                    gdk_buffer_width * gdk_buffer_height * 3);
        padded_buffer2[2] = realloc (padded_buffer2[2],
                                    gdk_buffer_width * gdk_buffer_height * 3);
                                    
        pixbuf = gdk_pixbuf_new_from_data ((guchar *) padded_buffer2[2],
                                           GDK_COLORSPACE_RGB,
                                           FALSE,
                                           8,
                                           gdk_buffer_width,
                                           gdk_buffer_height,
                                           gdk_buffer_width * 3,
                                           NULL,
                                           NULL);

    }

    if (last_known_width != dst_width || last_known_height != dst_height)
    {
        clear ();

        last_known_width = dst_width;
        last_known_height = dst_height;
    }

    S9xConvert (src1,
                padded_buffer1[2],
                src_pitch,
                gdk_buffer_width * 3,
                width,
                height,
                24);
    if(src2)                
    S9xConvert (src2,
                padded_buffer2[2],
                src_pitch,
                gdk_buffer_width * 3,
                width,
                height,
                24);
    uint8 *src_buffer = (uint8 *) padded_buffer1[2];
    uint8 *dst_buffer = (uint8 *) padded_buffer2[2];
       dst_buffer+=gdk_buffer_width * 3;
       for(int y=1;y<gdk_buffer_height;y+=2){
            memcpy(dst_buffer, src_buffer, gdk_buffer_width * 3);
            dst_buffer += gdk_buffer_width * 6;
            src_buffer += gdk_buffer_width * 6;
        }

    cairo_t *cr = gdk_cairo_create (gtk_widget_get_window (drawing_area));

    gdk_cairo_set_source_pixbuf (cr, pixbuf, x, y);

    if (width != dst_width || height != dst_height)
    {
        cairo_matrix_t matrix;
        cairo_pattern_t *pattern = cairo_get_source (cr);;

        cairo_matrix_init_identity (&matrix);
        cairo_matrix_scale (&matrix,
                            (double) width / (double) dst_width,
                            (double) height / (double) dst_height);
        cairo_matrix_translate (&matrix, -x, -y);
        cairo_pattern_set_matrix (pattern, &matrix);
        cairo_pattern_set_filter (pattern,
                                  config->bilinear_filter
                                       ? CAIRO_FILTER_BILINEAR
                                       : CAIRO_FILTER_NEAREST);
    }

    cairo_rectangle (cr, x, y, dst_width, dst_height);
    cairo_fill (cr);

    cairo_destroy (cr);

    window->set_mouseable_area (x, y, width, height);

    return;
}

int
S9xGTKDisplayDriver::init (void)
{
    int padding;
    GtkAllocation allocation;
    buffer1[0] = malloc (image_padded_size+1024);
    buffer1[1] = malloc (scaled_padded_size+1024);
    buffer2[0] = malloc (image_padded_size+1024);
    buffer2[1] = malloc (scaled_padded_size+1024);

    padding = (image_padded_size - image_size) / 2+512;
    padded_buffer1[0] = (void *) (((uint8 *) buffer1[0]) + padding);
    padded_buffer2[0] = (void *) (((uint8 *) buffer2[0]) + padding);

    padding = (scaled_padded_size - scaled_size) / 2;
    padded_buffer1[1] = (void *) (((uint8 *) buffer1[1]) + padding);
    padded_buffer2[1] = (void *) (((uint8 *) buffer2[1]) + padding);

    gtk_widget_get_allocation (drawing_area, &allocation);
    gdk_buffer_width = allocation.width;
    gdk_buffer_height = allocation.height;
    padded_buffer1[2] = malloc (gdk_buffer_width * gdk_buffer_height * 3);
    padded_buffer2[2] = malloc (gdk_buffer_width * gdk_buffer_height * 3);
    //SSSSSSS
    pixbuf = gdk_pixbuf_new_from_data ((guchar *) padded_buffer2[2],
                                       GDK_COLORSPACE_RGB,
                                       FALSE,
                                       8,
                                       gdk_buffer_width,
                                       gdk_buffer_height,
                                       gdk_buffer_width * 3,
                                       NULL,
                                       NULL);

    S9xSetEndianess (ENDIAN_MSB);
    //SSS
    memset (buffer1[0], 0, image_padded_size);
    memset (buffer1[1], 0, scaled_padded_size);
    memset (buffer2[0], 0, image_padded_size);
    memset (buffer2[1], 0, scaled_padded_size);

    GFX.Screen1 = (uint16 *) padded_buffer1[0];
    GFX.Screen2 = (uint16 *) padded_buffer2[0];
    GFX.Pitch = image_width * image_bpp;

    return 0;
}

void
S9xGTKDisplayDriver::deinit (void)
{
    padded_buffer1[0] = padded_buffer2[0] = NULL;
    padded_buffer1[1] = padded_buffer2[0] = NULL;

    free (buffer1[0]);
    free (buffer1[1]);
    free (buffer2[0]);
    free (buffer2[1]);

    gdk_pixbuf_unref (pixbuf);
    free (padded_buffer1[2]);
    free (padded_buffer2[2]);

    return;
}

void
S9xGTKDisplayDriver::clear (void)
{
    int  x, y, w, h;
    int  width, height;
    GtkAllocation allocation;

    gtk_widget_get_allocation (drawing_area, &allocation);
    width = allocation.width;
    height = allocation.height;

    cairo_t *cr = gdk_cairo_create (gtk_widget_get_window (drawing_area));

    cairo_set_source_rgb (cr, 0.0, 0.0, 0.0);

    if (window->last_width <= 0 || window->last_height <= 0)
    {
        cairo_paint (cr);
        cairo_destroy (cr);

        return;
    }

    x = window->last_width;
    y = window->last_height;
    get_filter_scale (x, y);
    w = width;
    h = height;
    S9xApplyAspect (x, y, w, h);

    if (x > 0)
    {
        cairo_rectangle (cr, 0, y, x, h);
    }
    if (x + w < width)
    {
        cairo_rectangle (cr, x + w, y, width - (x + w), h);
    }
    if (y > 0)
    {
        cairo_rectangle (cr, 0, 0, width, y);
    }
    if (y + h < height)
    {
        cairo_rectangle (cr, 0, y + h, width, height - (y + h));
    }

    cairo_fill (cr);
    cairo_destroy (cr);

    return;
}

void
S9xGTKDisplayDriver::refresh (int width, int height)
{
    if (!config->rom_loaded)
        return;

    clear ();

    return;
}

uint16 *
S9xGTKDisplayDriver::get_next_buffer1 (void)
{
    return (uint16 *) padded_buffer1[0];
}

uint16 *
S9xGTKDisplayDriver::get_next_buffer2 (void)
{
    return (uint16 *) padded_buffer2[0];
}

uint16 *
S9xGTKDisplayDriver::get_current_buffer1 (void)
{
    return (uint16 *) padded_buffer1[0];
}

uint16 *
S9xGTKDisplayDriver::get_current_buffer2 (void)
{
    return (uint16 *) padded_buffer2[0];
}


void
S9xGTKDisplayDriver::push_buffer (uint16 *src1, uint16 *src2)
{
    memmove (GFX.Screen1, src1, image_size);
    memmove (GFX.Screen2, src2, image_size);
    update (window->last_width, window->last_height);

    return;
}

void
S9xGTKDisplayDriver::clear_buffers (void)
{
    memset (buffer1[0], 0, image_padded_size);
    memset (buffer1[1], 0, scaled_padded_size);
    memset (buffer2[0], 0, image_padded_size);
    memset (buffer2[1], 0, scaled_padded_size);

    return;
}

void
S9xGTKDisplayDriver::reconfigure (int width, int height)
{
    return;
}
