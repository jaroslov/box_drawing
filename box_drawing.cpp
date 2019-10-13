#include <stdio.h>
#include <stdlib.h>

#include "maboxdrawing.h"

#include "cxutf8232.hpp"

using namespace CXUTF8232;

template <int N> struct F { };

int main(int argc, char *argv[])
{
    {
        F<utf32("╳")> f; (void)f;
        fprintf(stdout, "%lX\n", utf32("foo"));
        fprintf(stdout, "%lX\n", utf32("╳"));

        utf8a X         = utf8(utf32("╳"));
        fprintf(stdout, "'%s'\n", &X.str[0]);

        utf8a cF        = utf8(utf32("f"));
        fprintf(stdout, "'%s'\n", &cF.str[0]);
    }

//    int colSizes[]      = { 8, -1 };
//    int rowSizes[]      = { 1, 1, 1, 1, };
//    BDCStr userData[]           =
//    {
//        { .begin = "THE TITLE", .end = 0, },
//        { .begin = "KEYA",      .end = 0, },
//        { .begin = "KEYB",      .end = 0, },
//        { .begin = "KEYC",      .end = 0, },
//        { .begin = "VALA",      .end = 0, },
//        { .begin = "VALB",      .end = 0, },
//        { .begin = "VALC",      .end = 0, },
//    };
//    BDCell cells[]      =
//    {
//        BDCell{ .col = 0, .row = 0, .colSpan = 2, .rowSpan = 1, .cellData = { .userData = &userData[0] } },
//        BDCell{ .col = 0, .row = 1, .colSpan = 1, .rowSpan = 1, .cellData = { .userData = &userData[1] } },
//        BDCell{ .col = 0, .row = 2, .colSpan = 1, .rowSpan = 1, .cellData = { .userData = &userData[2] } },
//        BDCell{ .col = 0, .row = 3, .colSpan = 1, .rowSpan = 1, .cellData = { .userData = &userData[3] } },
//        BDCell{ .col = 1, .row = 1, .colSpan = 1, .rowSpan = 1, .cellData = { .userData = &userData[4] } },
//        BDCell{ .col = 1, .row = 2, .colSpan = 1, .rowSpan = 1, .cellData = { .userData = &userData[5] } },
//        BDCell{ .col = 1, .row = 3, .colSpan = 1, .rowSpan = 1, .cellData = { .userData = &userData[6] } },
//    };
//
//    for (int BC = 0; BC < 2; ++BC)
//    {
//        BDLayoutInfo bag =
//        {
//            .numCols        = 2,
//            .numRows        = 4,
//            .numCells       = 7,
//            .maxTotalWidth  = 72,
//            .borderCollapse = BC,
//            .border         = { BD_THIN, BD_THIN, BD_THIN, BD_THIN, },
//            .padding        = { !BC, !BC, !BC, !BC, },
//            .margin         = { 0, 0, 0, 0, },
//            .colSizes       = &colSizes[0],
//            .rowSizes       = &rowSizes[0],
//            .cells          = &cells[0],
//            .measureText    = &bdMeasureText,
//            .getTextEntry   = &bdGetTextEntry,
//        };
//
//        char* table         = bdDrawTable(&bag);
//        fprintf(stdout, "%s\n", table);
//        free(table);
//    }

    {
        BDASCIISizedCell cells[] =
        {
            { .col = 0, .row = 0, .colSpan = 2,     .text = "The Title." },
            {           .row = 1, .minWidth = 8,    .text = "key-a" },
            {           .row = 2,                   .text = "key-b" },
            {           .row = 3,                   .text = "key-c" },
            { .col = 1, .row = 1,                   .text = "val-a" },
            { .col = 1, .row = 2,                   .text = "val-b" },
            { .col = 1, .row = 3,                   .text = "val-c" },
        };
        char* table         = bdDrawASCIITable(7, &cells[0], 72);
        fprintf(stdout, "%s\n", table);
        free(table);
    }

    return 0;
}

#define MEGAAWESOME_BOX_DRAWING_CPP
#include "maboxdrawing.h"