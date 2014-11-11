#include "parser.h"

#include <iso646.h>

#include "ast.h"
#include "rtl.h"
#include "util.h"

static Scope *g_GlobalScope; // TODO: sort of a hack, all this should probably be in a class

static const Type *parseType(Scope *scope, const std::vector<Token> &tokens, std::vector<Token>::size_type &i, bool allow_nothing = false);
static const Expression *parseExpression(Scope *scope, const std::vector<Token> &tokens, std::vector<Token>::size_type &i);
static const VariableReference *parseVariableReference(Scope *scope, const std::vector<Token> &tokens, std::vector<Token>::size_type &i);

static ComparisonExpression::Comparison comparisonFromToken(const Token &token)
{
    switch (token.type) {
        case EQUAL:     return ComparisonExpression::EQ;
        case NOTEQUAL:  return ComparisonExpression::NE;
        case LESS:      return ComparisonExpression::LT;
        case GREATER:   return ComparisonExpression::GT;
        case LESSEQ:    return ComparisonExpression::LE;
        case GREATEREQ: return ComparisonExpression::GE;
        default:
            error(2000, token, "internal error");
    }
}

StringReference::StringReference(const VariableReference *str, const Expression *index)
  : VariableReference(TYPE_STRING, false),
    str(str),
    index(index),
    load(nullptr),
    store(nullptr)
{
    {
        std::vector<const Expression *> args;
        args.push_back(new VariableExpression(str));
        args.push_back(index);
        args.push_back(new ConstantNumberExpression(number_from_uint32(1)));
        load = new FunctionCall(new ScalarVariableReference(dynamic_cast<const Variable *>(g_GlobalScope->lookupName("substring"))), args);
    }
    {
        std::vector<const Expression *> args;
        args.push_back(new VariableExpression(str));
        args.push_back(index);
        args.push_back(new ConstantNumberExpression(number_from_uint32(1)));
        store = new FunctionCall(new ScalarVariableReference(dynamic_cast<const Variable *>(g_GlobalScope->lookupName("splice"))), args);
    }
}

static const Type *parseArrayType(Scope *scope, const std::vector<Token> &tokens, std::vector<Token>::size_type &i)
{
    if (tokens[i].type != ARRAY) {
        error(2001, tokens[i], "Array expected");
    }
    i++;
    if (tokens[i].type != LESS) {
        error(2002, tokens[i], "'<' expected");
    }
    i++;
    const Type *elementtype = parseType(scope, tokens, i);
    if (tokens[i].type != GREATER) {
        error(2003, tokens[i], "'>' expected");
    }
    i++;
    return new TypeArray(elementtype);
}

static const Type *parseDictionaryType(Scope *scope, const std::vector<Token> &tokens, std::vector<Token>::size_type &i)
{
    if (tokens[i].type != DICTIONARY) {
        error(2004, tokens[i], "Dictionary expected");
    }
    i++;
    if (tokens[i].type != LESS) {
        error(2005, tokens[i], "'<' expected");
    }
    i++;
    const Type *elementtype = parseType(scope, tokens, i);
    if (tokens[i].type != GREATER) {
        error(2006, tokens[i], "'>' expected");
    }
    i++;
    return new TypeDictionary(elementtype);
}

static const Type *parseRecordType(Scope *scope, const std::vector<Token> &tokens, std::vector<Token>::size_type &i)
{
    if (tokens[i].type != RECORD) {
        error(2007, tokens[i], "RECORD expected");
    }
    i++;
    std::map<std::string, std::pair<int, const Type *> > fields;
    int index = 0;
    while (tokens[i].type != END) {
        if (tokens[i].type != IDENTIFIER) {
            error(2008, tokens[i], "identifier expected");
        }
        std::string name = tokens[i].text;
        if (fields.find(name) != fields.end()) {
            error(2009, tokens[i], "duplicate field: " + name);
        }
        ++i;
        if (tokens[i].type != COLON) {
            error(2010, tokens[i], "colon expected");
        }
        ++i;
        const Type *t = parseType(scope, tokens, i);
        fields[name] = std::make_pair(index, t);
        index++;
    }
    i++;
    if (tokens[i].type != RECORD) {
        error(2100, tokens[i], "'RECORD' expected");
    }
    i++;
    return new TypeRecord(fields);
}

static const Type *parseEnumType(Scope *scope, const std::vector<Token> &tokens, std::vector<Token>::size_type &i)
{
    if (tokens[i].type != ENUM) {
        error(2011, tokens[i], "ENUM expected");
    }
    i++;
    std::map<std::string, int> names;
    int index = 0;
    while (tokens[i].type != END) {
        if (tokens[i].type != IDENTIFIER) {
            error(2012, tokens[i], "identifier expected");
        }
        std::string name = tokens[i].text;
        if (names.find(name) != names.end()) {
            error(2013, tokens[i], "duplicate enum: " + name);
        }
        i++;
        names[name] = index;
        index++;
    }
    i++;
    if (tokens[i].type != ENUM) {
        error(2101, tokens[i], "'ENUM' expected");
    }
    i++;
    return new TypeEnum(names);
}

static const Type *parseType(Scope *scope, const std::vector<Token> &tokens, std::vector<Token>::size_type &i, bool allow_nothing)
{
    if (tokens[i].type == ARRAY) {
        return parseArrayType(scope, tokens, i);
    }
    if (tokens[i].type == DICTIONARY) {
        return parseDictionaryType(scope, tokens, i);
    }
    if (tokens[i].type == RECORD) {
        return parseRecordType(scope, tokens, i);
    }
    if (tokens[i].type == ENUM) {
        return parseEnumType(scope, tokens, i);
    }
    if (tokens[i].type != IDENTIFIER) {
        error(2014, tokens[i], "identifier expected");
    }
    const Name *name = scope->lookupName(tokens[i].text);
    if (name == nullptr) {
        error(2015, tokens[i], "unknown type name");
    }
    const Type *type = dynamic_cast<const Type *>(name);
    if (type == nullptr) {
        error(2016, tokens[i], "name is not a type");
    }
    if (not allow_nothing && type == TYPE_NOTHING) {
        error(2017, tokens[i], "'Nothing' type is not allowed here");
    }
    i++;
    return type;
}

static const Statement *parseTypeDefinition(Scope *scope, const std::vector<Token> &tokens, std::vector<Token>::size_type &i)
{
    ++i;
    if (tokens[i].type != IDENTIFIER) {
        error(2018, tokens[i], "identifier expected");
    }
    std::string name = tokens[i].text;
    if (scope->lookupName(name) != nullptr) {
        error(2019, tokens[i], "name shadows outer");
    }
    ++i;
    if (tokens[i].type != ASSIGN) {
        error(2020, tokens[i], "':=' expected");
    }
    ++i;
    const Type *type = parseType(scope, tokens, i);
    scope->addName(name, const_cast<Type *>(type)); // TODO clean up when 'referenced' is fixed
    return nullptr;
}

static const Statement *parseConstantDefinition(Scope *scope, const std::vector<Token> &tokens, std::vector<Token>::size_type &i)
{
    ++i;
    if (tokens[i].type != IDENTIFIER) {
        error(2021, tokens[i], "identifier expected");
    }
    std::string name = tokens[i].text;
    if (scope->lookupName(name) != nullptr) {
        error(2022, tokens[i], "name shadows outer");
    }
    ++i;
    if (tokens[i].type != COLON) {
        error(2023, tokens[i], "':' expected");
    }
    ++i;
    const Type *type = parseType(scope, tokens, i);
    if (tokens[i].type != ASSIGN) {
        error(2024, tokens[i], "':=' expected");
    }
    ++i;
    const Expression *value = parseExpression(scope, tokens, i);
    if (not value->type->is_equivalent(type)) {
        error(2025, tokens[i], "type mismatch");
    }
    if (not value->is_constant) {
        error(2026, tokens[i], "value must be constant");
    }
    scope->addName(name, new Constant(name, value));
    return nullptr;
}

static const FunctionCall *parseFunctionCall(const VariableReference *ref, Scope *scope, const std::vector<Token> &tokens, std::vector<Token>::size_type &i)
{
    const TypeFunction *ftype = dynamic_cast<const TypeFunction *>(ref->type);
    if (ftype == nullptr) {
        error(2027, tokens[i-1], "not a function");
    }
    ++i;
    std::vector<const Type *>::size_type p = 0;
    std::vector<const Expression *> args;
    if (tokens[i].type != RPAREN) {
        for (;;) {
            const Expression *e = parseExpression(scope, tokens, i);
            if (ftype->params[p]->mode != ParameterType::IN) {
                const VariableReference *ref = e->get_reference();
                if (ref == nullptr) {
                    error(2028, tokens[i], "function call argument must be reference: " + e->text());
                }
            }
            if (not e->type->is_equivalent(ftype->params[p]->type)) {
                error(2029, tokens[i], "type mismatch");
            }
            args.push_back(e);
            ++p;
            if (tokens[i].type != COMMA) {
                break;
            }
            ++i;
        }
        if (tokens[i].type != RPAREN) {
            error(2030, tokens[i], "')' or ',' expected");
        }
    }
    if (p < ftype->params.size()) {
        error(2031, tokens[i], "not enough arguments");
    }
    ++i;
    return new FunctionCall(ref, args);
}

/*
 * Operator precedence:
 *
 *  ^        exponentiation                     parseExponentiation
 *  * / MOD  multiplication, division, modulo   parseMultiplication
 *  + -      addition, subtraction              parseAddition
 *  < = >    comparison                         parseComparison
 *  and      conjunction                        parseConjunction
 *  or       disjunction                        parseDisjunction
 *  if       conditional                        parseConditional
 */

static const Expression *parseAtom(Scope *scope, const std::vector<Token> &tokens, std::vector<Token>::size_type &i)
{
    switch (tokens[i].type) {
        case LPAREN: {
            ++i;
            const Expression *expr = parseExpression(scope, tokens, i);
            if (tokens[i].type != RPAREN) {
                error(2032, tokens[i], ") expected");
            }
            ++i;
            return expr;
        }
        case FALSE: {
            ++i;
            return new ConstantBooleanExpression(false);
        }
        case TRUE: {
            ++i;
            return new ConstantBooleanExpression(true);
        }
        case NUMBER: {
            return new ConstantNumberExpression(tokens[i++].value);
        }
        case STRING: {
            return new ConstantStringExpression(tokens[i++].text);
        }
        case MINUS: {
            auto op = i;
            ++i;
            const Expression *factor = parseAtom(scope, tokens, i);
            if (not factor->type->is_equivalent(TYPE_NUMBER)) {
                error(2033, tokens[op], "number required for negation");
            }
            return new UnaryMinusExpression(factor);
        }
        case NOT: {
            auto op = i;
            ++i;
            const Expression *atom = parseAtom(scope, tokens, i);
            if (not atom->type->is_equivalent(TYPE_BOOLEAN)) {
                error(2034, tokens[op], "boolean required for logical not");
            }
            return new LogicalNotExpression(atom);
        }
        case IDENTIFIER: {
            const TypeEnum *enumtype = dynamic_cast<const TypeEnum *>(scope->lookupName(tokens[i].text));
            if (enumtype != nullptr) {
                ++i;
                if (tokens[i].type != DOT) {
                    error(2035, tokens[i], "'.' expected");
                }
                ++i;
                if (tokens[i].type != IDENTIFIER) {
                    error(2036, tokens[i], "identifier expected");
                }
                auto name = enumtype->names.find(tokens[i].text);
                if (name == enumtype->names.end()) {
                    error(2037, tokens[i], "identifier not member of enum: " + tokens[i].text);
                }
                ++i;
                return new ConstantEnumExpression(enumtype, name->second);
            } else {
                const VariableReference *ref = parseVariableReference(scope, tokens, i);
                if (tokens[i].type == LPAREN) {
                    return parseFunctionCall(ref, scope, tokens, i);
                } else {
                    return new VariableExpression(ref);
                }
            }
        }
        default:
            error(2038, tokens[i], "Expression expected");
    }
}

static const Expression *parseExponentiation(Scope *scope, const std::vector<Token> &tokens, std::vector<Token>::size_type &i)
{
    const Expression *left = parseAtom(scope, tokens, i);
    for (;;) {
        if (tokens[i].type == EXP) {
            auto op = i;
            ++i;
            const Expression *right = parseAtom(scope, tokens, i);
            if (left->type->is_equivalent(TYPE_NUMBER) && right->type->is_equivalent(TYPE_NUMBER)) {
                left = new ExponentiationExpression(left, right);
            } else {
                error(2039, tokens[op], "type mismatch");
            }
        } else {
            return left;
        }
    }
}

static const Expression *parseMultiplication(Scope *scope, const std::vector<Token> &tokens, std::vector<Token>::size_type &i)
{
    const Expression *left = parseExponentiation(scope, tokens, i);
    for (;;) {
        switch (tokens[i].type) {
            case TIMES: {
                auto op = i;
                ++i;
                const Expression *right = parseExponentiation(scope, tokens, i);
                if (left->type->is_equivalent(TYPE_NUMBER) && right->type->is_equivalent(TYPE_NUMBER)) {
                    left = new MultiplicationExpression(left, right);
                } else {
                    error(2040, tokens[op], "type mismatch");
                }
                break;
            }
            case DIVIDE: {
                auto op = i;
                ++i;
                const Expression *right = parseExponentiation(scope, tokens, i);
                if (left->type->is_equivalent(TYPE_NUMBER) && right->type->is_equivalent(TYPE_NUMBER)) {
                    left = new DivisionExpression(left, right);
                } else {
                    error(2041, tokens[op], "type mismatch");
                }
                break;
            }
            case MOD: {
                auto op = i;
                ++i;
                const Expression *right = parseExponentiation(scope, tokens, i);
                if (left->type->is_equivalent(TYPE_NUMBER) && right->type->is_equivalent(TYPE_NUMBER)) {
                    left = new ModuloExpression(left, right);
                } else {
                    error(2042, tokens[op], "type mismatch");
                }
                break;
            }
            default:
                return left;
        }
    }
}

static const Expression *parseAddition(Scope *scope, const std::vector<Token> &tokens, std::vector<Token>::size_type &i)
{
    const Expression *left = parseMultiplication(scope, tokens, i);
    for (;;) {
        switch (tokens[i].type) {
            case PLUS: {
                auto op = i;
                ++i;
                const Expression *right = parseMultiplication(scope, tokens, i);
                if (left->type->is_equivalent(TYPE_NUMBER) && right->type->is_equivalent(TYPE_NUMBER)) {
                    left = new AdditionExpression(left, right);
                } else if (left->type->is_equivalent(TYPE_STRING) && right->type->is_equivalent(TYPE_STRING)) {
                    std::vector<const Expression *> args;
                    args.push_back(left);
                    args.push_back(right);
                    left = new FunctionCall(new ScalarVariableReference(dynamic_cast<const Variable *>(scope->lookupName("concat"))), args);
                } else {
                    error(2043, tokens[op], "type mismatch");
                }
                break;
            }
            case MINUS: {
                auto op = i;
                ++i;
                const Expression *right = parseMultiplication(scope, tokens, i);
                if (left->type->is_equivalent(TYPE_NUMBER) && right->type->is_equivalent(TYPE_NUMBER)) {
                    left = new SubtractionExpression(left, right);
                } else {
                    error(2044, tokens[op], "type mismatch");
                }
                break;
            }
            default:
                return left;
        }
    }
}

static const Expression *parseComparison(Scope *scope, const std::vector<Token> &tokens, std::vector<Token>::size_type &i)
{
    const Expression *left = parseAddition(scope, tokens, i);
    switch (tokens[i].type) {
        case EQUAL:
        case NOTEQUAL:
        case LESS:
        case GREATER:
        case LESSEQ:
        case GREATEREQ: {
            ComparisonExpression::Comparison comp = comparisonFromToken(tokens[i]);
            auto op = i;
            ++i;
            const Expression *right = parseAddition(scope, tokens, i);
            if (not left->type->is_equivalent(right->type)) {
                error(2045, tokens[op], "type mismatch");
            }
            if (left->type->is_equivalent(TYPE_BOOLEAN)) {
                if (comp != ComparisonExpression::EQ && comp != ComparisonExpression::NE) {
                    error(2046, tokens[op], "comparison not available for Boolean");
                }
                return new BooleanComparisonExpression(left, right, comp);
            } else if (left->type->is_equivalent(TYPE_NUMBER)) {
                return new NumericComparisonExpression(left, right, comp);
            } else if (left->type->is_equivalent(TYPE_STRING)) {
                return new StringComparisonExpression(left, right, comp);
            } else if (dynamic_cast<const TypeArray *>(left->type) != nullptr) {
                if (comp != ComparisonExpression::EQ && comp != ComparisonExpression::NE) {
                    error(2047, tokens[op], "comparison not available for Array");
                }
                return new ArrayComparisonExpression(left, right, comp);
            } else if (dynamic_cast<const TypeDictionary *>(left->type) != nullptr) {
                if (comp != ComparisonExpression::EQ && comp != ComparisonExpression::NE) {
                    error(2048, tokens[op], "comparison not available for Dictionary");
                }
                return new DictionaryComparisonExpression(left, right, comp);
            } else if (dynamic_cast<const TypeRecord *>(left->type) != nullptr) {
                if (comp != ComparisonExpression::EQ && comp != ComparisonExpression::NE) {
                    error(2049, tokens[op], "comparison not available for Array");
                }
                return new ArrayComparisonExpression(left, right, comp);
            } else {
                error(2050, tokens[op], "type mismatch");
            }
        }
        default:
            return left;
    }
}

static const Expression *parseConjunction(Scope *scope, const std::vector<Token> &tokens, std::vector<Token>::size_type &i)
{
    const Expression *left = parseComparison(scope, tokens, i);
    for (;;) {
        if (tokens[i].type == AND) {
            auto op = i;
            ++i;
            const Expression *right = parseComparison(scope, tokens, i);
            if (left->type->is_equivalent(TYPE_BOOLEAN) && right->type->is_equivalent(TYPE_BOOLEAN)) {
                left = new ConjunctionExpression(left, right);
            } else {
                error(2051, tokens[op], "type mismatch");
            }
        } else {
            return left;
        }
    }
}

static const Expression *parseDisjunction(Scope *scope, const std::vector<Token> &tokens, std::vector<Token>::size_type &i)
{
    const Expression *left = parseConjunction(scope, tokens, i);
    for (;;) {
        if (tokens[i].type == OR) {
            auto op = i;
            ++i;
            const Expression *right = parseConjunction(scope, tokens, i);
            if (left->type->is_equivalent(TYPE_BOOLEAN) && right->type->is_equivalent(TYPE_BOOLEAN)) {
                left = new DisjunctionExpression(left, right);
            } else {
                error(2052, tokens[op], "type mismatch");
            }
        } else {
            return left;
        }
    }
}

static const Expression *parseConditional(Scope *scope, const std::vector<Token> &tokens, std::vector<Token>::size_type &i)
{
    if (tokens[i].type == IF) {
        ++i;
        const Expression *cond = parseExpression(scope, tokens, i);
        if (tokens[i].type != THEN) {
            error(2053, tokens[i], "'THEN' expected");
        }
        ++i;
        const Expression *left = parseExpression(scope, tokens, i);
        if (tokens[i].type != ELSE) {
            error(2054, tokens[i], "'ELSE' expected");
        }
        ++i;
        const Expression *right = parseExpression(scope, tokens, i);
        if (not left->type->is_equivalent(right->type)) {
            error(2055, tokens[i], "type of THEN and ELSE must match");
        }
        return new ConditionalExpression(cond, left, right);
    } else {
        return parseDisjunction(scope, tokens, i);
    }
}

static const Expression *parseExpression(Scope *scope, const std::vector<Token> &tokens, std::vector<Token>::size_type &i)
{
    return parseConditional(scope, tokens, i);
}

typedef std::pair<std::vector<std::string>, const Type *> VariableInfo;

static const VariableInfo parseVariableDeclaration(Scope *scope, const std::vector<Token> &tokens, std::vector<Token>::size_type &i)
{
    std::vector<std::string> names;
    for (;;) {
        if (tokens[i].type != IDENTIFIER) {
            error(2056, tokens[i], "identifier expected");
        }
        std::string name = tokens[i].text;
        if (scope->lookupName(name) != nullptr) {
            error(2057, tokens[i], "name shadows outer");
        }
        ++i;
        names.push_back(name);
        if (tokens[i].type != COMMA) {
            break;
        }
        ++i;
    }
    if (tokens[i].type != COLON) {
        error(2058, tokens[i], "colon expected");
    }
    ++i;
    const Type *t = parseType(scope, tokens, i);
    return make_pair(names, t);
}

static const VariableReference *parseVariableReference(Scope *scope, const std::vector<Token> &tokens, std::vector<Token>::size_type &i)
{
    if (tokens[i].type == IDENTIFIER) {
        const Name *name = scope->lookupName(tokens[i].text);
        if (name == nullptr) {
            error(2059, tokens[i], "name not found: " + tokens[i].text);
        }
        const Constant *cons = dynamic_cast<const Constant *>(name);
        if (cons != nullptr) {
            ++i;
            return new ConstantReference(cons);
        }
        const Module *module = dynamic_cast<const Module *>(name);
        if (module != nullptr) {
            ++i;
            if (tokens[i].type != DOT) {
                error(2060, tokens[i], "'.' expected");
            }
            ++i;
            return parseVariableReference(module->scope, tokens, i);
        }
        const Variable *var = dynamic_cast<const Variable *>(name);
        if (var == nullptr) {
            error(2061, tokens[i], "name is not a variable: " + tokens[i].text);
        }
        const VariableReference *ref = new ScalarVariableReference(var);
        const Type *type = var->type;
        ++i;
        for (;;) {
            if (tokens[i].type == LBRACKET) {
                const TypeArray *arraytype = dynamic_cast<const TypeArray *>(type);
                const TypeDictionary *dicttype = dynamic_cast<const TypeDictionary *>(type);
                if (arraytype != nullptr) {
                    ++i;
                    const Expression *index = parseExpression(scope, tokens, i);
                    if (not index->type->is_equivalent(TYPE_NUMBER)) {
                        error(2062, tokens[i], "index must be a number");
                    }
                    if (tokens[i].type != RBRACKET) {
                        error(2063, tokens[i], "']' expected");
                    }
                    ++i;
                    type = arraytype->elementtype;
                    ref = new ArrayReference(type, ref, index);
                } else if (dicttype != nullptr) {
                    ++i;
                    const Expression *index = parseExpression(scope, tokens, i);
                    if (not index->type->is_equivalent(TYPE_STRING)) {
                        error(2064, tokens[i], "index must be a string");
                    }
                    if (tokens[i].type != RBRACKET) {
                        error(2065, tokens[i], "']' expected");
                    }
                    ++i;
                    type = dicttype->elementtype;
                    ref = new DictionaryReference(type, ref, index);
                } else if (type == TYPE_STRING) {
                    ++i;
                    const Expression *index = parseExpression(scope, tokens, i);
                    if (not index->type->is_equivalent(TYPE_NUMBER)) {
                        error(2066, tokens[i], "index must be a number");
                    }
                    if (tokens[i].type != RBRACKET) {
                        error(2067, tokens[i], "']' expected");
                    }
                    ++i;
                    return new StringReference(ref, index);
                } else {
                    error(2068, tokens[i], "not an array or dictionary");
                }
            } else if (tokens[i].type == DOT) {
                const TypeRecord *recordtype = dynamic_cast<const TypeRecord *>(type);
                if (recordtype != nullptr) {
                    ++i;
                    if (tokens[i].type != IDENTIFIER) {
                        error(2069, tokens[i], "identifier expected");
                    }
                    std::map<std::string, std::pair<int, const Type *> >::const_iterator f = recordtype->fields.find(tokens[i].text);
                    if (f == recordtype->fields.end()) {
                        error(2070, tokens[i], "field not found");
                    }
                    ++i;
                    type = f->second.second;
                    ref = new ArrayReference(type, ref, new ConstantNumberExpression(number_from_uint32(f->second.first)));
                } else {
                    error(2071, tokens[i], "not a record");
                }
            } else {
                break;
            }
        }
        return ref;
    } else {
        error(2072, tokens[i], "Identifier expected");
    }
}

static const Statement *parseStatement(Scope *scope, const std::vector<Token> &tokens, std::vector<Token>::size_type &i);
static const Statement *parseVarStatement(Scope *scope, const std::vector<Token> &tokens, std::vector<Token>::size_type &i);

static const Statement *parseFunctionDefinition(Scope *scope, const std::vector<Token> &tokens, std::vector<Token>::size_type &i)
{
    ++i;
    if (tokens[i].type != IDENTIFIER) {
        error(2073, tokens[i], "identifier expected");
    }
    std::string name = tokens[i].text;
    if (scope->lookupName(name) != nullptr) {
        error(2074, tokens[i], "name shadows outer");
    }
    ++i;
    if (tokens[i].type != LPAREN) {
        error(2075, tokens[i], "'(' expected");
    }
    ++i;
    std::vector<FunctionParameter *> args;
    Scope *newscope = new Scope(scope);
    if (tokens[i].type != RPAREN) {
        for (;;) {
            ParameterType::Mode mode = ParameterType::IN;
            switch (tokens[i].type) {
                case IN:    mode = ParameterType::IN;       i++; break;
                case INOUT: mode = ParameterType::INOUT;    i++; break;
                case OUT:   mode = ParameterType::OUT;      i++; break;
                default:
                    break;
            }
            const VariableInfo vars = parseVariableDeclaration(newscope, tokens, i);
            for (auto name: vars.first) {
                FunctionParameter *fp = new FunctionParameter(name, vars.second, mode, newscope);
                args.push_back(fp);
                newscope->addName(name, fp);
            }
            if (tokens[i].type != COMMA) {
                break;
            }
            ++i;
        }
        if (tokens[i].type != RPAREN) {
            error(2076, tokens[i], "')' or ',' expected");
        }
    }
    ++i;
    if (tokens[i].type != COLON) {
        error(2077, tokens[i], "':' expected");
    }
    ++i;
    const Type *returntype = parseType(newscope, tokens, i, true);
    Function *function = new Function(name, returntype, newscope, args);
    while (tokens[i].type != END) {
        const Statement *s = parseStatement(newscope, tokens, i);
        if (s != nullptr) {
            function->statements.push_back(s);
        }
    }
    ++i;
    if (tokens[i].type != FUNCTION) {
        error(2102, tokens[i], "'FUNCTION' expected");
    }
    ++i;
    scope->addName(name, function);
    return nullptr;
}

static const Statement *parseIfStatement(Scope *scope, const std::vector<Token> &tokens, std::vector<Token>::size_type &i)
{
    std::vector<std::pair<const Expression *, std::vector<const Statement *>>> condition_statements;
    std::vector<const Statement *> else_statements;
    do {
        ++i;
        auto j = i;
        const Expression *cond = parseExpression(scope, tokens, i);
        if (not cond->type->is_equivalent(TYPE_BOOLEAN)) {
            error(2078, tokens[j], "boolean value expected");
        }
        if (tokens[i].type != THEN) {
            error(2079, tokens[i], "THEN expected");
        }
        ++i;
        std::vector<const Statement *> statements;
        while (tokens[i].type != ELSIF && tokens[i].type != ELSE && tokens[i].type != END && tokens[i].type != END_OF_FILE) {
            const Statement *s = parseStatement(scope, tokens, i);
            if (s != nullptr) {
                statements.push_back(s);
            }
        }
        condition_statements.push_back(std::make_pair(cond, statements));
    } while (tokens[i].type == ELSIF);
    if (tokens[i].type == ELSE) {
        ++i;
        while (tokens[i].type != END && tokens[i].type != END_OF_FILE) {
            const Statement *s = parseStatement(scope, tokens, i);
            if (s != nullptr) {
                else_statements.push_back(s);
            }
        }
    }
    if (tokens[i].type != END) {
        error(2080, tokens[i], "END expected");
    }
    ++i;
    if (tokens[i].type != IF) {
        error(2103, tokens[i], "IF expected");
    }
    ++i;
    return new IfStatement(condition_statements, else_statements);
}

static const Statement *parseReturnStatement(Scope *scope, const std::vector<Token> &tokens, std::vector<Token>::size_type &i)
{
    ++i;
    const Expression *expr = parseExpression(scope, tokens, i);
    // TODO: check return type
    return new ReturnStatement(expr);
}

static const Statement *parseVarStatement(Scope *scope, const std::vector<Token> &tokens, std::vector<Token>::size_type &i)
{
    ++i;
    const VariableInfo vars = parseVariableDeclaration(scope, tokens, i);
    for (auto name: vars.first) {
        Variable *v;
        if (scope == g_GlobalScope) {
            v = new GlobalVariable(name, vars.second);
        } else {
            v = new LocalVariable(name, vars.second, scope);
        }
        scope->addName(name, v);
    }
    return nullptr;
}

static const Statement *parseWhileStatement(Scope *scope, const std::vector<Token> &tokens, std::vector<Token>::size_type &i)
{
    ++i;
    auto j = i;
    const Expression *cond = parseExpression(scope, tokens, i);
    if (not cond->type->is_equivalent(TYPE_BOOLEAN)) {
        error(2081, tokens[j], "boolean value expected");
    }
    if (tokens[i].type != DO) {
        error(2082, tokens[i], "DO expected");
    }
    ++i;
    std::vector<const Statement *> statements;
    while (tokens[i].type != END && tokens[i].type != END_OF_FILE) {
        const Statement *s = parseStatement(scope, tokens, i);
        if (s != nullptr) {
            statements.push_back(s);
        }
    }
    if (tokens[i].type != END) {
        error(2083, tokens[i], "END expected");
    }
    ++i;
    if (tokens[i].type != WHILE) {
        error(2104, tokens[i], "WHILE expected");
    }
    ++i;
    return new WhileStatement(cond, statements);
}

static const Statement *parseCaseStatement(Scope *scope, const std::vector<Token> &tokens, std::vector<Token>::size_type &i)
{
    ++i;
    const Expression *expr = parseExpression(scope, tokens, i);
    if (not expr->type->is_equivalent(TYPE_NUMBER) && not expr->type->is_equivalent(TYPE_STRING)) {
        error(2084, tokens[i], "CASE expression must be Number or String");
    }
    std::vector<std::pair<std::vector<const CaseStatement::WhenCondition *>, std::vector<const Statement *>>> clauses;
    while (tokens[i].type == WHEN) {
        std::vector<const CaseStatement::WhenCondition *> conditions;
        do {
            ++i;
            switch (tokens[i].type) {
                case EQUAL:
                case NOTEQUAL:
                case LESS:
                case GREATER:
                case LESSEQ:
                case GREATEREQ: {
                    auto op = tokens[i];
                    ++i;
                    const Expression *when = parseExpression(scope, tokens, i);
                    if (not when->type->is_equivalent(expr->type)) {
                        error(2085, tokens[i], "type mismatch");
                    }
                    if (not when->is_constant) {
                        error(2086, tokens[i], "WHEN condition must be constant");
                    }
                    const CaseStatement::WhenCondition *cond = new CaseStatement::ComparisonWhenCondition(comparisonFromToken(op), when);
                    for (auto clause: clauses) {
                        for (auto c: clause.first) {
                            if (cond->overlaps(c)) {
                                error(2106, tokens[i], "overlapping case condition");
                            }
                        }
                    }
                    for (auto c: conditions) {
                        if (cond->overlaps(c)) {
                            error(2107, tokens[i], "overlapping case condition");
                        }
                    }
                    conditions.push_back(cond);
                    break;
                }
                default: {
                    const Expression *when = parseExpression(scope, tokens, i);
                    if (not when->type->is_equivalent(expr->type)) {
                        error(2087, tokens[i], "type mismatch");
                    }
                    if (not when->is_constant) {
                        error(2088, tokens[i], "WHEN condition must be constant");
                    }
                    if (tokens[i].type == DOTDOT) {
                        ++i;
                        const Expression *when2 = parseExpression(scope, tokens, i);
                        if (not when2->type->is_equivalent(expr->type)) {
                            error(2089, tokens[i], "type mismatch");
                        }
                        if (not when2->is_constant) {
                            error(2090, tokens[i], "WHEN condition must be constant");
                        }
                        const CaseStatement::WhenCondition *cond = new CaseStatement::RangeWhenCondition(when, when2);
                        for (auto clause: clauses) {
                            for (auto c: clause.first) {
                                if (cond->overlaps(c)) {
                                    error(2108, tokens[i], "overlapping case condition");
                                }
                            }
                        }
                        for (auto c: conditions) {
                            if (cond->overlaps(c)) {
                                error(2109, tokens[i], "overlapping case condition");
                            }
                        }
                        conditions.push_back(cond);
                    } else {
                        const CaseStatement::WhenCondition *cond = new CaseStatement::ComparisonWhenCondition(ComparisonExpression::EQ, when);
                        for (auto clause: clauses) {
                            for (auto c: clause.first) {
                                if (cond->overlaps(c)) {
                                    error(2110, tokens[i], "overlapping case condition");
                                }
                            }
                        }
                        for (auto c: conditions) {
                            if (cond->overlaps(c)) {
                                error(2111, tokens[i], "overlapping case condition");
                            }
                        }
                        conditions.push_back(cond);
                    }
                    break;
                }
            }
        } while (tokens[i].type == COMMA);
        if (tokens[i].type != DO) {
            error(2091, tokens[i], "'DO' expected");
        }
        ++i;
        std::vector<const Statement *> statements;
        while (tokens[i].type != WHEN && tokens[i].type != ELSE && tokens[i].type != END) {
            const Statement *stmt = parseStatement(scope, tokens, i);
            statements.push_back(stmt);
        }
        clauses.push_back(std::make_pair(conditions, statements));
    }
    std::vector<const Statement *> else_statements;
    if (tokens[i].type == ELSE) {
        ++i;
        while (tokens[i].type != END) {
            const Statement *stmt = parseStatement(scope, tokens, i);
            else_statements.push_back(stmt);
        }
    }
    if (tokens[i].type != END) {
        error(2092, tokens[i], "'END' expected");
    }
    ++i;
    if (tokens[i].type != CASE) {
        error(2105, tokens[i], "CASE expected");
    }
    ++i;
    clauses.push_back(std::make_pair(std::vector<const CaseStatement::WhenCondition *>(), else_statements));
    return new CaseStatement(expr, clauses);
}

namespace overlap {

static bool operator==(const Number &x, const Number &y) { return number_is_equal(x, y); }
static bool operator!=(const Number &x, const Number &y) { return number_is_not_equal(x, y); }
static bool operator<(const Number &x, const Number &y) { return number_is_less(x, y); }
static bool operator>(const Number &x, const Number &y) { return number_is_greater(x, y); }
static bool operator<=(const Number &x, const Number &y) { return number_is_less_equal(x, y); }
static bool operator>=(const Number &x, const Number &y) { return number_is_greater_equal(x, y); }

template <typename T> bool check(ComparisonExpression::Comparison comp1, const T &value1, ComparisonExpression::Comparison comp2, const T &value2)
{
    switch (comp1) {
        case ComparisonExpression::EQ:
            switch (comp2) {
                case ComparisonExpression::EQ:
                    return value1 == value2;
                case ComparisonExpression::NE:
                    return value1 != value2;
                case ComparisonExpression::LT:
                    return value1 < value2;
                case ComparisonExpression::GT:
                    return value1 > value2;
                case ComparisonExpression::LE:
                    return value1 <= value2;
                case ComparisonExpression::GE:
                    return value1 >= value2;
            }
            break;
        case ComparisonExpression::NE:
            return false; // TODO
        case ComparisonExpression::LT:
            return false; // TODO
        case ComparisonExpression::GT:
            return false; // TODO
        case ComparisonExpression::LE:
            return false; // TODO
        case ComparisonExpression::GE:
            return false; // TODO
    }
    return false;
}

template <typename T> bool check(ComparisonExpression::Comparison comp1, const T &value1, const T &value2low, const T &value2high)
{
    return false; // TODO
}

template <typename T> bool check(const T &value1low, const T &value1high, const T &value2low, const T &value2high)
{
    return false; // TODO
}

} // namespace overlap

bool CaseStatement::ComparisonWhenCondition::overlaps(const WhenCondition *cond) const
{
    const ComparisonWhenCondition *cwhen = dynamic_cast<const ComparisonWhenCondition *>(cond);
    const RangeWhenCondition *rwhen = dynamic_cast<const RangeWhenCondition *>(cond);
    if (cwhen != nullptr) {
        if (expr->type->is_equivalent(TYPE_NUMBER)) {
            return overlap::check(comp, expr->eval_number(), cwhen->comp, cwhen->expr->eval_number());
        } else if (expr->type->is_equivalent(TYPE_STRING)) {
            return overlap::check(comp, expr->eval_string(), cwhen->comp, cwhen->expr->eval_string());
        } else {
            internal_error("ComparisonWhenCondition");
        }
    } else if (rwhen != nullptr) {
        if (expr->type->is_equivalent(TYPE_NUMBER)) {
            return overlap::check(comp, expr->eval_number(), rwhen->low_expr->eval_number(), rwhen->high_expr->eval_number());
        } else if (expr->type->is_equivalent(TYPE_STRING)) {
            return overlap::check(comp, expr->eval_string(), rwhen->low_expr->eval_string(), rwhen->high_expr->eval_string());
        } else {
            internal_error("ComparisonWhenCondition");
        }
    } else {
        internal_error("ComparisonWhenCondition");
    }
}

bool CaseStatement::RangeWhenCondition::overlaps(const WhenCondition *cond) const
{
    const ComparisonWhenCondition *cwhen = dynamic_cast<const ComparisonWhenCondition *>(cond);
    const RangeWhenCondition *rwhen = dynamic_cast<const RangeWhenCondition *>(cond);
    if (cwhen != nullptr) {
        if (low_expr->type->is_equivalent(TYPE_NUMBER)) {
            return overlap::check(cwhen->comp, cwhen->expr->eval_number(), low_expr->eval_number(), high_expr->eval_number());
        } else if (low_expr->type->is_equivalent(TYPE_STRING)) {
            return overlap::check(cwhen->comp, cwhen->expr->eval_string(), low_expr->eval_string(), high_expr->eval_string());
        } else {
            internal_error("RangeWhenCondition");
        }
    } else if (rwhen != nullptr) {
        if (low_expr->type->is_equivalent(TYPE_NUMBER)) {
            return overlap::check(low_expr->eval_number(), high_expr->eval_number(), rwhen->low_expr->eval_number(), rwhen->high_expr->eval_number());
        } else if (low_expr->type->is_equivalent(TYPE_STRING)) {
            return overlap::check(low_expr->eval_string(), high_expr->eval_string(), rwhen->low_expr->eval_string(), rwhen->high_expr->eval_string());
        } else {
            internal_error("RangeWhenCondition");
        }
    } else {
        internal_error("RangeWhenCondition");
    }
}

static const Statement *parseImport(Scope *scope, const std::vector<Token> &tokens, std::vector<Token>::size_type &i)
{
    ++i;
    if (tokens[i].type != IDENTIFIER) {
        error(2093, tokens[i], "identifier expected");
    }
    rtl_import(scope, tokens[i].text);
    ++i;
    return nullptr;
}

static const Statement *parseStatement(Scope *scope, const std::vector<Token> &tokens, std::vector<Token>::size_type &i)
{
    if (tokens[i].type == IMPORT) {
        return parseImport(scope, tokens, i);
    } else if (tokens[i].type == TYPE) {
        return parseTypeDefinition(scope, tokens, i);
    } else if (tokens[i].type == CONST) {
        return parseConstantDefinition(scope, tokens, i);
    } else if (tokens[i].type == FUNCTION) {
        return parseFunctionDefinition(scope, tokens, i);
    } else if (tokens[i].type == IF) {
        return parseIfStatement(scope, tokens, i);
    } else if (tokens[i].type == RETURN) {
        return parseReturnStatement(scope, tokens, i);
    } else if (tokens[i].type == VAR) {
        return parseVarStatement(scope, tokens, i);
    } else if (tokens[i].type == WHILE) {
        return parseWhileStatement(scope, tokens, i);
    } else if (tokens[i].type == CASE) {
        return parseCaseStatement(scope, tokens, i);
    } else if (tokens[i].type == IDENTIFIER) {
        const VariableReference *ref = parseVariableReference(scope, tokens, i);
        if (tokens[i].type == ASSIGN) {
            auto op = i;
            ++i;
            const Expression *expr = parseExpression(scope, tokens, i);
            if (not expr->type->is_equivalent(ref->type)) {
                error(2094, tokens[op], "type mismatch");
            }
            if (dynamic_cast<const ConstantReference *>(ref) != nullptr) {
                // TODO: there is probably a better way to detect this.
                error(2095, tokens[op], "name is not a variable");
            }
            return new AssignmentStatement(ref, expr);
        } else if (tokens[i].type == LPAREN) {
            const FunctionCall *fc = parseFunctionCall(ref, scope, tokens, i);
            if (fc->type != TYPE_NOTHING) {
                error(2096, tokens[i], "return value unused");
            }
            return new ExpressionStatement(fc);
        } else if (tokens[i].type == EQUAL) {
            error(2097, tokens[i], "':=' expected");
        } else {
            error(2098, tokens[i], "Unexpected");
        }
    } else {
        error(2099, tokens[i], "Identifier expected");
    }
}

const Program *parse(const std::vector<Token> &tokens)
{
    Program *program = new Program();
    g_GlobalScope = program->scope;
    std::vector<Token>::size_type i = 0;
    while (tokens[i].type != END_OF_FILE) {
        const Statement *s = parseStatement(program->scope, tokens, i);
        if (s != nullptr) {
            program->statements.push_back(s);
        }
    }
    return program;
}
