#include "cell.h"
#include "sheet.h"

#include <cassert>
#include <iostream>
#include <string>
#include <optional>
#include <stack>

using namespace std::literals;

Cell::Cell(Sheet& sheet)
	: sheet_(sheet)
{
	impl_ = std::make_unique<EmptyImpl>();
}

Cell::~Cell() = default;

void Cell::Set(const std::string& text) {
	if (text.size() > 1 && text[0] == FORMULA_SIGN) {
		std::unique_ptr<Impl> impl;
		impl = std::make_unique<FormulaImpl>(std::move(text), sheet_);

		FindCyclicDependencies(impl->GetReferencedCells());
		impl_ = std::move(impl);
	}
	else if (text.empty()) {
		impl_ = std::make_unique<EmptyImpl>();
	}
	else {
		impl_ = std::make_unique<TextImpl>(text);
	}

	for (auto cell : all_dependent_cells_) {
		cell->all_dependent_cells_.erase(this);
	}

	for (auto& cell : impl_->GetReferencedCells()) {
		auto c = sheet_.GetCell(cell);
		if (c == nullptr) {
			sheet_.SetCell(cell, "");
			c = sheet_.GetCell(cell);
		}
		dynamic_cast<Cell*>(c)->SetDependentCell(this);
	}
}

void Cell::SetDependentCell(Cell* cell) {
	all_dependent_cells_.insert(dynamic_cast<Cell*>(cell));
}

void Cell::Clear() {
	Set("");
}

Cell::Value Cell::GetValue() const {
	return impl_->GetValue();
}

std::string Cell::GetText() const {
	return impl_->GetText();
}

std::vector<Position> Cell::GetReferencedCells() const {
	return impl_->GetReferencedCells();
}

void Cell::FindCyclicDependencies(std::vector<Position> cells) {
	for (auto cell : cells) {
		CellInterface* c = sheet_.GetCell(cell);
		if (c == this) {
			throw CircularDependencyException("");
		}
		if (c != nullptr) {
			FindCyclicDependencies(sheet_.GetCell(cell)->GetReferencedCells());
		}
	}
}

CellInterface::Value Cell::EmptyImpl::GetValue() const {
	return {};
}

std::string Cell::EmptyImpl::GetText() const {
	return {};
}

std::vector<Position> Cell::EmptyImpl::GetReferencedCells() const {
	return {};
}

Cell::TextImpl::TextImpl(std::string text) : text_(text) {
}

CellInterface::Value Cell::TextImpl::GetValue() const {
	if (text_.size() > 0 && text_.front() == ESCAPE_SIGN) {
		return text_.substr(1);
	}
	else {
		return text_;
	}
}

std::string Cell::TextImpl::GetText() const {
	return text_;
}

std::vector<Position> Cell::TextImpl::GetReferencedCells() const {
	return {};
}

Cell::FormulaImpl::FormulaImpl(std::string text_formula, const SheetInterface& sheet)
	: sheet_(sheet)
{
	formula_ = ParseFormula(text_formula.substr(1));
	cache_ = formula_->Evaluate(sheet_);
}

CellInterface::Value Cell::FormulaImpl::GetValue() const {
	if (std::holds_alternative<double>(cache_.value())) {
		return std::get<double>(cache_.value());
	}
	else {
		return std::get<FormulaError>(cache_.value());
	}
}

std::string Cell::FormulaImpl::GetText() const {
	return FORMULA_SIGN + formula_->GetExpression();
}

std::vector<Position> Cell::FormulaImpl::GetReferencedCells() const {
	return formula_->GetReferencedCells();
}
