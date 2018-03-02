#ifndef __CELL_H
#define __CELL_H

#include "number.h"

typedef enum tagEtype {
    None,
    Address,
    Number,
    String,
} CellType;

typedef struct tagTCell {
    BID_UINT128 number;
    struct tagTCell *address;
    char *string;
    CellType type;
} Cell;

Cell *cell_newCell();
void cell_freeCell(Cell *c);

Cell *cell_fromNumber(BID_UINT128 n);
Cell *cell_fromString(const char *s);
Cell *cell_fromCell(const Cell *c);
Cell *cell_fromAddress(Cell *c);
void cell_copyCell(Cell *dest, const Cell *source);
void  cell_resetCell(Cell *c);

#endif