#include "cell.h"
#include "sheet.h"

#include <cassert>
#include <iostream>
#include <string>
#include <optional>
#include <stack>
#include <algorithm>

using namespace std::literals;

Cell::Cell(Sheet& sheet)
	: impl_(std::make_unique<EmptyImpl>())
	, sheet_(sheet)
{}

Cell::~Cell() {}

void Cell::Set(const std::string& text) {

	if (text.size() > 1 && text[0] == FORMULA_SIGN) {
		std::unique_ptr<Impl> impl;
		impl = std::make_unique<FormulaImpl>(std::move(text.substr(1)), sheet_);

		FindCyclicDependencies(impl->GetReferencedCells());
		impl_ = std::move(impl);
	}
	else if (text.empty()) {
		impl_ = std::make_unique<EmptyImpl>();
	}
	else {
		impl_ = std::make_unique<TextImpl>(text);
	}

	for (auto cell : dependent_cells_) {
		dynamic_cast<Cell*>(cell)->cache_cells_.erase(this);
	}
	dependent_cells_.clear();

	for (auto& cell : impl_->GetReferencedCells()) {
		auto c = sheet_.GetCell(cell);
		if (c == nullptr) {
			sheet_.SetCell(cell, {});
			c = sheet_.GetCell(cell);
		}

		dependent_cells_.insert(c);
		dynamic_cast<Cell*>(c)->SetDependentCell(this);
	}
	UpdateCache();
}

void Cell::SetDependentCell(Cell* cell) {
	cache_cells_.insert(cell);
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

void Cell::ClearCache() {
	return impl_->ClearCache();
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

void Cell::UpdateCache() {
	for (auto& c : cache_cells_) {
		dynamic_cast<Cell*>(c)->ClearCache();
		dynamic_cast<Cell*>(c)->UpdateCache();
	}
}

std::vector<Position> Cell::Impl::GetReferencedCells() const {
	return {};
}

void Cell::Impl::ClearCache() {}

CellInterface::Value Cell::EmptyImpl::GetValue() const {
	return {};
}

std::string Cell::EmptyImpl::GetText() const {
	return {};
}

Cell::TextImpl::TextImpl(std::string text) : text_(text) {}

CellInterface::Value Cell::TextImpl::GetValue() const {
	if (text_.size() > 0 && text_[0] == ESCAPE_SIGN) {
		return text_.substr(1);
	}
	else {
		return text_;
	}
}

std::string Cell::TextImpl::GetText() const {
	return text_;
}

Cell::FormulaImpl::FormulaImpl(std::string formula, const SheetInterface& sheet)
	: formula_(ParseFormula(formula))
	, sheet_(sheet)
{}

CellInterface::Value Cell::FormulaImpl::GetValue() const {
	if (!cache_) {
		cache_ = formula_->Evaluate(sheet_);
	}
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

void Cell::FormulaImpl::ClearCache() {
	cache_.reset();
}
