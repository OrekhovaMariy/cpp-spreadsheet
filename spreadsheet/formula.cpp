#include "formula.h"

#include "FormulaAST.h"

#include <algorithm>
#include <cassert>
#include <cctype>
#include <sstream>

using namespace std::literals;

std::ostream& operator<<(std::ostream& output, FormulaError fe) {
    return output << fe.ToString();
}

FormulaError::FormulaError(Category category)
    : category_(category) {}

FormulaError::Category FormulaError::GetCategory() const {
    return category_;
}

bool FormulaError::operator==(FormulaError rhs) const {
    return category_ == rhs.category_;
}

std::string_view FormulaError::ToString() const {
    if (category_ == Category::Ref) {
        return "#REF!";
    }
    if (category_ == Category::Value) {
        return "#VALUE!";
    }
    if (category_ == Category::Div0) {
        return "#DIV/0!";
    }
    return {};
}

namespace {
    class Formula : public FormulaInterface {
    public:
        explicit Formula(std::string expression)
            try : ast_(ParseFormulaAST(expression))
        {}
            catch (std::exception& exc) {
                throw FormulaException(exc.what());
            }
        
    Value Evaluate(const SheetInterface& sheet) const override {
        try {
            return ast_.Execute(sheet);
        }
        catch (FormulaError& exc) {
            return exc;
        }
    }

    // Возвращает выражение, которое описывает формулу.
    // Не содержит пробелов и лишних скобок.
    std::string GetExpression() const override {
        std::stringstream out;
        ast_.PrintFormula(out);
        return out.str();
    }

    std::vector<Position> GetReferencedCells() const override {
        std::vector<Position> cells;
        for (Position pos : ast_.GetCells()) {
            if (pos.IsValid()) {
                cells.emplace_back(std::move(pos));
            }
        }      
        cells.resize(std::unique(cells.begin(), cells.end()) - cells.begin());
        std::sort(cells.begin(), cells.end());
        return cells;
    }
    private:
        FormulaAST ast_;
    };

} //namespace

std::unique_ptr<FormulaInterface> ParseFormula(std::string expression) {
    return std::make_unique<Formula>(std::move(expression));
}
