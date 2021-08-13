/*
 *  arduino-prelude.cpp
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

#ifndef ARDUINO_PRELUDE_VERSION
#    define ARDUINO_PRELUDE_VERSION "00.custom"
#endif

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <numeric>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <system_error>
#include <unordered_map>
#include <vector>
#include <clang-c/CXString.h>
#include <clang-c/Index.h>

using namespace std::literals;

namespace stdfs = std::filesystem;

bool is_sketch_extension(const stdfs::path& ext) { return ext == ".ino" || ext == ".pde"; }

int main(int argc, char** argv) {
    std::ios::sync_with_stdio(false);
    if (argc < 2) {
        std::cerr << "error: no input file.\n";
        return EXIT_FAILURE;
    }

    if (argc == 2 && argv[1] == "--version"sv) {
        std::cout << "arduino-prelude v" ARDUINO_PRELUDE_VERSION "\n";
        return EXIT_SUCCESS;
    }

    struct {
        std::vector<std::string> files;
        std::unordered_map<std::string, std::optional<std::string>> functions;
        std::string curr_decl;
        ::CXSourceLocation curr_sloc;
        ::CXPrintingPolicy ppp;
    } state;

    stdfs::path input_path = argv[1];
    if (std::error_code ec; !stdfs::exists(input_path, ec)) {
        std::cerr << "error: specified sketch does not exist\n";
        return EXIT_FAILURE;
    } else if (ec) {
        std::cerr << "error: " << ec.message() << '\n';
        return EXIT_FAILURE;
    }

    if (std::error_code ec; !stdfs::is_directory(input_path, ec)) {
        if (const auto ext = input_path.extension(); !is_sketch_extension(ext)) {
            std::cerr << "error: Unknown sketch format \"" << ext << "\"\n";
            return EXIT_FAILURE;
        }
        input_path = stdfs::absolute(input_path).parent_path();
    } else if (ec) {
        std::cerr << "error: " << ec.message() << '\n';
        return EXIT_FAILURE;
    }

    for (auto e : stdfs::directory_iterator{input_path}) {
        if (!e.is_directory() && is_sketch_extension(e.path().extension()))
            state.files.push_back(stdfs::absolute(e.path()).generic_string());
    }

    if (state.files.empty()) {
        std::cerr << "error: no sketch files in specified directory\n";
        return EXIT_FAILURE;
    }

    std::sort(state.files.begin(), state.files.end());

    auto index = ::clang_createIndex(false, true);

    constexpr auto composite_name = "<arduino-source>.cxx";

    std::string composite_contents;
    const std::size_t filepaths_total_len =
        std::accumulate(state.files.begin(), state.files.end(), 0,
                        [](std::size_t acc, const std::string& e) { return acc + e.length(); });
    composite_contents.reserve(state.files.size() * sizeof("#include \"\"") + filepaths_total_len);
    for (const auto& fpath : state.files) {
        composite_contents += "#include \"";
        composite_contents += fpath;
        composite_contents += "\"\n";
    }

    ::CXUnsavedFile composite{composite_name, composite_contents.c_str(), composite_contents.size()};
    auto unit = ::clang_parseTranslationUnit(index, composite_name, argv + 2, argc - 2, &composite, 1,
                                             ::CXTranslationUnit_Incomplete);
    if (unit == nullptr) {
        std::cerr << "error: unable to parse translation unit." << std::endl;
        return EXIT_FAILURE;
    }

    auto cursor = ::clang_getTranslationUnitCursor(unit);

    state.ppp = ::clang_getCursorPrintingPolicy(cursor);
    ::clang_PrintingPolicy_setProperty(state.ppp, ::CXPrintingPolicy_TerseOutput, true);

    ::clang_visitChildren(
        cursor,
        [](::CXCursor c, [[maybe_unused]] ::CXCursor parent, ::CXClientData client_data) {
            auto& curr_state = *static_cast<decltype(state)*>(client_data);
            const auto kind = ::clang_getCursorKind(c);
            if (!curr_state.curr_decl.empty()) {
                switch (kind) {
                case ::CXCursor_CompoundStmt: {
                    ::CXString filename;
                    unsigned line;
                    ::clang_getPresumedLocation(curr_state.curr_sloc, &filename, &line, nullptr);
                    std::stringstream ss;
                    const char* fname_cstr = ::clang_getCString(filename);
                    if (std::binary_search(curr_state.files.begin(), curr_state.files.end(), fname_cstr)) {
                        ss << line << " \"" << fname_cstr << '\"';
                        curr_state.functions.emplace(std::move(curr_state.curr_decl), std::move(ss).str());
                    }
                    ::clang_disposeString(filename);
                    curr_state.curr_sloc = ::clang_getNullLocation();
                    curr_state.curr_decl = {};
                    return ::CXChildVisit_Continue;
                }
                case ::CXCursor_ParmDecl:
                case ::CXCursor_TemplateTypeParameter:
                case ::CXCursor_NonTypeTemplateParameter:
                case ::CXCursor_TemplateTemplateParameter:
                    break;
                default:
                    if (clang_isDeclaration(kind)) {
                        curr_state.functions.insert_or_assign(std::move(curr_state.curr_decl), std::nullopt);
                        curr_state.curr_decl = {};
                        curr_state.curr_sloc = ::clang_getNullLocation();
                    }
                }
            }
            switch (kind) {
            case ::CXCursor_FunctionDecl:
            case ::CXCursor_FunctionTemplate: {
                auto decl = ::clang_getCursorPrettyPrinted(c, curr_state.ppp);

                std::string decl_str = ::clang_getCString(decl);
                if (const auto it = curr_state.functions.find(decl_str);
                    it == curr_state.functions.cend() || it->second.has_value()) {
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

    std::cout << "#line 1 \"<arduino source>\"\n"
              << "#include <Arduino.h>\n";

    for (const auto& [decl, def_location] : state.functions) {
        if (!def_location.has_value())
            continue;
        std::cout << "#line " << *def_location << '\n' << decl << ";\n";
    }

    if (std::getenv("ARDUINO_PRELUDE_DUMP_COMPOSITE"))
        std::cout << "#line 1 \"<arduino source>\"\n" << composite_contents;
}
