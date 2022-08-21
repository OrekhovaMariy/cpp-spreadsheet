#pragma once

#include "common.h"
#include "formula.h"

#include <optional>
#include <unordered_set>

class Sheet;

class Cell final : public CellInterface {

public:
    Cell(Sheet& sheet);
    ~Cell();

    void Set(const std::string& text);
    void Clear();

    Value GetValue() const override;
    std::string GetText() const override;

    std::vector<Position> GetReferencedCells() const override;

    void FindCyclicDependencies(std::vector<Position> cells);
    void ClearCache();
    void UpdateCache();

    void SetDependentCell(Cell* cell);

private:
    class Impl;
    class EmptyImpl;
    class TextImpl;
    class FormulaImpl;
    std::unique_ptr<Impl> impl_;
    Sheet& sheet_;
    std::unordered_set<CellInterface*> dependent_cells_;
    std::unordered_set<CellInterface*> cache_cells_;
};

class Cell::Impl {
public:
    Impl() = default;
    virtual ~Impl() = default;
    virtual CellInterface::Value GetValue() const = 0;
    virtual std::string GetText() const = 0;
    virtual std::vector<Position> GetReferencedCells() const;
    virtual void ClearCache();
};

class Cell::EmptyImpl final : public Impl {
public:
    CellInterface::Value GetValue() const override;
    std::string GetText() const override;
};

class Cell::TextImpl final : public Impl {
public:
    explicit TextImpl(std::string text);
    CellInterface::Value GetValue() const override;
    std::string GetText() const override;
private:
    std::string text_;
};

class Cell::FormulaImpl final : public Impl {
public:
    explicit FormulaImpl(std::string formula, const SheetInterface& sheet);
    CellInterface::Value GetValue() const override;
    std::string GetText() const override;
    std::vector<Position> GetReferencedCells() const override;
    void ClearCache() override;
private:
    std::unique_ptr<FormulaInterface> formula_;
    mutable std::optional<FormulaInterface::Value> cache_;
    const SheetInterface& sheet_;
};
