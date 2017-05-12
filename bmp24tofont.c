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
#define FONT_WIDTH  16 // todo\r\n\
#define FONT_HEIGHT 28 // todo\r\n\
static void watermark_putchar(char *ptr, int width, int x, int y, char c) {\r\n\
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
    dst = ptr + y * width + x;\r\n\
\r\n\
    for (i=0; i<FONT_HEIGHT; i++) {\r\n\
        for (j=0; j<FONT_WIDTH; j++) {\r\n\
            if (*src != 0) {\r\n\
                *dst = *src;\r\n\
            }\r\n\
            src++; dst++;\r\n\
        }\r\n\
        dst -= FONT_WIDTH;\r\n\
        dst += width;\r\n\
    }\r\n\
}\r\n\
\r\n\
static void watermark_putstring(char *ptr, int width, int x, int y, char *str) {\r\n\
    int curx = x;\r\n\
    int cury = y;\r\n\
    while (*str) {\r\n\
        if (*str == '|') {\r\n\
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
static int bmp24_to_font(const char *bmp24, const char *font, int flag)
{
    BMPFILEHEADER header = {0};
    int   offset  = 0;
    int   width   = 0;
    int   height  = 0;
    int   planes  = 0;
    int   bitcnt  = 0;
    int   compress= 0;
    int   stride  = 0;
    int   skip    = 0;
    int   ret     = -1;
    int   n, i, j, r, g, b, c;

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
//      printf("offset = %d, width = %d, height = %d, planes = %d, bitcnt = %d, compress = %d\n", offset, width, height, planes, bitcnt, compress);
        if (planes != 1 || compress != 0) {
            goto done;
        }

        if (flag) {
            fprintf(fpdst, "#ifndef __WATERMARK_FONT_H__\r\n");
            fprintf(fpdst, "#define __WATERMARK_FONT_H__\r\n\r\n");
            fprintf(fpdst, "static char watermark_font[] = {\r\n");
        }

        stride = (width * 3 + 3) / 4 * 4;
        skip   = stride - width * 3;
        fseek(fpsrc, offset + stride * (abs(height) - 1), SEEK_SET);
        for (n=0,i=0; i<abs(height); i++) {
            for (j=0; j<width; j++) {
                r = fgetc(fpsrc);
                g = fgetc(fpsrc);
                b = fgetc(fpsrc);
                c = (r + g + b) / 3;
                if (flag) {
                    fprintf(fpdst, "0x%02x, ", c);
                    if (++n % 16 == 0) {
                        fprintf(fpdst, "\r\n");
                    }
                }
                else {
                    fputc(c, fpdst);
                }
            }
            for (j=0; j<skip; j++) {
                fgetc(fpsrc);
            }
            fseek(fpsrc, -2 * stride, SEEK_CUR);
        }

        if (flag) {
            if (n % 16 != 0) {
                fprintf(fpdst, "\r\n");
            }
            fprintf(fpdst, "};\r\n\r\n");
            fprintf(fpdst, text);
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

    if (argc < 2) {
        return -1;
    }

    strcpy(dst, argv[1]);
    len = strlen(dst);

    if (argc >= 3 && strcmp(argv[2], "-c") == 0) {
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

    return bmp24_to_font(argv[1], dst, flag);
}


