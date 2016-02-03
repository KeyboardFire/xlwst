#include <stdlib.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <png.h>

#define PNG_BIT_DEPTH 8

int main(int argc, char *argv[]) {
    /* resources:
     * http://stackoverflow.com/q/8249669/1223693
     * https://tronche.com/gui/x/xlib/graphics/XGetImage.html
     * http://zarb.org/~gc/html/libpng.html
     */

    // obtain image data with xlib
    Display *display = XOpenDisplay(NULL);
    Window root = DefaultRootWindow(display);
    XWindowAttributes wa;
    XGetWindowAttributes(display, root, &wa);
    int w = wa.width, h = wa.height;
    XImage *img = XGetImage(display, root, 0, 0, w, h, AllPlanes, ZPixmap);

    // convert to pnglib-friendly format
    png_bytep *png = (png_bytep*) malloc(sizeof(png_bytep) * h);
    for (int y = 0; y < h; ++y) {
        png[y] = (png_byte*) malloc(sizeof(png_byte) * w * 3);
        for (int x = 0; x < w; ++x) {
            png_byte *byte = png[y] + x * 3;
            unsigned long pixel = XGetPixel(img, x, y);
            byte[0] = (pixel & img->red_mask) >> 16;
            byte[1] = (pixel & img->green_mask) >> 8;
            byte[2] = (pixel & img->blue_mask) >> 0;
        }
    }

    // the output file we're writing the PNG to
    FILE *fp = fopen("out.png", "wb");
    if (!fp) {
        fprintf(stderr, "failed to open out.png\n");
        return 1;
    }

    // libpng stuff
    png_structp imgPtr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL,
            NULL, NULL);
    if (!imgPtr) {
        fprintf(stderr, "failed to create PNG write struct\n");
        return 1;
    }

    png_infop infoPtr = png_create_info_struct(imgPtr);
    if (!infoPtr) {
        fprintf(stderr, "failed to create PNG info struct\n");
        return 1;
    }

    if (setjmp(png_jmpbuf(imgPtr))) {
        fprintf(stderr, "error initializing PNG IO\n");
        return 1;
    }
    png_init_io(imgPtr, fp);

    if (setjmp(png_jmpbuf(imgPtr))) {
        fprintf(stderr, "error writing PNG header\n");
        return 1;
    }
    png_set_IHDR(imgPtr, infoPtr, w, h, PNG_BIT_DEPTH, PNG_COLOR_TYPE_RGB,
            PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT,
            PNG_FILTER_TYPE_DEFAULT);
    png_write_info(imgPtr, infoPtr);

    if (setjmp(png_jmpbuf(imgPtr))) {
        fprintf(stderr, "error writing PNG data\n");
        return 1;
    }
    png_write_image(imgPtr, png);

    if (setjmp(png_jmpbuf(imgPtr))) {
        fprintf(stderr, "error finishing PNG write\n");
        return 1;
    }
    png_write_end(imgPtr, NULL);

    fclose(fp);
}
