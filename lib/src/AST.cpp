//
// Created by rob on 7/14/2020.
//

#include "hackasm/AST.h"

#include "hackasm/Instructions/Symbol.h"
#include "hackasm/SymbolTable.h"

#include <utility>
#include <iostream>
#include <variant>
#include <ranges>
#include <algorithm>

#include <ctre.hpp>

using namespace std::views;

namespace hackasm {

    //TODO: clean up somehow
    Instruction parse(const AsmLine &s) {
        if (A_Type::identify(s)) {
            return A_Type(s);
        }
        if (L_Type::identify(s)) {
            return L_Type(s);
        }
        if (C_Type::identify(s)) {
            return C_Type(s);
        }

        throw std::runtime_error("Cannot parse assembly line: " + s.inst);
    }

    AST::AST(AsmFile f) : assembly(std::move(f)) {
        //Parse assembly source to instructions
        auto insts = assembly.instructions | transform([](AsmLine &a) { return parse(a); });
        std::ranges::copy(insts, std::back_inserter(listing));

        auto labels = listing |
                      //select only L-Type instructions
                      filter([](const Instruction &i) { return std::holds_alternative<L_Type>(i); }) |
                      //convert Instruction to L-Type
                      transform([](const Instruction &i) { return std::get<L_Type>(i); }) |
                      //extract symbol from L-Type
                      transform([](const L_Type &l) { return l.s; });
        //Insert all found labels into the symbol table.
        //This is a special operation.  Labels must be known to not get confused with symbols in the second pass.
        for (const Symbol &s : labels) {
            symbols.insert_label(s);
        }
        //TODO: change to reifying symbols instead.  Extract symbols in range op
        //Second pass: reify all instructions
        for (auto &i : listing) {
            symbols.reify(i);
        }

    }

    std::ostream &operator<<(std::ostream &os, const AST &obj) {
        os << "AST:\n";
        for (const auto &i : obj.listing) {
            std::visit([&](auto e) { os << e << '\n'; }, i);
        }
        os << "Symbol Table:\n";
        os << obj.symbols;
        return os;
    }

    std::vector<std::string> AST::to_binary() const {
        std::vector<std::string> output{};
        for (const auto &i : listing) {
            std::visit([&](auto inst) {
                std::string bin = inst.to_binary_format();
                if (bin.size() > 0) {
                    output.push_back(inst.to_binary_format());
                }
            }, i);
        }
        return output;
    }

    const std::vector<Instruction> &AST::get_listing() const {
        return std::as_const(listing);
    }

    const SymbolTable &AST::get_symbol_table() const {
        return std::as_const(symbols);
    }

    const AsmFile &AST::get_asmfile() const {
        return std::as_const(assembly);
    }
}
