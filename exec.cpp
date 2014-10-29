#include "exec.h"

#include <assert.h>
#include <iso646.h>
#include <iostream>
#include <map>
#include <sstream>
#include <stack>
#include <stdlib.h>

#include "bytecode.h"
#include "cell.h"
#include "number.h"
#include "opcode.h"
#include "rtl.h"

class ActivationFrame {
public:
    ActivationFrame(size_t count): locals(count) {}
    // TODO (for nested functions) std::vector<ActivationFrame *> outer;
    std::vector<Variant> locals;
};

class Executor {
public:
    Executor(const Bytecode::bytecode &obj);
    void exec();
private:
    const Bytecode obj;
    Bytecode::bytecode::size_type ip;
    std::stack<Variant> stack;
    std::stack<Bytecode::bytecode::size_type> callstack;
    std::vector<Variant> globals;
    std::vector<ActivationFrame> frames;

    void exec_ENTER();
    void exec_LEAVE();
    void exec_PUSHB();
    void exec_PUSHN();
    void exec_PUSHS();
    void exec_PUSHAG();
    void exec_PUSHAL();
    void exec_LOADB();
    void exec_LOADN();
    void exec_LOADS();
    void exec_STOREB();
    void exec_STOREN();
    void exec_STORES();
    void exec_NEGN();
    void exec_ADDN();
    void exec_SUBN();
    void exec_MULN();
    void exec_DIVN();
    void exec_MODN();
    void exec_EXPN();
    void exec_EQN();
    void exec_NEN();
    void exec_LTN();
    void exec_GTN();
    void exec_LEN();
    void exec_GEN();
    void exec_EQS();
    void exec_NES();
    void exec_LTS();
    void exec_GTS();
    void exec_LES();
    void exec_GES();
    void exec_ANDB();
    void exec_ORB();
    void exec_NOTB();
    void exec_INDEXA();
    void exec_INDEXD();
    void exec_CALLP();
    void exec_CALLF();
    void exec_JUMP();
    void exec_JF();
    void exec_RET();
private:
    Executor(const Executor &);
    Executor &operator=(const Executor &);
};

Executor::Executor(const Bytecode::bytecode &obj)
  : obj(obj),
    ip(0),
    stack(),
    callstack(),
    globals(),
    frames()
{
}

void Executor::exec_ENTER()
{
    int val = (obj.code[ip+1] << 24) | (obj.code[ip+2] << 16) | (obj.code[ip+3] << 8) | obj.code[ip+4];
    ip += 5;
    frames.push_back(ActivationFrame(val));
}

void Executor::exec_LEAVE()
{
    frames.pop_back();
    ip++;
}

void Executor::exec_PUSHB()
{
    bool val = obj.code[ip+1] != 0;
    ip += 2;
    stack.push(Variant(val));
}

void Executor::exec_PUSHN()
{
    // TODO: endian
    Number val = *reinterpret_cast<const Number *>(&obj.code[ip+1]);
    ip += 1 + sizeof(val);
    stack.push(Variant(val));
}

void Executor::exec_PUSHS()
{
    int val = (obj.code[ip+1] << 24) | (obj.code[ip+2] << 16) | (obj.code[ip+3] << 8) | obj.code[ip+4];
    ip += 5;
    stack.push(Variant(obj.strtable[val]));
}

void Executor::exec_PUSHAG()
{
    size_t addr = (obj.code[ip+1] << 24) | (obj.code[ip+2] << 16) | (obj.code[ip+3] << 8) | obj.code[ip+4];
    ip += 5;
    if (globals.size() < addr+1) {
        globals.resize(addr+1);
    }
    stack.push(Variant(&globals.at(addr)));
}

void Executor::exec_PUSHAL()
{
    size_t addr = (obj.code[ip+1] << 24) | (obj.code[ip+2] << 16) | (obj.code[ip+3] << 8) | obj.code[ip+4];
    ip += 5;
    stack.push(Variant(&frames.back().locals.at(addr)));
}

void Executor::exec_LOADB()
{
    ip++;
    Variant *addr = stack.top().address_value; stack.pop();
    stack.push(Variant(addr->boolean_value));
}

void Executor::exec_LOADN()
{
    ip++;
    Variant *addr = stack.top().address_value; stack.pop();
    stack.push(Variant(addr->number_value));
}

void Executor::exec_LOADS()
{
    ip++;
    Variant *addr = stack.top().address_value; stack.pop();
    stack.push(Variant(addr->string_value));
}

void Executor::exec_STOREB()
{
    ip++;
    Variant *addr = stack.top().address_value; stack.pop();
    bool val = stack.top().boolean_value; stack.pop();
    *addr = Variant(val);
}

void Executor::exec_STOREN()
{
    ip++;
    Variant *addr = stack.top().address_value; stack.pop();
    Number val = stack.top().number_value; stack.pop();
    *addr = Variant(val);
}

void Executor::exec_STORES()
{
    ip++;
    Variant *addr = stack.top().address_value; stack.pop();
    std::string val = stack.top().string_value; stack.pop();
    *addr = Variant(val);
}

void Executor::exec_NEGN()
{
    ip++;
    Number x = stack.top().number_value; stack.pop();
    stack.push(Variant(number_negate(x)));
}

void Executor::exec_ADDN()
{
    ip++;
    Number b = stack.top().number_value; stack.pop();
    Number a = stack.top().number_value; stack.pop();
    stack.push(Variant(number_add(a, b)));
}

void Executor::exec_SUBN()
{
    ip++;
    Number b = stack.top().number_value; stack.pop();
    Number a = stack.top().number_value; stack.pop();
    stack.push(Variant(number_subtract(a, b)));
}

void Executor::exec_MULN()
{
    ip++;
    Number b = stack.top().number_value; stack.pop();
    Number a = stack.top().number_value; stack.pop();
    stack.push(Variant(number_multiply(a, b)));
}

void Executor::exec_DIVN()
{
    ip++;
    Number b = stack.top().number_value; stack.pop();
    Number a = stack.top().number_value; stack.pop();
    stack.push(Variant(number_divide(a, b)));
}

void Executor::exec_MODN()
{
    ip++;
    Number b = stack.top().number_value; stack.pop();
    Number a = stack.top().number_value; stack.pop();
    stack.push(Variant(number_modulo(a, b)));
}

void Executor::exec_EXPN()
{
    ip++;
    Number b = stack.top().number_value; stack.pop();
    Number a = stack.top().number_value; stack.pop();
    stack.push(Variant(number_pow(a, b)));
}

void Executor::exec_EQN()
{
    ip++;
    Number b = stack.top().number_value; stack.pop();
    Number a = stack.top().number_value; stack.pop();
    stack.push(Variant(number_is_equal(a, b)));
}

void Executor::exec_NEN()
{
    ip++;
    Number b = stack.top().number_value; stack.pop();
    Number a = stack.top().number_value; stack.pop();
    stack.push(Variant(number_is_not_equal(a, b)));
}

void Executor::exec_LTN()
{
    ip++;
    Number b = stack.top().number_value; stack.pop();
    Number a = stack.top().number_value; stack.pop();
    stack.push(Variant(number_is_less(a, b)));
}

void Executor::exec_GTN()
{
    ip++;
    Number b = stack.top().number_value; stack.pop();
    Number a = stack.top().number_value; stack.pop();
    stack.push(Variant(number_is_greater(a, b)));
}

void Executor::exec_LEN()
{
    ip++;
    Number b = stack.top().number_value; stack.pop();
    Number a = stack.top().number_value; stack.pop();
    stack.push(Variant(number_is_less_equal(a, b)));
}

void Executor::exec_GEN()
{
    ip++;
    Number b = stack.top().number_value; stack.pop();
    Number a = stack.top().number_value; stack.pop();
    stack.push(Variant(number_is_greater_equal(a, b)));
}

void Executor::exec_EQS()
{
    ip++;
    std::string b = stack.top().string_value; stack.pop();
    std::string a = stack.top().string_value; stack.pop();
    stack.push(Variant(a == b));
}

void Executor::exec_NES()
{
    ip++;
    std::string b = stack.top().string_value; stack.pop();
    std::string a = stack.top().string_value; stack.pop();
    stack.push(Variant(a != b));
}

void Executor::exec_LTS()
{
    ip++;
    std::string b = stack.top().string_value; stack.pop();
    std::string a = stack.top().string_value; stack.pop();
    stack.push(Variant(a < b));
}

void Executor::exec_GTS()
{
    ip++;
    std::string b = stack.top().string_value; stack.pop();
    std::string a = stack.top().string_value; stack.pop();
    stack.push(Variant(a > b));
}

void Executor::exec_LES()
{
    ip++;
    std::string b = stack.top().string_value; stack.pop();
    std::string a = stack.top().string_value; stack.pop();
    stack.push(Variant(a <= b));
}

void Executor::exec_GES()
{
    ip++;
    std::string b = stack.top().string_value; stack.pop();
    std::string a = stack.top().string_value; stack.pop();
    stack.push(Variant(a >= b));
}

void Executor::exec_ANDB()
{
    ip++;
    bool b = stack.top().boolean_value; stack.pop();
    bool a = stack.top().boolean_value; stack.pop();
    stack.push(Variant(a && b));
}

void Executor::exec_ORB()
{
    ip++;
    bool b = stack.top().boolean_value; stack.pop();
    bool a = stack.top().boolean_value; stack.pop();
    stack.push(Variant(a || b));
}

void Executor::exec_NOTB()
{
    ip++;
    bool x = stack.top().boolean_value; stack.pop();
    stack.push(Variant(not x));
}

void Executor::exec_INDEXA()
{
    ip++;
    Number index = stack.top().number_value; stack.pop();
    Variant *addr = stack.top().address_value; stack.pop();
    assert(number_is_integer(index));
    uint32_t i = number_to_uint32(index); // TODO: to signed instead of unsigned for better errors
    // TODO: check for i >= 0 and throw exception if not
    if (i >= addr->array_value.size()) {
        addr->array_value.resize(i+1);
    }
    assert(i < addr->array_value.size());
    stack.push(Variant(&addr->array_value.at(i)));
}

void Executor::exec_INDEXD()
{
    ip++;
    std::string index = stack.top().string_value; stack.pop();
    Variant *addr = stack.top().address_value; stack.pop();
    stack.push(Variant(&addr->dictionary_value[index]));
}

void Executor::exec_CALLP()
{
    size_t val = (obj.code[ip+1] << 24) | (obj.code[ip+2] << 16) | (obj.code[ip+3] << 8) | obj.code[ip+4];
    ip += 5;
    std::string func = obj.strtable.at(val);
    rtl_call(stack, func);
}

void Executor::exec_CALLF()
{
    size_t val = (obj.code[ip+1] << 24) | (obj.code[ip+2] << 16) | (obj.code[ip+3] << 8) | obj.code[ip+4];
    ip += 5;
    callstack.push(ip);
    ip = val;
}

void Executor::exec_JUMP()
{
    int target = (obj.code[ip+1] << 24) | (obj.code[ip+2] << 16) | (obj.code[ip+3] << 8) | obj.code[ip+4];
    ip += 5;
    ip = target;
}

void Executor::exec_JF()
{
    int target = (obj.code[ip+1] << 24) | (obj.code[ip+2] << 16) | (obj.code[ip+3] << 8) | obj.code[ip+4];
    ip += 5;
    bool a = stack.top().boolean_value; stack.pop();
    if (not a) {
        ip = target;
    }
}

void Executor::exec_RET()
{
    ip = callstack.top();
    callstack.pop();
}

void Executor::exec()
{
    callstack.push(obj.code.size());
    while (not callstack.empty() && ip < obj.code.size()) {
        //std::cerr << "ip " << ip << " op " << (int)obj.code[ip] << "\n";
        auto last_ip = ip;
        switch (static_cast<Opcode>(obj.code[ip])) {
            case ENTER:   exec_ENTER(); break;
            case LEAVE:   exec_LEAVE(); break;
            case PUSHB:   exec_PUSHB(); break;
            case PUSHN:   exec_PUSHN(); break;
            case PUSHS:   exec_PUSHS(); break;
            case PUSHAG:  exec_PUSHAG(); break;
            case PUSHAL:  exec_PUSHAL(); break;
            case LOADB:   exec_LOADB(); break;
            case LOADN:   exec_LOADN(); break;
            case LOADS:   exec_LOADS(); break;
            case STOREB:  exec_STOREB(); break;
            case STOREN:  exec_STOREN(); break;
            case STORES:  exec_STORES(); break;
            case NEGN:    exec_NEGN(); break;
            case ADDN:    exec_ADDN(); break;
            case SUBN:    exec_SUBN(); break;
            case MULN:    exec_MULN(); break;
            case DIVN:    exec_DIVN(); break;
            case MODN:    exec_MODN(); break;
            case EXPN:    exec_EXPN(); break;
            case EQN:     exec_EQN(); break;
            case NEN:     exec_NEN(); break;
            case LTN:     exec_LTN(); break;
            case GTN:     exec_GTN(); break;
            case LEN:     exec_LEN(); break;
            case GEN:     exec_GEN(); break;
            case EQS:     exec_EQS(); break;
            case NES:     exec_NES(); break;
            case LTS:     exec_LTS(); break;
            case GTS:     exec_GTS(); break;
            case LES:     exec_LES(); break;
            case GES:     exec_GES(); break;
            case ANDB:    exec_ANDB(); break;
            case ORB:     exec_ORB(); break;
            case NOTB:    exec_NOTB(); break;
            case INDEXA:  exec_INDEXA(); break;
            case INDEXD:  exec_INDEXD(); break;
            case CALLP:   exec_CALLP(); break;
            case CALLF:   exec_CALLF(); break;
            case JUMP:    exec_JUMP(); break;
            case JF:      exec_JF(); break;
            case RET:     exec_RET(); break;
        }
        if (ip == last_ip) {
            fprintf(stderr, "exec: Unexpected opcode: %d\n", obj.code[ip]);
            abort();
        }
    }
}

void exec(const Bytecode::bytecode &obj)
{
    Executor(obj).exec();
}
