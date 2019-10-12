#include <stdio.h>

#include <algorithm>
#include <string>
#include <vector>
#include <stdlib.h>

#include "cxutf8232.hpp"

using namespace CXUTF8232;

enum BDDir
{
    BD_LEF,
    BD_RIT,
    BD_TOP,
    BD_BOT,
};

enum BDBorder
{
    BD_NONE,
    BD_DOT,
    BD_THIN,
    BD_THICK,
    BD_TWICE,

    BD_LAST,
};

struct BDCell
{
    int         col;
    int         row;
    int         colSpan;
    int         rowSpan;
    // This is just a localized hack, for now. Really, we need
    // to have all sorts of cool features. For instance, it'd be
    // nice to be able to put a 'title' or a 'footer' on the cell:
    //
    // ┌TITLE──────┐
    // │           │
    // │ KEYC      │
    // │           │
    // └F─O─O─T─E─R┘
    //
    // Or, even just to be able to control valign/halign of the text,
    // including the justification of the text.
    char       *cellData;
};

struct BDLayoutInfo
{
    int         numCols;
    int         numRows;
    int         numCells;
    int         maxTotalWidth;  // We allow the last column to 'fill out' the table.
    int         borderCollapse; // margins must be 0, border must be on.
    int         border[4];
    int         padding[4];
    int         margin[4];
    int        *colSizes;       // These are strict sizes.
    int        *rowSizes;       // These are *minimum* heights;
    BDCell     *cells;
};

char* bdDrawTable(BDLayoutInfo* info);

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

    int colSizes[]      = { 8, -1 };
    int rowSizes[]      = { 1, 1, 1, 1, };
    BDCell cells[]      =
    {
        BDCell{ .col = 0, .row = 0, .colSpan = 2, .rowSpan = 1, .cellData = (char*)"THE TITLE" },
        BDCell{ .col = 0, .row = 1, .colSpan = 1, .rowSpan = 1, .cellData = (char*)"KEYA" },
        BDCell{ .col = 0, .row = 2, .colSpan = 1, .rowSpan = 1, .cellData = (char*)"KEYB" },
        BDCell{ .col = 0, .row = 3, .colSpan = 1, .rowSpan = 1, .cellData = (char*)"KEYC" },
        BDCell{ .col = 1, .row = 1, .colSpan = 1, .rowSpan = 1, .cellData = (char*)"VALA" },
        BDCell{ .col = 1, .row = 2, .colSpan = 1, .rowSpan = 1, .cellData = (char*)"VALB" },
        BDCell{ .col = 1, .row = 3, .colSpan = 1, .rowSpan = 1, .cellData = (char*)"VALC" },
    };

    for (int BC = 0; BC < 2; ++BC)
    {
        BDLayoutInfo bag =
        {
            .numCols        = 2,
            .numRows        = 4,
            .numCells       = 7,
            .maxTotalWidth  = 72,
            .borderCollapse = BC,
            .border         = { BD_THIN, BD_THIN, BD_THIN, BD_THIN, },
            .padding        = { 1, 1, 1, 1, },
            .margin         = { 0, 0, 0, 0, },
            .colSizes       = &colSizes[0],
            .rowSizes       = &rowSizes[0],
            .cells          = &cells[0],
        };

        char* table         = bdDrawTable(&bag);
        fprintf(stdout, "%s\n", table);
        free(table);
    }

    return 0;
}

int bdWallWidth(BDLayoutInfo* info, int col)
{
    int marginDiff      = info->margin[BD_LEF] + info->margin[BD_RIT];
    int paddingDiff     = info->padding[BD_LEF] + info->padding[BD_RIT];
    int borderDiff      = !!info->border[BD_LEF] + !!info->border[BD_RIT];
    if (((col > 0) && info->borderCollapse) && (borderDiff == 2) && (marginDiff == 0))
    {
        borderDiff      -= 1;
    }
    return marginDiff + paddingDiff + borderDiff;
}

int bdCeilWidth(BDLayoutInfo* info, int row)
{
    int marginDiff      = info->margin[BD_TOP] + info->margin[BD_BOT];
    int paddingDiff     = info->padding[BD_TOP] + info->padding[BD_BOT];
    int borderDiff      = !!info->border[BD_TOP] + !!info->border[BD_BOT];
    if (((row > 0) && info->borderCollapse) && (borderDiff == 2) && (marginDiff == 0))
    {
        borderDiff      -= 1;
    }
    return marginDiff + paddingDiff + borderDiff;
}

int bdCellWidth(BDLayoutInfo* info, std::vector<int>& colHisto, int whichCell)
{
    BDCell* cell        = &info->cells[whichCell];
    int cellStart       = colHisto[cell->col];
    int cellStop        = colHisto[cell->col + cell->colSpan];
    int cellWidth       = cellStop - cellStart;
    return cellWidth;
}

int bdCellCanvasWidth(BDLayoutInfo* info, std::vector<int>& colHisto, int whichCell)
{
    int cellWidth       = bdCellWidth(info, colHisto, whichCell);
    return cellWidth - bdWallWidth(info, info->cells[whichCell].col);
}

int bdCellHeight(BDLayoutInfo* info, std::vector<int>& rowHisto, int whichCell)
{
    BDCell* cell        = &info->cells[whichCell];
    int cellStart       = rowHisto[cell->row];
    int cellStop        = rowHisto[cell->row + cell->rowSpan];
    int cellHeight      = cellStop - cellStart;
    return cellHeight;
}

int bdCellCanvasHeight(BDLayoutInfo* info, std::vector<int>& rowHisto, int whichCell)
{
    int cellHeight      = bdCellHeight(info, rowHisto, whichCell);
    return cellHeight - bdCeilWidth(info, info->cells[whichCell].row);
}

int bdMeasureText(BDLayoutInfo* info, char* text)
{
    return (int)strlen(text);
}

struct BDLayout
{
    std::vector<int>    colSizes;
    std::vector<int>    colHisto;
    std::vector<int>    rowSizes;
    std::vector<int>    rowHisto;
};

BDLayout bdLayoutCells(BDLayoutInfo* info)
{
    BDLayout layout             = { };
    std::vector<int>& colSizes  = layout.colSizes;
    std::vector<int>& colHisto  = layout.colHisto;
    std::vector<int>& rowSizes  = layout.rowSizes;
    std::vector<int>& rowHisto  = layout.rowHisto;

    {
        colSizes.resize(info->numCols);
        colHisto.resize(info->numCols + 1);
        for (int CC = 0; CC < info->numCols - 1; ++CC)
        {
            colSizes[CC]            = info->colSizes[CC];
            // Throw the margin, padding, and border in.
            colSizes[CC]            += bdWallWidth(info, CC);
            colHisto[CC+1]          = colHisto[CC] + colSizes[CC];
        }
        if (info->colSizes[info->numCols-1] < 0)
        {
            info->colSizes[info->numCols-1] = info->maxTotalWidth - colHisto[info->numCols-1];
        }
        colHisto[info->numCols]     = info->maxTotalWidth;
    }

    {
        rowSizes.resize(info->numRows);
        rowHisto.resize(info->numRows + 1);

        for (int RR = 0; RR < info->numRows; ++RR)
        {
            // XXX: rowSizes[RR] is the *min* height; need to get
            // the actual height.
            rowSizes[RR]            = info->rowSizes[RR];
            rowSizes[RR]            += bdCeilWidth(info, RR);
        }

        // Crawl through the cells and determine their heights.
        // We round-robin overflow to all the rows.
        for (int EE = 0; EE < info->numCells; ++EE)
        {
            BDCell* cell            = &info->cells[EE];
            int cellWidth           = bdCellCanvasWidth(info, colHisto, EE);
            int tlength             = bdMeasureText(info, cell->cellData);
            int cellLength          = (((int)tlength + cellWidth - 1) / cellWidth);

            // Throw the margin, padding, and border in.
            cellLength              += bdCeilWidth(info, cell->row);

            for (int RR = 0; RR < cell->rowSpan; ++RR)
            {
                cellLength          = std::max(0, cellLength - rowSizes[cell->row + RR]);
            }

            int perRowAtLeast       = cellLength / cell->rowSpan;
            int perRowRemainder     = cellLength - (perRowAtLeast * cell->rowSpan);
            for (int RR = 0; RR < cell->rowSpan; ++RR)
            {
                rowSizes[cell->row + RR]    += perRowAtLeast + !!(RR < perRowRemainder);
            }
        }

        for (int RR = 0; RR < info->numRows; ++RR)
        {
            rowHisto[RR+1]          = rowHisto[RR] + rowSizes[RR];
        }
    }

    return layout;
}

unsigned int bdCardinalPoints(signed long corner)
{
    switch (corner)
    {
    case utf32("╴") : return (1 << BD_LEF)                                                ;
    case utf32("╶") : return                 (1 << BD_RIT)                                ;
    case utf32("╵") : return                                 (1 << BD_TOP)                ;
    case utf32("╷") : return                                                 (1 << BD_BOT);

    case utf32("─") : return (1 << BD_LEF) | (1 << BD_RIT)                                ;
    case utf32("│") : return                                 (1 << BD_TOP) | (1 << BD_BOT);

    case utf32("┌") : return                 (1 << BD_RIT)                 | (1 << BD_BOT);
    case utf32("┐") : return (1 << BD_LEF)                                 | (1 << BD_BOT);
    case utf32("└") : return                 (1 << BD_RIT) | (1 << BD_TOP)                ;
    case utf32("┘") : return (1 << BD_LEF)                 | (1 << BD_TOP)                ;

    case utf32("┬") : return (1 << BD_LEF) | (1 << BD_RIT)                 | (1 << BD_BOT);
    case utf32("├") : return                 (1 << BD_RIT) | (1 << BD_TOP) | (1 << BD_BOT);
    case utf32("┤") : return (1 << BD_LEF)                 | (1 << BD_TOP) | (1 << BD_BOT);
    case utf32("┴") : return (1 << BD_LEF) | (1 << BD_RIT) | (1 << BD_TOP)                ;

    case utf32("┼") : return (1 << BD_LEF) | (1 << BD_RIT) | (1 << BD_TOP) | (1 << BD_BOT);

    default         : return 0;
    }
}

signed long bdPointsCardinal(unsigned int rose)
{
    switch (rose)
    {
    case (1 << BD_LEF)                                                 : return utf32("╴");
    case                 (1 << BD_RIT)                                 : return utf32("╶");
    case                                 (1 << BD_TOP)                 : return utf32("╵");
    case                                                 (1 << BD_BOT) : return utf32("╷");

    case (1 << BD_LEF) | (1 << BD_RIT)                                 : return utf32("─");
    case                                 (1 << BD_TOP) | (1 << BD_BOT) : return utf32("│");

    case                 (1 << BD_RIT)                 | (1 << BD_BOT) : return utf32("┌");
    case (1 << BD_LEF)                                 | (1 << BD_BOT) : return utf32("┐");
    case                 (1 << BD_RIT) | (1 << BD_TOP)                 : return utf32("└");
    case (1 << BD_LEF)                 | (1 << BD_TOP)                 : return utf32("┘");

    case (1 << BD_LEF) | (1 << BD_RIT)                 | (1 << BD_BOT) : return utf32("┬");
    case                 (1 << BD_RIT) | (1 << BD_TOP) | (1 << BD_BOT) : return utf32("├");
    case (1 << BD_LEF)                 | (1 << BD_TOP) | (1 << BD_BOT) : return utf32("┤");
    case (1 << BD_LEF) | (1 << BD_RIT) | (1 << BD_TOP)                 : return utf32("┴");

    case (1 << BD_LEF) | (1 << BD_RIT) | (1 << BD_TOP) | (1 << BD_BOT) : return utf32("┼");

    default                                                            : return 0;
    }
}

signed long bdCornerUpdate(signed long oldCorner, signed long updCorner)
{
    return bdPointsCardinal(bdCardinalPoints(oldCorner) | bdCardinalPoints(updCorner));
}

char* bdRenderTable(BDLayoutInfo* info, BDLayout& layout)
{
    int rowStride               = layout.colHisto.back();

    std::vector<signed long> rowExample;
    //rowExample.resize(rowStride, utf32("░"));
    rowExample.resize(rowStride, utf32(" "));
    std::vector<std::vector<signed long>> canvas{(std::size_t)layout.rowHisto.back(), rowExample};

    for (int EE = 0; EE < info->numCells; ++EE)
    {
        // We're going to draw the border, to begin with.
        BDCell* cell            = &info->cells[EE];
        int top                 = layout.rowHisto[cell->row];
        int bot                 = layout.rowHisto[cell->row + cell->rowSpan] - 1;
        int lef                 = layout.colHisto[cell->col];
        int rit                 = layout.colHisto[cell->col + cell->colSpan] - 1;

        int btop                = top + info->margin[BD_TOP];
        int bbot                = bot - info->margin[BD_BOT];
        int blef                = lef + info->margin[BD_LEF];
        int brit                = rit - info->margin[BD_RIT];

        // Render borders.
        int hasBorders          = (!!info->border[0]) + (!!info->border[1]) + (!!info->border[2]) + (!!info->border[3]);
        int noMargin            = !(info->margin[BD_TOP] + info->margin[BD_BOT] + info->margin[BD_LEF] + info->margin[BD_RIT]);

        int borderCollapse      = (hasBorders == 4) && noMargin && info->borderCollapse;

        if ((cell->row > 0) && borderCollapse)
        {
            btop                -= 1;
        }

        if ((cell->col > 0) && borderCollapse)
        {
            blef                -= 1;
        }

        if (hasBorders)
        {
            for (int CC = blef; CC <= brit; ++CC)
            {
                if (info->border[BD_TOP])
                {
                    canvas[btop][CC]    = bdCornerUpdate(canvas[btop][CC], (CC == blef) ? utf32("╶") : ((CC == brit) ? utf32("╴") : utf32("─")));
                }
                if (info->border[BD_BOT])
                {
                    canvas[bbot][CC]    = bdCornerUpdate(canvas[bbot][CC], (CC == blef) ? utf32("╶") : ((CC == brit) ? utf32("╴") : utf32("─")));
                }
            }

            for (int RR = btop; RR <= bbot; ++RR)
            {
                if (info->border[BD_LEF])
                {
                    canvas[RR][blef]    = bdCornerUpdate(canvas[RR][blef], (RR == btop) ? utf32("╷") : ((RR == bbot) ? utf32("╵") : utf32("│")));
                }
                if (info->border[BD_RIT])
                {
                    canvas[RR][brit]    = bdCornerUpdate(canvas[RR][brit], (RR == btop) ? utf32("╷") : ((RR == bbot) ? utf32("╵") : utf32("│")));
                }
            }
        }

        // Render content.
        int ctop                = top + (info->padding[BD_TOP] + !!info->border[BD_TOP]);
        int cbot                = bot - (info->padding[BD_BOT] + !!info->border[BD_BOT]);
        int clef                = lef + (info->padding[BD_LEF] + !!info->border[BD_LEF]);
        int crit                = rit - (info->padding[BD_RIT] + !!info->border[BD_RIT]);

        int tlength             = bdMeasureText(info, cell->cellData);

        // Considering that I have a correct implementation of Knuth-Plass hanging
        // around somewhere, this is shameful stuff.
        for (int RR = cbot, TT = 0; (RR <= ctop) && (TT < tlength); ++RR)
        {
            for (int CC = clef; (CC <= crit) && (TT < tlength); ++CC, ++TT)
            {
                canvas[RR][CC]  = (signed long)cell->cellData[TT];
            }
        }
    }

    std::string stitched    = { };
    for (auto& row : canvas)
    {
        for (auto& col : row)
        {
            stitched    += utf8(col).str;
        }
        stitched        += "\n";
    }

    char* buffer        = (char*)calloc(stitched.size(), 1);
    memcpy(&buffer[0], stitched.c_str(), stitched.size());

    return buffer;
}

char* bdDrawTable(BDLayoutInfo* info)
{
    BDLayout layout = bdLayoutCells(info);

    char* buffer    = bdRenderTable(info, layout);

    return buffer;
}