#ifndef MEGAAWESOME_BOX_DRAWING_H
#define MEGAAWESOME_BOX_DRAWING_H

#ifdef  __cplusplus
extern "C"
{
#endif//__cplusplus

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

typedef struct BDCellData BDCellData;

typedef int           (*bdfptrMeasureText)  (BDCellData* text, int cellWidth);
typedef signed long   (*bdfptrGetTextEntry) (BDCellData* cellData, int cellWidth, int row, int col);

struct BDCellData
{
    void                   *userData;
};

struct BDCell
{
    int         col;
    int         row;
    int         colSpan;
    int         rowSpan;
    BDCellData  cellData;
};

struct BDLayoutInfo
{
    int                     numCols;
    int                     numRows;
    int                     numCells;
    int                     maxTotalWidth;  // We allow the last column to 'fill out' the table.
    int                     borderCollapse; // margins must be 0, border must be on.
    int                     border[4];
    int                     padding[4];
    int                     margin[4];
    int                    *colSizes;       // These are strict sizes.
    int                    *rowSizes;       // These are *minimum* heights;
    BDCell                 *cells;
    bdfptrMeasureText       measureText;
    bdfptrGetTextEntry      getTextEntry;
};

char* bdDrawTable(BDLayoutInfo* info);

struct BDCStr
{
    const char     *begin;
    const char     *end;
};

int bdMeasureText(BDCellData* text, int cellWidth);
signed long bdGetTextEntry(BDCellData* cellData, int cellWidth, int row, int col);

struct BDASCIISizedCell
{
    int         col;
    int         row;
    int         colSpan;
    int         rowSpan;
    int         minWidth;
    int         minHeight;
    const char* text;
};

char* bdDrawASCIITable(int numCells, BDASCIISizedCell* cells, int maxTotalWidth);

#ifdef  __cplusplus
}//end extern "C".
#endif//__cplusplus

#endif//MEGAAWESOME_BOX_DRAWING_H

#ifdef  MEGAAWESOME_BOX_DRAWING_CPP

#include <algorithm>
#include <string>
#include <vector>
#include <stdlib.h>

#include "cxutf8232.hpp"

using namespace CXUTF8232;

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
int bdMeasureText(BDCellData* cellData, int cellWidth)
{
    BDCStr* text    = (BDCStr*)cellData->userData;
    int tlength     = (int)strlen(text->begin);
    text->end       = text->begin + tlength;
    int numRows     = (((int)tlength + cellWidth - 1) / cellWidth);
    return numRows;
}

signed long bdGetTextEntry(BDCellData* cellData, int cellWidth, int row, int col)
{
    BDCStr* text    = (BDCStr*)cellData->userData;
    if ((cellWidth * row + col) < (text->end - text->begin))
    {
        return (signed long)(text->begin[cellWidth * row + col]);
    }
    else
    {
        return utf32(" ");
    }
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
            int cellLength          = info->measureText(&cell->cellData, cellWidth);

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
        int ctop                = btop + (info->padding[BD_TOP] + !!info->border[BD_TOP]);
        int cbot                = bbot - (info->padding[BD_BOT] + !!info->border[BD_BOT]);
        int clef                = blef + (info->padding[BD_LEF] + !!info->border[BD_LEF]);
        int crit                = brit - (info->padding[BD_RIT] + !!info->border[BD_RIT]);

        // Considering that I have a correct implementation of Knuth-Plass hanging
        // around somewhere, this is shameful stuff.
        for (int RR = 0; RR <= (cbot - ctop); ++RR)
        {
            for (int CC = 0; CC <= (crit - clef); ++CC)
            {
                canvas[cbot + RR][clef + CC] = info->getTextEntry(&cell->cellData, crit - clef + 1, RR, CC);
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
    if (!info->numRows)
    {
        return 0;
    }
    if (!info->numCols)
    {
        return 0;
    }
    if (!info->numCells)
    {
        return 0;
    }
    if (info->maxTotalWidth < 3)
    {
        return 0;
    }
    if (!info->rowSizes)
    {
        return 0;
    }
    if (!info->colSizes)
    {
        return 0;
    }
    if (!info->cells)
    {
        return 0;
    }
    if (!info->measureText)
    {
        return 0;
    }
    if (!info->getTextEntry)
    {
        return 0;
    }

    BDLayout layout = bdLayoutCells(info);

    char* buffer    = bdRenderTable(info, layout);

    return buffer;
}

char* bdDrawASCIITable(int numCells, BDASCIISizedCell* cells, int maxTotalWidth)
{
    BDCell *bdcells         = (BDCell*)calloc(sizeof(BDCell), numCells);
    BDCStr* cStrs           = (BDCStr*)calloc(sizeof(BDCStr), numCells);

    char* buffer            = nullptr;

    std::vector<int> colSizes;
    std::vector<int> rowSizes;

    for (int EE = 0; EE < numCells; ++EE)
    {
        cStrs[EE].begin         = cells[EE].text;
        cStrs[EE].end           = cells[EE].text + strlen(cells[EE].text);
        bdcells[EE].col         = cells[EE].col;
        bdcells[EE].row         = cells[EE].row;
        bdcells[EE].colSpan     = std::max(1, cells[EE].colSpan);
        bdcells[EE].rowSpan     = std::max(1, cells[EE].rowSpan);
        bdcells[EE].cellData.userData = &cStrs[EE];

        colSizes.resize(std::max<std::size_t>(colSizes.size(), bdcells[EE].col + bdcells[EE].colSpan));
        rowSizes.resize(std::max<std::size_t>(rowSizes.size(), bdcells[EE].row + bdcells[EE].rowSpan));

        if (bdcells[EE].colSpan == 1)
        {
            colSizes[bdcells[EE].col]   = std::max(colSizes[bdcells[EE].col], cells[EE].minWidth);
        }

        if (bdcells[EE].rowSpan == 1)
        {
            rowSizes[bdcells[EE].row]   = cells[EE].minHeight;
        }
    }

    BDLayoutInfo bag            =
    {
        .numCols                = (int)colSizes.size(),
        .numRows                = (int)rowSizes.size(),
        .numCells               = numCells,
        .maxTotalWidth          = maxTotalWidth,
        .borderCollapse         = 1,
        .border                 = { BD_THIN, BD_THIN, BD_THIN, BD_THIN },
        .padding                = { 0, 0, 0, 0, },
        .margin                 = { 0, 0, 0, 0, },
        .colSizes               = &colSizes[0],
        .rowSizes               = &rowSizes[0],
        .cells                  = bdcells,
        .measureText            = &bdMeasureText,
        .getTextEntry           = &bdGetTextEntry,
    };

    buffer  = bdDrawTable(&bag);

    free(bdcells);
    free(cStrs);
    return buffer;
}

#endif//MEGAAWESOME_BOX_DRAWING_CPP
