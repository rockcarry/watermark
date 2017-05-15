#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

// 内部常量定义
#ifndef MAX_PATH
#define MAX_PATH  256
#endif

// 内部类型定义
#pragma pack(1)
typedef struct tagBITMAPFILEHEADER { 
    uint16_t   bfType;
    uint32_t   bfSize;
    uint16_t   bfReserved1;
    uint16_t   bfReserved2;
    uint32_t   bfOffBits;
    uint32_t   biSize;
    int32_t    biWidth;
    int32_t    biHeight;
    uint16_t   biPlanes;
    uint16_t   biBitCount;
    uint32_t   biCompression;
    uint32_t   biSizeImage;
    int32_t    biXPelsPerMeter;
    int32_t    biYPelsPerMeter;
    uint32_t   biClrUsed;
    uint32_t   biClrImportant;
} BMPFILEHEADER;
#pragma pack()

static char *text = "\
#define FONT_WIDTH  %d\r\n\
#define FONT_HEIGHT %d\r\n\
static void watermark_putchar(void *ptr, int width, int x, int y, char c) {\r\n\
    char *src;\r\n\
    char *dst;\r\n\
    int  n = 54;\r\n\
    int  i, j;\r\n\
\r\n\
    if (c >= '(' && c <= ']') {\r\n\
        n = c - '(';\r\n\
    }\r\n\
\r\n\
    src = watermark_font + n * FONT_WIDTH * FONT_HEIGHT;\r\n\
    dst = (char*)ptr + y * width + x;\r\n\
\r\n\
    for (i=0; i<FONT_HEIGHT; i++) {\r\n\
        for (j=0; j<FONT_WIDTH; j++) {\r\n\
            if (*src) {\r\n\
                *dst = *src;\r\n\
            }\r\n\
            src++; dst++;\r\n\
        }\r\n\
        dst -= FONT_WIDTH;\r\n\
        dst += width;\r\n\
    }\r\n\
}\r\n\
\r\n\
static void watermark_putstring(void *ptr, int width, int x, int y, char *str) {\r\n\
    int curx = x;\r\n\
    int cury = y;\r\n\
    while (*str) {\r\n\
        if (*str == '\\n') {\r\n\
            cury += FONT_HEIGHT;\r\n\
            curx  = x;\r\n\
        }\r\n\
        else {\r\n\
            watermark_putchar(ptr, width, curx, cury, *str);\r\n\
            curx += FONT_WIDTH;\r\n\
        }\r\n\
        str++;\r\n\
    }\r\n\
}\r\n\
\r\n\
#endif\r\n\
\r\n\
";

// 内部函数实现
static int bmp24_to_font(const char *bmp24, const char *font, int flag, int cw, int ch)
{
    BMPFILEHEADER header = {0};
    int   offset  = 0;
    int   width   = 0;
    int   height  = 0;
    int   planes  = 0;
    int   bitcnt  = 0;
    int   compress= 0;
    int   stride  = 0;
    int   ret     = -1;
    int   rows, cols, x, y;
    int   m, n, i, j, r, g, b, c;

    FILE *fpsrc = fopen(bmp24, "rb");
    FILE *fpdst = fopen(font , "wb");

    if (fpsrc && fpdst) {
        fread(&header, sizeof(header), 1, fpsrc);
        offset   = header.bfOffBits;
        width    = header.biWidth;
        height   = header.biHeight;
        planes   = header.biPlanes;
        bitcnt   = header.biBitCount;
        compress = header.biCompression;
        rows     = height / ch;
        cols     = width  / cw;
        stride   =(width * 3 + 3) / 4 * 4;
//      printf("offset = %d, width = %d, height = %d, planes = %d, bitcnt = %d, compress = %d\n", offset, width, height, planes, bitcnt, compress);
        if (planes != 1 || compress != 0) {
            goto done;
        }

        if (flag) {
            fprintf(fpdst, "#ifndef __WATERMARK_FONT_H__\r\n");
            fprintf(fpdst, "#define __WATERMARK_FONT_H__\r\n\r\n");
            fprintf(fpdst, "static char watermark_font[] = {\r\n");
        }

        for (m=0,n=0; n<rows*cols; n++) {
            x = (n % cols) * cw;
            y = (n / cols) * ch;
            for (i=0; i<ch; i++,y++) {
                fseek(fpsrc, header.bfOffBits + stride * (height - y - 1) + x * 3, SEEK_SET);
                for (j=0; j<cw; j++) {
                    r = fgetc(fpsrc);
                    g = fgetc(fpsrc);
                    b = fgetc(fpsrc);
                    c = (r + g + b) / 3;
                    if (flag) {
                        fprintf(fpdst, "0x%02x, ", c);
                        if (++m % 16 == 0) {
                            fprintf(fpdst, "\r\n");
                        }
                    }
                    else {
                        fputc(c, fpdst);
                    }
                }
            }
        }

        if (flag) {
            if (m % 16 != 0) {
                fprintf(fpdst, "\r\n");
            }
            fprintf(fpdst, "};\r\n\r\n");
            fprintf(fpdst, text, cw, ch);
        }
        ret = 0;
    }

done:
    fclose(fpsrc);
    fclose(fpdst);
    return ret;
}

int main(int argc, char *argv[])
{
    char dst[256];
    int  flag = 0;
    int  len  = 0;

    if (argc < 4) {
        return -1;
    }

    strcpy(dst, argv[1]);
    len = strlen(dst);

    if (argc >= 5 && strcmp(argv[4], "-c") == 0) {
        flag = 1;
        dst[len - 3] = 'h';
        dst[len - 2] = '\0';
    }
    else {
        flag = 0;
        dst[len - 3] = 'f';
        dst[len - 2] = 'n';
        dst[len - 1] = 't';
    }

    return bmp24_to_font(argv[1], dst, flag, atoi(argv[2]), atoi(argv[3]));
}


