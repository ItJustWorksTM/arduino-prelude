/*
 *  ardpp.cpp
 *  Copyright 2021 ItJustWorksTM
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 */

#include <array>
#include <cstdlib>
#include <iostream>
#include <optional>
#include <string>
#include <unordered_map>
#include <clang-c/CXString.h>
#include <clang-c/Index.h>

int main(int argc, char** argv) {
    std::ios::sync_with_stdio(false);
    if (argc != 2) {
        std::cerr << "error: no input file.";
        return EXIT_FAILURE;
    }

    auto index = ::clang_createIndex(0, 0);

    const std::array<const char*, 3> cmd = {"-x", "c++", "-std=gnu++20"};
    auto unit = ::clang_parseTranslationUnit(index, argv[1], cmd.data(), cmd.size(), nullptr, 0,
                                             ::CXTranslationUnit_SingleFileParse);
    if (unit == nullptr) {
        std::cerr << "error: unable to parse translation unit." << std::endl;
        return EXIT_FAILURE;
    }

#if USE_DIAGNOSTICS
    bool has_error = false;
    ::CXDiagnosticSet diags = ::clang_getDiagnosticSetFromTU(unit);
    const unsigned diags_count = ::clang_getNumDiagnosticsInSet(diags);
    for (unsigned i = 0; i < diags_count; ++i) {
        auto diag = ::clang_getDiagnosticInSet(diags, i);
        const auto severity = clang_getDiagnosticSeverity(diag);
        if (severity >= ::CXDiagnostic_Error) {
            has_error = true;
            auto diag_fmt = ::clang_formatDiagnostic(diag, ::clang_defaultDiagnosticDisplayOptions());
            std::cerr << ::clang_getCString(diag_fmt) << std::endl;
            ::clang_disposeString(diag_fmt);
        }

        ::clang_disposeDiagnostic(diag);
    }
    ::clang_disposeDiagnosticSet(diags);

    if (has_error) {
        ::clang_disposeTranslationUnit(unit);
        ::clang_disposeIndex(index);
        return EXIT_FAILURE;
    }
#endif

    struct {
        std::unordered_map<std::string, std::optional<std::string>> elements;
        std::string curr_decl;
        ::CXSourceLocation curr_sloc;
        ::CXPrintingPolicy ppp;
    } state;

    auto cursor = ::clang_getTranslationUnitCursor(unit);

    state.ppp = ::clang_getCursorPrintingPolicy(cursor);
    ::clang_PrintingPolicy_setProperty(state.ppp, ::CXPrintingPolicy_TerseOutput, true);

    ::clang_visitChildren(
        cursor,
        [](::CXCursor c, ::CXCursor, ::CXClientData client_data) {
            auto& curr_state = *static_cast<decltype(state)*>(client_data);
            const auto kind = ::clang_getCursorKind(c);
            if (!curr_state.curr_decl.empty()) {
                switch (kind) {
                case ::CXCursor_CompoundStmt: {
                    CXString filename;
                    unsigned line;
                    ::clang_getPresumedLocation(curr_state.curr_sloc, &filename, &line, nullptr);
                    curr_state.elements.emplace(std::move(curr_state.curr_decl),
                                                std::to_string(line) + " \"" + clang_getCString(filename) + '\"');
                    curr_state.curr_sloc = ::clang_getNullLocation();
                    clang_disposeString(filename);
                    return ::CXChildVisit_Continue;
                }
                case ::CXCursor_ParmDecl:
                case ::CXCursor_TemplateTypeParameter:
                case ::CXCursor_NonTypeTemplateParameter:
                case ::CXCursor_TemplateTemplateParameter:
                    break;
                default:
                    if (clang_isDeclaration(kind)) {
                        curr_state.elements.insert_or_assign(std::move(curr_state.curr_decl), std::nullopt);
                        curr_state.curr_sloc = ::clang_getNullLocation();
                    }
                }
            }
            switch (kind) {
            case ::CXCursor_FunctionDecl:
            case ::CXCursor_FunctionTemplate: {
                auto decl = ::clang_getCursorPrettyPrinted(c, curr_state.ppp);

                std::string decl_str = ::clang_getCString(decl);
                if (const auto it = curr_state.elements.find(decl_str);
                    it == curr_state.elements.cend() || it->second.has_value()) {
                    curr_state.curr_decl = std::move(decl_str);
                    curr_state.curr_sloc = ::clang_getCursorLocation(c);
                }

                ::clang_disposeString(decl);
            }
                [[fallthrough]];
            case ::CXCursor_TemplateTypeParameter:
            case ::CXCursor_NonTypeTemplateParameter:
            case ::CXCursor_TemplateTemplateParameter:
                return ::CXChildVisit_Recurse;
            default:
                return ::CXChildVisit_Continue;
            }
        },
        &state);

    ::clang_PrintingPolicy_dispose(state.ppp);
    ::clang_disposeTranslationUnit(unit);
    ::clang_disposeIndex(index);

    std::cout << "#include <Arduino.h>\n#line 1 \"" << argv[1] << "\"\n";

    for (const auto& [decl, def_location] : state.elements) {
        if (def_location->empty())
            continue;
        std::cout << "#line " << *def_location << '\n' << decl << ";\n";
    }
}
