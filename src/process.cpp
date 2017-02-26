/*
 *  Copyright (C) 2015, 2016, 2017 Marek Marecki
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
#include <algorithm>
#include <viua/bytecode/opcodes.h>
#include <viua/bytecode/maps.h>
#include <viua/types/integer.h>
#include <viua/types/exception.h>
#include <viua/types/reference.h>
#include <viua/types/process.h>
#include <viua/process.h>
#include <viua/kernel/kernel.h>
#include <viua/scheduler/vps.h>
using namespace std;


viua::process::Stack::Stack(string fn, viua::kernel::RegisterSet** curs, viua::kernel::RegisterSet* gs, viua::scheduler::VirtualProcessScheduler* sch):
    entry_function(fn),
    jump_base(nullptr),
    instruction_pointer(nullptr),
    frame_new(nullptr), try_frame_new(nullptr),
    thrown(nullptr), caught(nullptr),
    currently_used_register_set(curs),
    global_register_set(gs),
    return_value(nullptr),
    scheduler(sch)
{
}


auto viua::process::Stack::bind(viua::kernel::RegisterSet** curs, viua::kernel::RegisterSet* gs) -> void {
    currently_used_register_set = curs;
    global_register_set = gs;
}

auto viua::process::Stack::begin() const -> decltype(frames.begin()) {
    return frames.begin();
}

auto viua::process::Stack::end() const -> decltype(frames.end()) {
    return frames.end();
}

auto viua::process::Stack::at(decltype(frames)::size_type i) const -> decltype(frames.at(i)) {
    return frames.at(i);
}

auto viua::process::Stack::back() const -> decltype(frames.back()) {
    return frames.back();
}

auto viua::process::Stack::pop() -> unique_ptr<Frame> {
    unique_ptr<Frame> frame { std::move(frames.back()) };
    frames.pop_back();

    for (viua::internals::types::register_index i = 0; i < frame->arguments->size(); ++i) {
        if (frame->arguments->at(i) != nullptr and frame->arguments->isflagged(i, MOVED)) {
            throw new viua::types::Exception("unused pass-by-move parameter");
        }
    }

    if (size() == 0) {
        return_value = frame->local_register_set->pop(0);
    }

    if (size()) {
        *currently_used_register_set = back()->local_register_set.get();
    } else {
        *currently_used_register_set = global_register_set;
    }

    return frame;
}

auto viua::process::Stack::size() const -> decltype(frames)::size_type {
    return frames.size();
}

auto viua::process::Stack::clear() -> void {
    frames.clear();
}

auto viua::process::Stack::emplace_back(unique_ptr<Frame> frame) -> decltype(frames.emplace_back(frame)) {
    return frames.emplace_back(std::move(frame));
}

viua::internals::types::byte* viua::process::Stack::adjust_jump_base_for_block(const string& call_name) {
    viua::internals::types::byte *entry_point = nullptr;
    auto ep = scheduler->getEntryPointOfBlock(call_name);
    entry_point = ep.first;
    jump_base = ep.second;
    return entry_point;
}
viua::internals::types::byte* viua::process::Stack::adjust_jump_base_for(const string& call_name) {
    viua::internals::types::byte *entry_point = nullptr;
    auto ep = scheduler->getEntryPointOf(call_name);
    entry_point = ep.first;
    jump_base = ep.second;
    return entry_point;
}

auto viua::process::Stack::adjust_instruction_pointer(TryFrame* tframe, string handler_found_for_type) -> void {
    instruction_pointer = adjust_jump_base_for_block(tframe->catchers.at(handler_found_for_type)->catcher_name);
}
auto viua::process::Stack::unwind_call_stack_to(TryFrame* tframe) -> void {
    size_type distance = 0;
    for (size_type j = (size()-1); j > 1; --j) {
        if (at(j).get() == tframe->associated_frame) {
            break;
        }
        ++distance;
    }
    for (size_type j = 0; j < distance; ++j) {
        pop();
    }
}
auto viua::process::Stack::unwind_try_stack_to(TryFrame* tframe) -> void {
    while (tryframes.back().get() != tframe) {
        tryframes.pop_back();
    }
}

auto viua::process::Stack::unwind_to(TryFrame* tframe, string handler_found_for_type) -> void {
    adjust_instruction_pointer(tframe, handler_found_for_type);
    unwind_call_stack_to(tframe);
    unwind_try_stack_to(tframe);
}

auto viua::process::Stack::find_catch_frame() -> tuple<TryFrame*, string> {
    TryFrame* found_exception_frame = nullptr;
    string caught_with_type = "";

    for (decltype(tryframes)::size_type i = tryframes.size(); i > 0; --i) {
        TryFrame* tframe = tryframes[(i-1)].get();
        string handler_found_for_type = thrown->type();
        bool handler_found = tframe->catchers.count(handler_found_for_type);

        // FIXME: mutex
        if ((not handler_found) and scheduler->isClass(handler_found_for_type)) {
            vector<string> types_to_check = scheduler->inheritanceChainOf(handler_found_for_type);
            for (decltype(types_to_check)::size_type j = 0; j < types_to_check.size(); ++j) {
                if (tframe->catchers.count(types_to_check[j])) {
                    handler_found = true;
                    handler_found_for_type = types_to_check[j];
                    break;
                }
            }
        }

        if (handler_found) {
            found_exception_frame = tframe;
            caught_with_type = handler_found_for_type;
            break;
        }
    }

    return tuple<TryFrame*, string>(found_exception_frame, caught_with_type);
}

auto viua::process::Stack::unwind() -> void {
    TryFrame* tframe = nullptr;
    string handler_found_for_type = "";

    tie(tframe, handler_found_for_type) = find_catch_frame();
    if (tframe != nullptr) {
        unwind_to(tframe, handler_found_for_type);
        caught = std::move(thrown);
    }
}

auto viua::process::Stack::prepare_frame(viua::internals::types::register_index arguments_size, viua::internals::types::register_index registers_size) -> Frame* {
    if (frame_new) { throw "requested new frame while last one is unused"; }
    frame_new.reset(new Frame(nullptr, arguments_size, registers_size));
    return frame_new.get();
}

auto viua::process::Stack::push_prepared_frame() -> void {
    if (size() > MAX_STACK_SIZE) {
        ostringstream oss;
        oss << "stack size (" << MAX_STACK_SIZE << ") exceeded with call to '" << frame_new->function_name << '\'';
        throw new viua::types::Exception(oss.str());
    }

    *currently_used_register_set = frame_new->local_register_set.get();
    if (find(begin(), end(), frame_new) != end()) {
        ostringstream oss;
        oss << "stack corruption: frame ";
        oss << hex << frame_new.get() << dec;
        oss << " for function " << frame_new->function_name << '/' << frame_new->arguments->size();
        oss << " pushed more than once";
        throw oss.str();
    }
    emplace_back(std::move(frame_new));
}


auto viua::process::Process::current_stack() const -> Stack* {
    if (activations.empty()) {
        throw new viua::types::Exception("no stack available");
    }
    return activations.back();
}


viua::types::Type* viua::process::Process::fetch(viua::internals::types::register_index index) const {
    /*  Return pointer to object at given register.
     *  This method safeguards against reaching for out-of-bounds registers and
     *  reading from an empty register.
     */
    viua::types::Type* object = currently_used_register_set->get(index);
    if (dynamic_cast<viua::types::Reference*>(object)) {
        object = static_cast<viua::types::Reference*>(object)->pointsTo();
    }
    return object;
}
viua::types::Type* viua::process::Process::obtain(viua::internals::types::register_index index) const {
    return fetch(index);
}

viua::kernel::Register* viua::process::Process::register_at(viua::internals::types::register_index i) {
    return currently_used_register_set->register_at(i);
}

viua::kernel::Register* viua::process::Process::register_at(viua::internals::types::register_index i, viua::internals::RegisterSets rs) {
    if (rs == viua::internals::RegisterSets::CURRENT) {
        return currently_used_register_set->register_at(i);
    } else if (rs == viua::internals::RegisterSets::LOCAL) {
        return current_stack()->back()->local_register_set->register_at(i);
    } else if (rs == viua::internals::RegisterSets::STATIC) {
        ensureStaticRegisters(current_stack()->back()->function_name);
        return static_registers.at(current_stack()->back()->function_name)->register_at(i);
    } else if (rs == viua::internals::RegisterSets::GLOBAL) {
        return global_register_set->register_at(i);
    } else {
        throw new viua::types::Exception("unsupported register set type");
    }
}

unique_ptr<viua::types::Type> viua::process::Process::pop(viua::internals::types::register_index index) {
    return currently_used_register_set->pop(index);
}
void viua::process::Process::place(viua::internals::types::register_index index, unique_ptr<viua::types::Type> obj) {
    /** Place an object in register with given index.
     *
     *  Before placing an object in register, a check is preformed if the register is empty.
     *  If not - the `viua::types::Type` previously stored in it is destroyed.
     *
     */
    currently_used_register_set->set(index, std::move(obj));
}
void viua::process::Process::put(viua::internals::types::register_index index, unique_ptr<viua::types::Type> o) {
    place(index, std::move(o));
}
void viua::process::Process::ensureStaticRegisters(string function_name) {
    /** Makes sure that static register set for requested function is initialized.
     */
    try {
        static_registers.at(function_name);
    } catch (const std::out_of_range& e) {
        // FIXME: amount of static registers should be customizable
        static_registers[function_name] = unique_ptr<viua::kernel::RegisterSet>(new viua::kernel::RegisterSet(16));
    }
}

Frame* viua::process::Process::requestNewFrame(viua::internals::types::register_index arguments_size, viua::internals::types::register_index registers_size) {
    return current_stack()->prepare_frame(arguments_size, registers_size);
}
void viua::process::Process::pushFrame() {
    /** Pushes new frame to be the current (top-most) one.
     */
    if (current_stack()->size() > MAX_STACK_SIZE) {
        ostringstream oss;
        oss << "stack size (" << MAX_STACK_SIZE << ") exceeded with call to '" << current_stack()->frame_new->function_name << '\'';
        throw new viua::types::Exception(oss.str());
    }

    currently_used_register_set = current_stack()->frame_new->local_register_set.get();
    if (find(current_stack()->begin(), current_stack()->end(), current_stack()->frame_new) != current_stack()->end()) {
        ostringstream oss;
        oss << "stack corruption: frame " << hex << current_stack()->frame_new.get() << dec << " for function " << current_stack()->frame_new->function_name << '/' << current_stack()->frame_new->arguments->size() << " pushed more than once";
        throw oss.str();
    }
    current_stack()->emplace_back(std::move(current_stack()->frame_new));
}

viua::internals::types::byte* viua::process::Process::adjustJumpBaseForBlock(const string& call_name) {
    return current_stack()->adjust_jump_base_for_block(call_name);
}
viua::internals::types::byte* viua::process::Process::adjustJumpBaseFor(const string& call_name) {
    return current_stack()->adjust_jump_base_for(call_name);
}
viua::internals::types::byte* viua::process::Process::callNative(viua::internals::types::byte* return_address, const string& call_name, viua::kernel::Register* return_register, const string&) {
    viua::internals::types::byte* call_address = adjustJumpBaseFor(call_name);

    if (not current_stack()->frame_new) {
        throw new viua::types::Exception("function call without a frame: use `frame 0' in source code if the function takes no parameters");
    }

    current_stack()->frame_new->function_name = call_name;
    current_stack()->frame_new->return_address = return_address;
    current_stack()->frame_new->return_register = return_register;

    pushFrame();

    return call_address;
}
viua::internals::types::byte* viua::process::Process::callForeign(viua::internals::types::byte* return_address, const string& call_name, viua::kernel::Register* return_register, const string&) {
    if (not current_stack()->frame_new) {
        throw new viua::types::Exception("external function call without a frame: use `frame 0' in source code if the function takes no parameters");
    }

    current_stack()->frame_new->function_name = call_name;
    current_stack()->frame_new->return_address = return_address;
    current_stack()->frame_new->return_register = return_register;

    suspend();
    scheduler->requestForeignFunctionCall(current_stack()->frame_new.release(), this);

    return return_address;
}
viua::internals::types::byte* viua::process::Process::callForeignMethod(viua::internals::types::byte* return_address, viua::types::Type* object, const string& call_name, viua::kernel::Register* return_register, const string&) {
    if (not current_stack()->frame_new) {
        throw new viua::types::Exception("foreign method call without a frame");
    }

    current_stack()->frame_new->function_name = call_name;
    current_stack()->frame_new->return_address = return_address;
    current_stack()->frame_new->return_register = return_register;

    Frame* frame = current_stack()->frame_new.get();

    pushFrame();

    if (not scheduler->isForeignMethod(call_name)) {
        throw new viua::types::Exception("call to unregistered foreign method: " + call_name);
    }

    viua::types::Reference* rf = nullptr;
    if ((rf = dynamic_cast<viua::types::Reference*>(object))) {
        object = rf->pointsTo();
    }

    try {
        // FIXME: supply static and global registers to foreign functions
        scheduler->requestForeignMethodCall(call_name, object, frame, nullptr, nullptr, this);
    } catch (const std::out_of_range& e) {
        throw new viua::types::Exception(e.what());
    }

    // FIXME: woohoo! segfault!
    unique_ptr<viua::types::Type> returned;
    if (return_register != nullptr) {
        // we check in 0. register because it's reserved for return values
        if (currently_used_register_set->at(0) == nullptr) {
            throw new viua::types::Exception("return value requested by frame but foreign method did not set return register");
        }
        returned = currently_used_register_set->pop(0);
    }

    current_stack()->pop();

    // place return value
    if (returned and current_stack()->size() > 0) {
        *return_register = std::move(returned);
    }

    return return_address;
}

void viua::process::Process::handleActiveException() {
    current_stack()->unwind();
}
viua::internals::types::byte* viua::process::Process::tick() {
    bool halt = false;

    viua::internals::types::byte* previous_instruction_pointer = current_stack()->instruction_pointer;

    try {
        current_stack()->instruction_pointer = dispatch(current_stack()->instruction_pointer);
    } catch (viua::types::Exception* e) {
        /* All machine-thrown exceptions are passed back to user code.
         * This is much easier than checking for erroneous conditions and
         * terminating functions conditionally, instead - machine just throws viua::types::Exception objects which
         * are then caught here.
         *
         * If user code cannot deal with them (i.e. did not register a catcher block) they will terminate execution later.
         */
        current_stack()->thrown.reset(e);
    } catch (const HaltException& e) {
        halt = true;
    } catch (viua::types::Type* e) {
        current_stack()->thrown.reset(e);
    } catch (const char* e) {
        current_stack()->thrown.reset(new viua::types::Exception(e));
    }

    if (halt or current_stack()->size() == 0) {
        finished = true;
        return nullptr;
    }

    /*  Machine should halt execution if previous instruction pointer is the same as current one as
     *  it means that the execution flow is corrupted and
     *  entered an infinite loop.
     *
     *  However, execution *should not* be halted if:
     *      - the offending opcode is RETURN (as this may indicate exiting recursive function),
     *      - the offending opcode is JOIN (as this means that a process is waiting for another process to finish),
     *      - the offending opcode is RECEIVE (as this means that a process is waiting for a message),
     *      - an object has been thrown, as the instruction pointer will be adjusted by
     *        catchers or execution will be halted on unhandled types,
     */
    if (current_stack()->instruction_pointer == previous_instruction_pointer and (OPCODE(*current_stack()->instruction_pointer) != RETURN and OPCODE(*current_stack()->instruction_pointer) != JOIN and OPCODE(*current_stack()->instruction_pointer) != RECEIVE) and (not current_stack()->thrown)) {
        current_stack()->thrown.reset(new viua::types::Exception("InstructionUnchanged"));
    }

    if (current_stack()->thrown and current_stack()->frame_new) {
        /*  Delete active frame after an exception is thrown.
         *  There're two reasons for such behaviour:
         *  - it prevents memory leaks if an exception escapes and
         *    is handled by the watchdog process,
         *  - if the exception is caught, it provides the servicing block with a
         *    clean environment (as there is no way of dropping a frame without
         *    using it),
         */
        current_stack()->frame_new.reset(nullptr);
    }

    if (current_stack()->thrown) {
        handleActiveException();
    }

    if (current_stack()->thrown) {
        return nullptr;
    }

    return current_stack()->instruction_pointer;
}

void viua::process::Process::join() {
    /** Join a process with calling process.
     *
     *  This function causes calling process to be blocked until
     *  this process has stopped.
     */
    is_joinable.store(false, std::memory_order_release);
}
void viua::process::Process::detach() {
    /** Detach a process.
     *
     *  This function causes the process to become unjoinable, but
     *  allows it to run in the background.
     *
     *  Keep in mind that while detached processes cannot be joined,
     *  they can receive messages.
     *  Also, they will run even after the main/ function has exited.
     */
    is_joinable.store(false, std::memory_order_release);
    parent_process = nullptr;
}
bool viua::process::Process::joinable() const {
    return is_joinable.load(std::memory_order_acquire);
}

void viua::process::Process::suspend() {
    is_suspended.store(true, std::memory_order_release);
}
void viua::process::Process::wakeup() {
    is_suspended.store(false, std::memory_order_release);
}
bool viua::process::Process::suspended() const {
    return is_suspended.load(std::memory_order_acquire);
}

viua::process::Process* viua::process::Process::parent() const {
    return parent_process;
}

string viua::process::Process::starting_function() const {
    return current_stack()->entry_function;
}

auto viua::process::Process::priority() const -> decltype(process_priority) {
    return process_priority;
}
void viua::process::Process::priority(decltype(process_priority) p) {
    process_priority = p;
}

bool viua::process::Process::stopped() const {
    return (finished or terminated());
}

bool viua::process::Process::terminated() const {
    return static_cast<bool>(current_stack()->thrown);
}

void viua::process::Process::pass(unique_ptr<viua::types::Type> message) {
    message_queue.push(std::move(message));
    wakeup();
}


viua::types::Type* viua::process::Process::getActiveException() {
    return current_stack()->thrown.get();
}

unique_ptr<viua::types::Type> viua::process::Process::transferActiveException() {
    return std::move(current_stack()->thrown);
}

void viua::process::Process::raise(unique_ptr<viua::types::Type> exception) {
    current_stack()->thrown = std::move(exception);
}


unique_ptr<viua::types::Type> viua::process::Process::getReturnValue() {
    return std::move(current_stack()->return_value);
}

bool viua::process::Process::watchdogged() const {
    return (not watchdog_function.empty());
}
string viua::process::Process::watchdog() const {
    return watchdog_function;
}
viua::internals::types::byte* viua::process::Process::become(const string& function_name, std::unique_ptr<Frame> frame_to_use) {
    if (not scheduler->isNativeFunction(function_name)) {
        throw new viua::types::Exception("process from undefined function: " + function_name);
    }

    current_stack()->clear();
    current_stack()->thrown.reset(nullptr);
    current_stack()->caught.reset(nullptr);
    finished = false;

    frame_to_use->function_name = function_name;
    current_stack()->frame_new = std::move(frame_to_use);

    pushFrame();

    return (current_stack()->instruction_pointer = adjustJumpBaseFor(function_name));
}

viua::internals::types::byte* viua::process::Process::begin() {
    if (not scheduler->isNativeFunction(current_stack()->at(0)->function_name)) {
        throw new viua::types::Exception("process from undefined function: " + current_stack()->at(0)->function_name);
    }
    return (current_stack()->instruction_pointer = adjustJumpBaseFor(current_stack()->at(0)->function_name));
}
auto viua::process::Process::executionAt() const -> decltype(current_stack()->instruction_pointer) {
    return current_stack()->instruction_pointer;
}


vector<Frame*> viua::process::Process::trace() const {
    vector<Frame*> tr;
    for (auto& each : (*current_stack())) {
        tr.push_back(each.get());
    }
    return tr;
}

viua::process::PID viua::process::Process::pid() const {
    return process_id;
}
bool viua::process::Process::hidden() const {
    return is_hidden;
}
void viua::process::Process::hidden(bool state) {
    is_hidden = state;
}

bool viua::process::Process::empty() const {
    return message_queue.empty();
}

void viua::process::Process::migrate_to(viua::scheduler::VirtualProcessScheduler *sch) {
    scheduler = sch;
}

auto viua::process::Process::spawn_stack(const string function_name) -> Stack* {
    unique_ptr<Stack> new_stack { new Stack(function_name, &currently_used_register_set, global_register_set.get(), scheduler) } ;
    Stack* key = new_stack.get();
    stacks.emplace(key, std::move(new_stack));
    return key;
}

viua::process::Process::Process(unique_ptr<Frame> frm, viua::scheduler::VirtualProcessScheduler *sch, viua::process::Process* pt): scheduler(sch), parent_process(pt),
    global_register_set(nullptr), currently_used_register_set(nullptr),
    finished(false), is_joinable(true),
    is_suspended(false),
    process_priority(1),
    process_id(this),
    is_hidden(false)
{
    /*
     *  Initialise global register set.
     *  This has to be done before any stack initialisation because stacks use this register set.
     */
    global_register_set.reset(new viua::kernel::RegisterSet(DEFAULT_REGISTER_SIZE));

    /*
     *  Spawn the initial stack (initial coroutine of the process), and
     *  push it on the activation stack.
     *  This initialises the process by setting a coroutine that can be run inside it.
     */
    auto key = spawn_stack(frm->function_name);
    activations.push_back(key);

    /*
     *  All processes start running using the local register set.
     *  Make it so.
     */
    currently_used_register_set = frm->local_register_set.get();

    /*
     *  Make current stack runnable: this requires binding it to a process (for global and
     *  static register sets), and putting the initial frame on it.
     */
    current_stack()->emplace_back(std::move(frm));
    current_stack()->bind(&currently_used_register_set, global_register_set.get());
}

viua::process::Process::~Process() {}
