#ifndef BH_VIDEO_LAYOUT_H
#define BH_VIDEO_LAYOUT_H
/* SDL presentation policy. Original 640x480 coordinates remain a centered,
   uniformly scaled safe area; wider/taller outputs reveal additional world. */
static void video_layout_canvas(float aspect, float *margin_x, float *margin_y)
{
    *margin_x = *margin_y = 0;
    /* The x86 engine and adapter can evaluate 4/3 at different intermediate
       precision. Tiny rounding margins must not enable a widescreen layout. */
    if (aspect > 4.0 / 3.0 + .000001)
        *margin_x = (480.0f * aspect - 640.0f) * .5f;
    else if (aspect > 0 && aspect < 4.0 / 3.0 - .000001)
        *margin_y = (640.0f / aspect - 480.0f) * .5f;
}
static int video_layout_aspect_matches(int width, int height, int numerator, int denominator)
{
    double difference = (double)width * denominator - (double)height * numerator;
    double tolerance = (double)height * numerator * (numerator == 21 ? .03 : .003);
    return difference >= -tolerance && difference <= tolerance;
}
/* Fit once into the actual output. Never stretch the chosen render aspect or
   crop its expanded world; the unused output pixels are cleared by the blit. */
static void video_layout_fit(unsigned source_w, unsigned source_h, unsigned output_w, unsigned output_h,
                             unsigned *x, unsigned *y, unsigned *width, unsigned *height)
{
    *width = output_w;
    *height = output_h;
    if ((double)source_w * output_h > (double)output_w * source_h)
        *height = (unsigned)((double)output_w * source_h / source_w + .5);
    else
        *width = (unsigned)((double)output_h * source_w / source_h + .5);
    if (!*width) *width = 1;
    if (!*height) *height = 1;
    *x = (output_w - *width) / 2;
    *y = (output_h - *height) / 2;
}
#endif
