/*
 *  Copyright (C) 2015, 2016 Marek Marecki
 *
 *  This file is part of Viua VM.
 *
 *  Viua VM is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  Viua VM is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Viua VM.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <tuple>
#include <map>
#include <set>
#include <algorithm>
#include <regex>
#include <viua/support/string.h>
#include <viua/bytecode/maps.h>
#include <viua/cg/lex.h>
#include <viua/cg/assembler/assembler.h>
#include <viua/program.h>
using namespace std;


using ErrorReport = pair<unsigned, string>;
using Token = viua::cg::lex::Token;


static bool is_defined(string function_name, const vector<string>& function_names, const vector<string>& function_signatures) {
    bool is_undefined = (find(function_names.begin(), function_names.end(), function_name) == function_names.end());
    // if function is undefined, check if we got a signature for it
    if (is_undefined) {
        is_undefined = (find(function_signatures.begin(), function_signatures.end(), function_name) == function_signatures.end());
    }
    return (not is_undefined);
}
void assembler::verify::functionCallsAreDefined(const vector<Token>& tokens, const vector<string>& function_names, const vector<string>& function_signatures) {
    ostringstream report("");
    string line;
    for (decltype(tokens.size()) i = 0; i < tokens.size(); ++i) {
        auto token = tokens.at(i);
        if (not (token == "call" or token == "process" or token == "watchdog" or token == "tailcall")) {
            continue;
        }

        if (token == "tailcall" or token == "watchdog") {
            string function_name = tokens.at(i+1);
            if (not is_defined(function_name, function_names, function_signatures)) {
                throw viua::cg::lex::InvalidSyntax(tokens.at(i+1), (string(token == "tailcall" ? "tail call to" : "watchdog from") + " undefined function " + function_name));
            }
        } else if (token == "call" or token == "process") {
            string function_name = tokens.at(i+2);
            if (not is_defined(function_name, function_names, function_signatures)) {
                throw viua::cg::lex::InvalidSyntax(tokens.at(i+2), (string(token == "call" ? "call to" : "process from") + " undefined function " + function_name));
            }
        }
    }
}

void assembler::verify::functionCallArities(const vector<string>& lines) {
    ostringstream report("");
    string line;
    int frame_parameters_count = 0;
    for (unsigned i = 0; i < lines.size(); ++i) {
        line = str::lstrip(lines[i]);
        if (not (str::startswith(line, "call") or str::startswith(line, "process") or str::startswith(line, "watchdog") or str::startswith(line, "frame"))) {
            continue;
        }

        string instr_name = str::chunk(line);
        line = str::lstrip(line.substr(instr_name.size()));

        if (instr_name == "frame") {
            line = str::chunk(line);
            if (str::isnum(line)) {
                frame_parameters_count = stoi(str::chunk(line));
            } else {
                frame_parameters_count = -1;
            }
            continue;
        }

        string function_name = str::chunk(line);
        line = str::lstrip(line.substr(function_name.size()));
        if (line.size()) {
            function_name = str::chunk(line);
        }

        if (not assembler::utils::isValidFunctionName(function_name)) {
            report << "'" << function_name << "' is not a valid function name";
            throw ErrorReport(i, report.str());
        }

        int arity = assembler::utils::getFunctionArity(function_name);

        if (arity == -1) {
            report << "call to function with undefined arity ";
            if (frame_parameters_count >= 0) {
                report << "as " << function_name << (arity == -1 ? "/" : "") << frame_parameters_count;
            } else {
                report << ": " << function_name;
            }
            throw ErrorReport(i, report.str());
        }
        if (frame_parameters_count == -1) {
            // frame paramters count could not be statically determined, deffer the check until runtime
            continue;
        }

        if (arity > 0 and arity != frame_parameters_count) {
            report << "invalid number of parameters in call to function " << function_name << ": expected " << arity << " got " << frame_parameters_count;
            throw ErrorReport(i, report.str());
        }
    }
}

void assembler::verify::msgArities(const vector<string>& lines) {
    ostringstream report("");
    string line;
    int frame_parameters_count = 0;

    for (unsigned i = 0; i < lines.size(); ++i) {
        line = str::lstrip(lines[i]);
        if (not (str::startswith(line, "msg") or str::startswith(line, "frame"))) {
            continue;
        }

        string instr_name = str::chunk(line);
        line = str::lstrip(line.substr(instr_name.size()));

        if (instr_name == "frame") {
            line = str::chunk(line);
            if (str::isnum(line)) {
                frame_parameters_count = stoi(str::chunk(line));
            } else {
                frame_parameters_count = -1;
            }
            continue;
        }

        string function_name = str::chunk(line);
        if (str::isnum(function_name)) {
            function_name = str::chunk(str::lstrip(line.substr(function_name.size())));
        }

        if (not assembler::utils::isValidFunctionName(function_name)) {
            report << "'" << function_name << "' is not a valid function name";
            throw ErrorReport(i, report.str());
        }

        if (frame_parameters_count == 0) {
            report << "invalid number of parameters in dynamic dispatch of " << function_name << ": expected at least 1, got 0";
            throw ErrorReport(i, report.str());
        }

        int arity = assembler::utils::getFunctionArity(function_name);

        if (arity == -1) {
            report << "dynamic dispatch call with undefined arity ";
            if (frame_parameters_count >= 0) {
                report << "as " << function_name << (arity == -1 ? "/" : "") << frame_parameters_count;
            } else {
                report << ": " << function_name;
            }
            throw ErrorReport(i, report.str());
        }
        if (frame_parameters_count == -1) {
            // frame paramters count could not be statically determined, deffer the check until runtime
            continue;
        }

        if (arity > 0 and arity != frame_parameters_count) {
            report << "invalid number of parameters in dynamic dispatch of " << function_name << ": expected " << arity << " got " << frame_parameters_count;
            throw ErrorReport(i, report.str());
        }
    }
}

void assembler::verify::functionNames(const std::vector<std::string>& lines) {
    ostringstream report("");
    string line;
    string function;

    for (unsigned i = 0; i < lines.size(); ++i) {
        line = str::lstrip(lines[i]);
        if (assembler::utils::lines::is_function(line)) {
            function = str::chunk(str::lstrip(str::sub(line, str::chunk(line).size())));
        } else {
            continue;
        }

        if (not assembler::utils::isValidFunctionName(function)) {
            report << "invalid function name: " << function;
            throw ErrorReport(i, report.str());
        }

        if (assembler::utils::getFunctionArity(function) == -1) {
            report << "function with undefined arity: " << function;
            throw ErrorReport(i, report.str());
        }
    }
}

void assembler::verify::functionsEndWithReturn(const std::vector<std::string>& lines) {
    ostringstream report("");
    string line;
    string function;

    for (unsigned i = 0; i < lines.size(); ++i) {
        line = str::lstrip(lines[i]);
        if (assembler::utils::lines::is_function(line)) {
            function = str::chunk(str::lstrip(str::sub(line, str::chunk(line).size())));
            continue;
        } else if (str::startswithchunk(line, ".end") and function.size()) {
            // .end may have been reached while not in a function because blocks also end with .end
            // so we also make sure that we were inside a function
        } else {
            continue;
        }

        if (i and (not (str::startswithchunk(str::lstrip(lines[i-1]), "return") or str::startswithchunk(str::lstrip(lines[i-1]), "tailcall")))) {
            report << "function does not end with 'return' or 'tailcall': " << function;
            throw ErrorReport(i, report.str());
        }

        // if we're here, then the .end at the end of function has been reached and
        // the function name marker should be reset
        function = "";
    }
}

void assembler::verify::frameBalance(const vector<string>& lines, const map<unsigned long, unsigned long>& expanded_lines_to_source_lines) {
    ostringstream report("");
    string line;
    string instruction;

    int balance = 0;
    long unsigned previous_frame_spawnline = 0;
    for (unsigned i = 0; i < lines.size(); ++i) {
        line = lines[i];
        if (line.size() == 0) { continue; }

        line = str::lstrip(line);
        instruction = str::chunk(line);
        if (not (instruction == "call" or instruction == "tailcall" or instruction == "process" or instruction == "watchdog" or instruction == "fcall" or instruction == "frame" or instruction == "msg" or instruction == "return")) {
            continue;
        }

        if (instruction == "call" or instruction == "tailcall" or instruction == "process" or instruction == "fcall" or instruction == "msg" or instruction == "watchdog") {
            --balance;
        }
        if (instruction == "frame") {
            ++balance;
        }

        if (balance < 0) {
            report << "call with '" << instruction << "' without a frame";
            throw ErrorReport(i, report.str());
        }
        if (balance > 1) {
            report << "excess frame spawned (unused frame spawned at line ";
            report << (expanded_lines_to_source_lines.at(previous_frame_spawnline)+1) << ')';
            throw ErrorReport(i, report.str());
        }
        if (instruction == "return" and balance > 0) {
            report << "leftover frame (spawned at line " << (expanded_lines_to_source_lines.at(previous_frame_spawnline)+1) << ')';
            throw ErrorReport(i, report.str());
        }

        if (instruction == "frame") {
            previous_frame_spawnline = i;
        }
    }
}

void assembler::verify::blockTries(const vector<string>& lines, const vector<string>& block_names, const vector<string>& block_signatures) {
    ostringstream report("");
    string line;
    for (unsigned i = 0; i < lines.size(); ++i) {
        line = str::lstrip(lines[i]);
        if (not str::startswithchunk(line, "enter")) {
            continue;
        }

        string block = str::chunk(str::lstrip(str::sub(line, str::chunk(line).size())));
        bool is_undefined = (find(block_names.begin(), block_names.end(), block) == block_names.end());
        // if block is undefined, check if we got a signature for it
        if (is_undefined) {
            is_undefined = (find(block_signatures.begin(), block_signatures.end(), block) == block_signatures.end());
        }

        if (is_undefined) {
            report << "cannot enter undefined block: " << block;
            throw ErrorReport(i, report.str());
        }
    }
}

void assembler::verify::blockCatches(const vector<string>& lines, const vector<string>& block_names, const vector<string>& block_signatures) {
    ostringstream report("");
    string line;
    for (unsigned i = 0; i < lines.size(); ++i) {
        line = str::lstrip(lines[i]);
        if (not str::startswithchunk(line, "catch")) {
            continue;
        }

        // remove instruction
        line = str::lstrip(str::sub(line, str::chunk(line).size()));

        // remove name of caught type
        line = str::lstrip(str::sub(line, str::chunk(line).size()));

        string block = str::chunk(line);
        bool is_undefined = (find(block_names.begin(), block_names.end(), block) == block_names.end());
        // if block is undefined, check if we got a signature for it
        if (is_undefined) {
            is_undefined = (find(block_signatures.begin(), block_signatures.end(), block) == block_signatures.end());
        }

        if (is_undefined) {
            report << "cannot catch using undefined block: " << block;
            throw ErrorReport(i, report.str());
        }
    }
}

void assembler::verify::callableCreations(const vector<string>& lines, const vector<string>& function_names, const vector<string>& function_signatures) {
    ostringstream report("");
    string line;
    string callable_type;
    for (unsigned i = 0; i < lines.size(); ++i) {
        line = str::lstrip(lines[i]);
        if (not str::startswith(line, "closure") and not str::startswith(line, "function")) {
            continue;
        }

        callable_type = str::chunk(line);
        line = str::lstrip(str::sub(line, callable_type.size()));

        string register_index = str::chunk(line);
        line = str::lstrip(str::sub(line, register_index.size()));

        string function = str::chunk(line);
        bool is_undefined = (find(function_names.begin(), function_names.end(), function) == function_names.end());
        // if function is undefined, check if we got a signature for it
        if (is_undefined) {
            is_undefined = (find(function_signatures.begin(), function_signatures.end(), function) == function_signatures.end());
        }

        if (is_undefined) {
            report << callable_type << " from undefined function: " << function;
            throw ErrorReport(i, report.str());
        }
    }
}

void assembler::verify::ressInstructions(const vector<string>& lines, bool as_lib) {
    ostringstream report("");
    vector<string> legal_register_sets = {
        "global",   // global register set
        "local",    // local register set for function
        "static",   // static register set
        "temp",     // temporary register set
    };
    string line;
    string function;
    for (unsigned i = 0; i < lines.size(); ++i) {
        line = str::lstrip(lines[i]);
        if (assembler::utils::lines::is_function(line)) {
            function = str::chunk(str::lstrip(str::sub(line, str::chunk(line).size())));
            continue;
        }
        if (not str::startswith(line, "ress")) {
            continue;
        }

        string registerset_name = str::chunk(str::lstrip(str::sub(line, str::chunk(line).size())));

        if (find(legal_register_sets.begin(), legal_register_sets.end(), registerset_name) == legal_register_sets.end()) {
            report << "illegal register set name in ress instruction '" << registerset_name << "' in function " << function;
            throw ErrorReport(i, report.str());
        }
        if (registerset_name == "global" and as_lib and function != "main/1") {
            report << "global registers used in library function " << function;
            throw ErrorReport(i, report.str());
        }
    }
}

static void bodiesAreNonempty(const vector<string>& lines) {
    ostringstream report("");
    string line, block, function;

    for (unsigned i = 0; i < lines.size(); ++i) {
        line = str::lstrip(lines[i]);
        if (assembler::utils::lines::is_block(line)) {
            block = str::chunk(str::lstrip(str::sub(line, str::chunk(line).size())));
            continue;
        } else if (assembler::utils::lines::is_function(line)) {
            function = str::chunk(str::lstrip(str::sub(line, str::chunk(line).size())));
            continue;
        } else if (assembler::utils::lines::is_closure(line)) {
            function = str::chunk(str::lstrip(str::sub(line, str::chunk(line).size())));
            continue;
        } else if (assembler::utils::lines::is_end(line)) {
            // '.end' is also interesting because we want to see if it's immediately preceded by
            // the interesting prefix
        } else {
            continue;
        }

        if (not (block.size() or function.size())) {
            report << "stray .end marker";
            throw ErrorReport(i, report.str());
        }

        // this if is reached only of '.end' was matched - so we just check if it is preceded by
        // the interesting prefix, and
        // if it is - report an error
        if (i and (assembler::utils::lines::is_block(str::lstrip(lines[i-1])) or assembler::utils::lines::is_function(str::lstrip(lines[i-1])))) {
            report << (function.size() ? "function" : "block") << " with empty body: " << (function.size() ? function : block);
            throw ErrorReport(i, report.str());
        }
        function = "";
        block = "";
    }
}
void assembler::verify::functionBodiesAreNonempty(const vector<string>& lines) {
    bodiesAreNonempty(lines);
}
void assembler::verify::blockBodiesAreNonempty(const std::vector<std::string>& lines) {
    bodiesAreNonempty(lines);
}

void assembler::verify::blocksEndWithFinishingInstruction(const vector<string>& lines) {
    ostringstream report("");
    string line;
    string block;

    for (unsigned i = 0; i < lines.size(); ++i) {
        line = str::lstrip(lines[i]);
        if (assembler::utils::lines::is_block(line)) {
            block = str::chunk(str::lstrip(str::sub(line, string(".block:").size())));
            continue;
        } else if (str::startswithchunk(line, ".end") and block.size()) {
            // .end may have been reached while not in a block because functions also end with .end
            // so we also make sure that we were inside a block
        } else {
            continue;
        }

        if (i and (not (str::startswithchunk(str::lstrip(lines[i-1]), "leave") or str::startswithchunk(str::lstrip(lines[i-1]), "return") or str::startswithchunk(str::lstrip(lines[i-1]), "halt")))) {
            report << "missing returning instruction (leave, return or halt) at the end of block '" << block << "'";
            throw ErrorReport(i, report.str());
        }

        // if we're here, then the .end at the end of function has been reached and
        // the block name marker should be reset
        block = "";
    }
}

void assembler::verify::directives(const vector<Token>& tokens) {
    for (decltype(tokens.size()) i = 0; i < tokens.size(); ++i) {
        if (not (i == 0 or tokens.at(i-1) == "\n")) {
            continue;
        }
        if (tokens.at(i).str().at(0) != '.') {
            continue;
        }
        if (not assembler::utils::lines::is_directive(tokens.at(i))) {
            throw viua::cg::lex::InvalidSyntax(tokens.at(i), "illegal directive");
        }
    }
}

void assembler::verify::instructions(const vector<Token>& tokens) {
    for (decltype(tokens.size()) i = 1; i < tokens.size(); ++i) {
        // instructions can only appear after newline
        if (tokens.at(i-1) != "\n") {
            continue;
        }
        // directives and newlines can *also* apear after newline so filter them out
        if (tokens.at(i).str().at(0) == '.' or tokens.at(i) == "\n") {
            continue;
        }
        if (OP_SIZES.count(tokens.at(i)) == 0) {
            throw viua::cg::lex::InvalidSyntax(tokens.at(i), ("unknown instruction: '" + tokens.at(i).str() + "'"));
        }
    }
}

void assembler::verify::framesHaveOperands(const vector<string>& lines) {
    ostringstream report("");
    string line;

    for (unsigned i = 0; i < lines.size(); ++i) {
        line = str::lstrip(lines[i]);

        if (not str::startswith(line, "frame")) {
            continue;
        }

        // remove leading instruction name and
        // strip the remaining string of leading whitespace
        line = str::lstrip(str::sub(line, str::chunk(line).size()));

        if (line.size() == 0) {
            report << "frame instruction without operands";
            throw ErrorReport(i, report.str());
        }
    }
}

void assembler::verify::framesHaveNoGaps(const vector<string>& lines, const map<unsigned long, unsigned long>& expanded_lines_to_source_lines) {
    ostringstream report("");
    string line;
    unsigned long frame_parameters_count = 0;
    bool detected_frame_parameters_count = false;
    unsigned last_frame = 0;

    vector<bool> filled_slots;
    vector<unsigned> pass_lines;

    for (unsigned i = 0; i < lines.size(); ++i) {
        line = str::lstrip(lines[i]);
        if (not (str::startswith(line, "call") or str::startswith(line, "process") or str::startswith(line, "watchdog") or str::startswith(line, "frame") or str::startswith(line, "param") or str::startswith(line, "pamv"))) {
            continue;
        }

        string instr_name = str::chunk(line);
        line = str::lstrip(line.substr(instr_name.size()));

        if (instr_name == "frame") {
            last_frame = i;

            line = str::chunk(line);
            if (str::isnum(line)) {
                frame_parameters_count = stoul(str::chunk(line));
                filled_slots.clear();
                pass_lines.clear();
                filled_slots.resize(frame_parameters_count, false);
                pass_lines.resize(frame_parameters_count);
                detected_frame_parameters_count = true;
            } else {
                detected_frame_parameters_count = false;
            }
            continue;
        }

        if (instr_name == "param" or instr_name == "pamv") {
            line = str::chunk(line);
            unsigned long slot_index;
            bool detected_slot_index = false;
            if (str::isnum(line)) {
                slot_index = stoul(line);
                detected_slot_index = true;
            }
            if (detected_slot_index and detected_frame_parameters_count and slot_index >= frame_parameters_count) {
                report << "pass to parameter slot " << slot_index << " in frame with only " << frame_parameters_count << " slots available";
                throw ErrorReport(i, report.str());
            }
            if (detected_slot_index and detected_frame_parameters_count) {
                if (filled_slots[slot_index]) {
                    report << "double pass to parameter slot " << slot_index << " in frame defined at line " << expanded_lines_to_source_lines.at(last_frame)+1;
                    report << ", first pass at line " << expanded_lines_to_source_lines.at(pass_lines[slot_index])+1;
                    throw ErrorReport(i, report.str());
                }
                filled_slots[slot_index] = true;
                pass_lines[slot_index] = i;
            }
            continue;
        }

        if (instr_name == "call" or instr_name == "process" or instr_name == "watchdog") {
            for (decltype(frame_parameters_count) f = 0; f < frame_parameters_count; ++f) {
                if (not filled_slots[f]) {
                    report << "gap in frame defined at line " << expanded_lines_to_source_lines.at(last_frame)+1 << ", slot " << f << " left empty";
                    throw ErrorReport(i, report.str());
                }
            }
        }
    }
}

static void validate_jump(const unsigned lineno, const string& extracted_jump, const int function_instruction_counter, vector<pair<unsigned, int>>& forward_jumps, vector<pair<unsigned, string>>& deferred_marker_jumps, const map<string, int>& jump_targets) {
    int target = -1;
    if (str::isnum(extracted_jump, false)) {
        target = stoi(extracted_jump);
    } else if (str::startswith(extracted_jump, "+") and str::isnum(extracted_jump.substr(1))) {
        int jump_offset = stoi(extracted_jump.substr(1));
        if (jump_offset == 0) {
            throw ErrorReport(lineno, "zero-distance jump");
        }
        target = (function_instruction_counter + jump_offset);
    } else if (str::startswith(extracted_jump, "-") and str::isnum(extracted_jump)) {
        int jump_offset = stoi(extracted_jump);
        if (jump_offset == 0) {
            throw ErrorReport(lineno, "zero-distance jump");
        }
        target = (function_instruction_counter + jump_offset);
    } else if (str::startswith(extracted_jump, ".") and str::isnum(extracted_jump.substr(1))) {
        target = stoi(extracted_jump.substr(1));
        if (target < 0) {
            throw ErrorReport(lineno, "absolute jump with negative value");
        }
        if (target == 0 and function_instruction_counter == 0) {
            throw ErrorReport(lineno, "zero-distance jump");
        }
        // absolute jumps cannot be verified without knowing how many bytes the bytecode spans
        // this is a FIXME: add check for absolute jumps
        return;
    } else if (str::ishex(extracted_jump)) {
        // absolute jumps cannot be verified without knowing how many bytes the bytecode spans
        // this is a FIXME: add check for absolute jumps
        return;
    } else {
        if (jump_targets.count(extracted_jump) == 0) {
            deferred_marker_jumps.emplace_back(lineno, extracted_jump);
            return;
        } else {
            // FIXME: jump targets are saved with an off-by-one error, that surfaces when
            // a .mark: directive immediately follows .function: declaration
            target = jump_targets.at(extracted_jump)+1;
        }
    }

    if (target < 0) {
        throw ErrorReport(lineno, "backward out-of-range jump");
    }
    if (target == function_instruction_counter) {
        throw ErrorReport(lineno, "zero-distance jump");
    }
    if (target > function_instruction_counter) {
        forward_jumps.emplace_back(lineno, target);
    }
}

static void verify_forward_jumps(const int function_instruction_counter, const vector<pair<unsigned, int>>& forward_jumps) {
    for (auto jmp : forward_jumps) {
        if (jmp.second > function_instruction_counter) {
            throw ErrorReport(jmp.first, "forward out-of-range jump");
        }
    }
}

static void verify_marker_jumps(const int function_instruction_counter, const vector<pair<unsigned, string>>& deferred_marker_jumps, const map<string, int>& jump_targets) {
    for (auto jmp : deferred_marker_jumps) {
        if (jump_targets.count(jmp.second) == 0) {
            throw ErrorReport(jmp.first, ("jump to unrecognised marker: " + jmp.second));
        }
        if (jump_targets.at(jmp.second) > function_instruction_counter) {
            throw ErrorReport(jmp.first, "marker out-of-range jump");
        }
    }
}

void assembler::verify::jumpsAreInRange(const vector<string>& lines) {
    ostringstream report("");
    string line;

    map<string, int> jump_targets;
    vector<pair<unsigned, int>> forward_jumps;
    vector<pair<unsigned, string>> deferred_marker_jumps;
    int function_instruction_counter = 0;
    string function_name;

    for (unsigned i = 0; i < lines.size(); ++i) {
        line = str::lstrip(lines[i]);

        if (line.size() == 0) {
            continue;
        }
        if (function_name.size() > 0 and not str::startswith(line, ".")) {
            ++function_instruction_counter;
        }
        if (not (str::startswith(line, ".function:") or str::startswith(line, ".closure:") or str::startswith(line, ".block:") or str::startswith(line, "jump") or str::startswith(line, "branch") or str::startswith(line, ".mark:") or str::startswith(line, ".end"))) {
            continue;
        }

        string first_part = str::chunk(line);
        line = str::lstrip(line.substr(first_part.size()));

        string second_part = str::chunk(line);
        line = str::lstrip(line.substr(second_part.size()));

        if (first_part == ".function:" or first_part ==".closure:" or first_part == ".block:") {
            function_instruction_counter = -1;
            jump_targets.clear();
            forward_jumps.clear();
            deferred_marker_jumps.clear();
            function_name = second_part;
        } else if (first_part == "jump") {
            validate_jump(i, second_part, function_instruction_counter, forward_jumps, deferred_marker_jumps, jump_targets);
        } else if (first_part == "branch") {
            if (line.size() == 0) {
                throw ErrorReport(i, "branch without a target");
            }

            string when_true = str::chunk(line);
            line = str::lstrip(line.substr(when_true.size()));

            string when_false = str::chunk(line);

            if (when_true.size()) {
                validate_jump(i, when_true, function_instruction_counter, forward_jumps, deferred_marker_jumps, jump_targets);
            }

            if (when_false.size()) {
                validate_jump(i, when_false, function_instruction_counter, forward_jumps, deferred_marker_jumps, jump_targets);
            }
        } else if (first_part == ".mark:") {
            jump_targets[second_part] = function_instruction_counter;
        } else if (first_part == ".end") {
            function_name = "";
            verify_forward_jumps(function_instruction_counter, forward_jumps);
            verify_marker_jumps(function_instruction_counter, deferred_marker_jumps, jump_targets);
        }
    }
}

static auto skip_till_next_line(const std::vector<viua::cg::lex::Token>& tokens, decltype(tokens.size()) i) -> decltype(i) {
    do {
        ++i;
    } while (i < tokens.size() and tokens.at(i) != "\n");
    return i;
}
static string resolve_register_name(const map<string, string>& named_registers, viua::cg::lex::Token token) {
    string name = token;
    if (name.at(0) == '@') {
        name = name.substr(1);
    }
    if (str::isnum(name, false)) {
        return name;
    }
    if (str::isnum(name, true)) {
        throw viua::cg::lex::InvalidSyntax(token, ("register indexes cannot be negative: " + name));
    }
    if (named_registers.count(name) == 0) {
        throw viua::cg::lex::InvalidSyntax(token, ("not a named register: " + name));
    }
    return named_registers.at(name);
}
static void check_block_body(const vector<viua::cg::lex::Token>& body_tokens, set<string>& defined_registers, const map<string, vector<viua::cg::lex::Token>>& block_bodies, const bool debug) {
    map<string, string> named_registers;
    for (decltype(body_tokens.size()) i = 0; i < body_tokens.size(); ++i) {
        auto token = body_tokens.at(i);
        if (token == "\n") {
            continue;
        }
        if (token == ".name:") {
            named_registers[body_tokens.at(i+2)] = body_tokens.at(i+1);
            if (debug) {
                cout << "  " << "register " << str::enquote(str::strencode(body_tokens.at(i+1))) << " is named " << str::enquote(str::strencode(body_tokens.at(i+2))) << endl;
            }
            i = skip_till_next_line(body_tokens, i);
            continue;
        }
        if (token == "leave" or token == "return") {
            return;
        }
        if (token == ".mark:" or token == "nop" or token == "tryframe" or token == "try" or token == "catch" or token == "frame" or
            token == "tailcall" or token == "halt" or
            token == "watchdog" or token == "jump" or
            token == "link" or token == "import" or token == "ress") {
            i = skip_till_next_line(body_tokens, i);
            continue;
        }
        if (token == "enter") {
            check_block_body(block_bodies.at(body_tokens.at(i+1)), defined_registers, block_bodies, debug);
            i = skip_till_next_line(body_tokens, i);
            continue;
        }
        if (token == "move") {
            if (defined_registers.find(resolve_register_name(named_registers, body_tokens.at(i+2))) == defined_registers.end()) {
                throw viua::cg::lex::InvalidSyntax(body_tokens.at(i+2), ("move from empty register: " + str::strencode(body_tokens.at(i+2))));
            }
            defined_registers.insert(resolve_register_name(named_registers, body_tokens.at(i+1)));
            defined_registers.erase(defined_registers.find(resolve_register_name(named_registers, body_tokens.at(i+2))));
            i = skip_till_next_line(body_tokens, i);
            continue;
        } else if (token == "vpop" or token == "vat" or token == "vlen") {
            if (defined_registers.find(resolve_register_name(named_registers, body_tokens.at(i+2))) == defined_registers.end()) {
                throw viua::cg::lex::InvalidSyntax(body_tokens.at(i+2), (token.str() + " from empty register: " + str::strencode(body_tokens.at(i+2))));
            }
            defined_registers.insert(resolve_register_name(named_registers, body_tokens.at(i+1)));
            i = skip_till_next_line(body_tokens, i);
            continue;
        } else if (token == "vinsert" or token == "vpush") {
            if (defined_registers.find(resolve_register_name(named_registers, body_tokens.at(i+1))) == defined_registers.end()) {
                throw viua::cg::lex::InvalidSyntax(body_tokens.at(i+1), (token.str() + " into empty register: " + str::strencode(body_tokens.at(i+1))));
            }
            if (defined_registers.find(resolve_register_name(named_registers, body_tokens.at(i+2))) == defined_registers.end()) {
                throw viua::cg::lex::InvalidSyntax(body_tokens.at(i+2), (token.str() + " from empty register: " + str::strencode(body_tokens.at(i+2))));
            }
            defined_registers.erase(defined_registers.find(resolve_register_name(named_registers, body_tokens.at(i+2))));
            i = skip_till_next_line(body_tokens, i);
            continue;
        } else if (token == "delete") {
            if (defined_registers.find(resolve_register_name(named_registers, body_tokens.at(i+1))) == defined_registers.end()) {
                throw viua::cg::lex::InvalidSyntax(body_tokens.at(i+1), ("delete of empty register: " + str::strencode(body_tokens.at(i+1))));
            }
            defined_registers.erase(defined_registers.find(resolve_register_name(named_registers, body_tokens.at(i+1))));
            i = skip_till_next_line(body_tokens, i);
        } else if (token == "branch") {
            if (defined_registers.find(resolve_register_name(named_registers, body_tokens.at(i+1))) == defined_registers.end()) {
                throw viua::cg::lex::InvalidSyntax(body_tokens.at(i+1), ("branch depends on empty register: " + str::strencode(body_tokens.at(i+1))));
            }
            i = skip_till_next_line(body_tokens, i);
        } else if (token == "echo" or token == "print" or token == "not") {
            if (defined_registers.find(resolve_register_name(named_registers, body_tokens.at(i+1))) == defined_registers.end()) {
                throw viua::cg::lex::InvalidSyntax(body_tokens.at(i+1), (token.str() + " of empty register: " + str::strencode(body_tokens.at(i+1))));
            }
            i = skip_till_next_line(body_tokens, i);
        } else if (token == "pamv" or token == "param") {
            if (debug) {
                cout << str::enquote(token) << " from register " << str::enquote(str::strencode(body_tokens.at(i+2))) << endl;
            }
            if (defined_registers.find(resolve_register_name(named_registers, body_tokens.at(i+2))) == defined_registers.end()) {
                throw viua::cg::lex::InvalidSyntax(body_tokens.at(i+2), ("parameter " + string(token.str() == "pamv" ? "move" : "pass") + " from empty register: " + str::strencode(body_tokens.at(i+2))));
            }
            if (token == "pamv") {
                defined_registers.erase(defined_registers.find(resolve_register_name(named_registers, body_tokens.at(i+2))));
            }
            i = skip_till_next_line(body_tokens, i);
            continue;
        } else if (token == "enclose" or token == "enclosecopy" or token == "enclosemove") {
            if (defined_registers.find(resolve_register_name(named_registers, body_tokens.at(i+2))) == defined_registers.end()) {
                throw viua::cg::lex::InvalidSyntax(body_tokens.at(i+2), ("closure of empty register: " + str::strencode(body_tokens.at(i+2))));
            }
            i = skip_till_next_line(body_tokens, i);
        } else if (token == "copy") {
            string copy_from = body_tokens.at(i+2);
            if ((not str::isnum(copy_from)) and named_registers.count(copy_from)) {
                copy_from = named_registers.at(copy_from);
            }
            if (defined_registers.find(resolve_register_name(named_registers, body_tokens.at(i+2))) == defined_registers.end()) {
                throw viua::cg::lex::InvalidSyntax(body_tokens.at(i+2), ("copy from empty register: " + str::strencode(body_tokens.at(i+2))));
            }
            defined_registers.insert(resolve_register_name(named_registers, body_tokens.at(i+1)));
            i = skip_till_next_line(body_tokens, i);
            continue;
        } else if (token == "swap") {
            if (defined_registers.find(resolve_register_name(named_registers, body_tokens.at(i+1))) == defined_registers.end()) {
                throw viua::cg::lex::InvalidSyntax(body_tokens.at(i+1), ("swap with empty register: " + str::strencode(body_tokens.at(i+1))));
            }
            if (defined_registers.find(resolve_register_name(named_registers, body_tokens.at(i+2))) == defined_registers.end()) {
                throw viua::cg::lex::InvalidSyntax(body_tokens.at(i+2), ("swap with empty register: " + str::strencode(body_tokens.at(i+2))));
            }
            i = skip_till_next_line(body_tokens, i);
            continue;
        } else if (token == "itof" or token == "ftoi" or token == "stoi" or token == "stof") {
            if (defined_registers.find(resolve_register_name(named_registers, body_tokens.at(i+2))) == defined_registers.end()) {
                throw viua::cg::lex::InvalidSyntax(body_tokens.at(i+1), ("use of empty register: " + str::strencode(body_tokens.at(i+2))));
            }
            defined_registers.insert(resolve_register_name(named_registers, body_tokens.at(i+1)));
            i = skip_till_next_line(body_tokens, i);
            continue;
        } else if (token == "vec") {
            ++i; // the "vec" token
            int starting_register = stoi(resolve_register_name(named_registers, body_tokens.at(i+1)));
            int registers_to_pack = stoi(resolve_register_name(named_registers, body_tokens.at(i+2)));
            if (registers_to_pack) {
                for (int j = starting_register; j < (starting_register+registers_to_pack); ++j) {
                    if (defined_registers.find(str::stringify(j, false)) == defined_registers.end()) {
                        throw viua::cg::lex::InvalidSyntax(token, ("packing empty register: " + str::stringify(j, false)));
                    }
                    defined_registers.erase(str::stringify(j, false));
                }
            }
            defined_registers.insert(resolve_register_name(named_registers, body_tokens.at(i)));
            i = skip_till_next_line(body_tokens, i);
            continue;
        } else if (token == "iadd" or token == "isub" or token == "imul" or token == "idiv" or
                   token == "ilt" or token == "ilte" or token == "igt" or token == "igte" or token == "ieq" or
                   token == "fadd" or token == "fsub" or token == "fmul" or token == "fdiv" or
                   token == "flt" or token == "flte" or token == "fgt" or token == "fgte" or token == "feq" or
                   token == "and" or token == "or") {
            ++i; // skip mnemonic token
            if (defined_registers.find(resolve_register_name(named_registers, body_tokens.at(i+1))) == defined_registers.end()) {
                throw viua::cg::lex::InvalidSyntax(token, ("use of empty register: " + body_tokens.at(i+1).str()));
            }
            if (body_tokens.at(i+2) != "\n" and defined_registers.find(resolve_register_name(named_registers, body_tokens.at(i+2))) == defined_registers.end()) {
                throw viua::cg::lex::InvalidSyntax(token, ("use of empty register: " + body_tokens.at(i+2).str()));
            }
            defined_registers.insert(resolve_register_name(named_registers, body_tokens.at(i)));
            i = skip_till_next_line(body_tokens, i);
            continue;
        } else {
            if (not ((token == "call" or token == "process") and body_tokens.at(i+1) == "0")) {
                string reg_original = body_tokens.at(i+1), reg = resolve_register_name(named_registers, body_tokens.at(i+1));
                defined_registers.insert(reg);
                if (debug) {
                    cout << "  " << str::enquote(token) << " defined register " << str::enquote(str::strencode(reg_original));
                    if (reg != reg_original) {
                        cout << " = " << str::enquote(str::strencode(reg));
                    }
                    cout << endl;
                }
            }
            i = skip_till_next_line(body_tokens, i);
            continue;
        }
    }
}
void assembler::verify::manipulationOfDefinedRegisters(const std::vector<viua::cg::lex::Token>& tokens, const map<string, vector<viua::cg::lex::Token>>& block_bodies, const bool debug) {
    set<string> defined_registers;
    string opened_function;

    vector<viua::cg::lex::Token> body;
    for (decltype(tokens.size()) i = 0; i < tokens.size(); ++i) {
        auto token = tokens.at(i);
        if (token == ".function:") {
            opened_function = tokens.at(i+1);
            if (debug) {
                cout << "analysing '" << opened_function << "'\n";
            }
            i = skip_till_next_line(tokens, i);
            continue;
        }
        if (token == ".end") {
            if (debug) {
                cout << "running analysis of '" << opened_function << "' (" << body.size() << " tokens)\n";
            }
            check_block_body(body, defined_registers, block_bodies, debug);
            defined_registers.clear();
            body.clear();
            opened_function = "";
            i = skip_till_next_line(tokens, i);
            continue;
        }
        if (opened_function == "") {
            continue;
        }
        body.push_back(token);
    }
}
