#include "sheet.h"

#include "cell.h"
#include "common.h"

#include <algorithm>
#include <functional>
#include <iostream>
#include <optional>
#include <variant>

using namespace std::literals;

Sheet::~Sheet() {}

void Sheet::SetCell(Position pos, const std::string& text) {
    if (!pos.IsValid()) {
        throw InvalidPositionException("Cell position is not valid");
    }
    if (pos_cell_.count(pos) == 0) {
        pos_cell_[pos] = std::make_unique<Cell>(*this);
    }

    occupied_cells_.insert(pos);
    pos_cell_.at(pos)->Set(std::move(text));

    if (size_.cols <= pos.col) {
        size_.cols = pos.col + 1;
    }
    if (size_.rows <= pos.row) {
        size_.rows = pos.row + 1;
    }
}

const CellInterface* Sheet::GetCell(Position pos) const {
    if (!pos.IsValid()) {
        throw InvalidPositionException("Cell position is not valid");
    }
    if (pos_cell_.count(pos) > 0 && occupied_cells_.count(pos) > 0) {
        return pos_cell_.at(pos).get();
    }
    else {
        return nullptr;
    }
}

CellInterface* Sheet::GetCell(Position pos) {
    if (!pos.IsValid()) {
        throw InvalidPositionException("Cell position is not valid");
    }
    if (pos_cell_.count(pos) > 0 && occupied_cells_.count(pos) > 0) {
        return pos_cell_.at(pos).get();
    }
    else {
        return nullptr;
    }
}

void Sheet::ClearCell(Position pos) {
    if (!pos.IsValid()) {
        throw InvalidPositionException("Cell position is not valid");
    }
    if (pos_cell_.count(pos) > 0) {
        pos_cell_.at(pos)->Set("");
    }
    occupied_cells_.erase(pos);

    if (occupied_cells_.empty()) {
        size_.cols = 0;
        size_.rows = 0;
    }
    else {
        int max_col = 0;
        int max_row = 0;
        for (const auto& el : occupied_cells_) {
            if (el.row > max_row) {
                max_row = el.row;
            }
            else {
                continue;
            }
            if (el.col > max_col) {
                max_col = el.col;
            }
            else {
                continue;
            }
        }
        size_.cols = max_col + 1;
        size_.rows = max_row + 1;
    }
    }

Size Sheet::GetPrintableSize() const {
    return size_;
}

void Sheet::PrintValues(std::ostream& output) const {
    for (int i = 0; i != size_.rows; ++i) {
        for (int j = 0; j != size_.cols; ++j) {
            Position pos = { i, j };
            if (pos_cell_.count(pos) != 0) {
                auto res = pos_cell_.at(pos)->GetValue();
                if (std::holds_alternative<double>(res)) {
                    output << std::get<double>(res);
                }
                else if (std::holds_alternative<FormulaError>(res)) {
                    output << std::get<FormulaError>(res);
                }
                else {
                    output << std::get<std::string>(res);
                }
            }
            else {
                output << "";
            }
            if (j < size_.cols - 1) {
                output << '\t';
            }
        }
        output << '\n';
    }
}

void Sheet::PrintTexts(std::ostream& output) const {
    for (int i = 0; i != size_.rows; ++i) {
        for (int j = 0; j != size_.cols; ++j) {
            Position pos = { i, j };
            if (pos_cell_.count(pos) != 0) {
                output << pos_cell_.at(pos)->GetText();
            }
            else {
                output << "";
            }
            if (j < size_.cols - 1) {
                output << '\t';
            }
        }
        output << '\n';
    }
}

std::unique_ptr<SheetInterface> CreateSheet() {
    return std::make_unique<Sheet>();
}
