#include "number.h"

#if defined(NUMBER_DOUBLE)

#include <math.h>

Number number_add(Number x, Number y)
{
    Number r;
    r.x = x.x + y.x;
    return r;
}

Number number_subtract(Number x, Number y)
{
    Number r;
    r.x = x.x - y.x;
    return r;
}

Number number_multiply(Number x, Number y)
{
    Number r;
    r.x = x.x * y.x;
    return r;
}

Number number_divide(Number x, Number y)
{
    Number r;
    r.x = x.x / y.x;
    return r;
}

Number number_negate(Number x)
{
    Number r;
    r.x = - x.x;
    return r;
}

Number number_abs(Number x)
{
    Number r;
    r.x = fabs(x.x);
    return r;
}

bool number_is_zero(Number x)
{
    return x.x == 0;
}

bool number_is_equal(Number x, Number y)
{
    return x.x == y.x;
}

bool number_is_not_equal(Number x, Number y)
{
    return x.x != y.x;
}

bool number_is_less(Number x, Number y)
{
    return x.x < y.x;
}

bool number_is_greater(Number x, Number y)
{
    return x.x > y.x;
}

bool number_is_less_equal(Number x, Number y)
{
    return x.x <= y.x;
}

bool number_is_greater_equal(Number x, Number y)
{
    return x.x >= y.x;
}

std::string number_to_string(Number x)
{
    std::string r = std::to_string(x.x);
    
    // TODO: This hack converts a canonical string like 42.00000 to just 42
    const auto d = r.find('.');
    if (d != std::string::npos) {
        auto i = d + 1;
        while (i < r.length() && r.at(i) == '0') {
            i++;
        }
        if (i >= r.length()) {
            r = r.substr(0, d);
        }
    }

    return r;
}

Number number_from_string(const std::string &s)
{
    Number r;
    r.x = stod(s);
    return r;
}

Number number_from_uint32(uint32_t x)
{
    Number r;
    r.x = x;
    return r;
}

#elif defined(NUMBER_DECIMAL)

Number number_add(Number x, Number y)
{
    Number r;
    r.x = bid64_add(x.x, y.x);
    return r;
}

Number number_subtract(Number x, Number y)
{
    Number r;
    r.x = bid64_sub(x.x, y.x);
    return r;
}

Number number_multiply(Number x, Number y)
{
    Number r;
    r.x = bid64_mul(x.x, y.x);
    return r;
}

Number number_divide(Number x, Number y)
{
    Number r;
    r.x = bid64_div(x.x, y.x);
    // TODO: division by zero
    return r;
}

Number number_negate(Number x)
{
    Number r;
    r.x = bid64_negate(x.x);
    return r;
}

Number number_abs(Number x)
{
    Number r;
    r.x = bid64_abs(x.x);
    return r;
}

bool number_is_zero(Number x)
{
    return bid64_isZero(x.x) != 0;
}

bool number_is_equal(Number x, Number y)
{
    return bid64_quiet_equal(x.x, y.x) != 0;
}

bool number_is_not_equal(Number x, Number y)
{
    return bid64_quiet_not_equal(x.x, y.x);
}

bool number_is_less(Number x, Number y)
{
    return bid64_quiet_less(x.x, y.x);
}

bool number_is_greater(Number x, Number y)
{
    return bid64_quiet_greater(x.x, y.x);
}

bool number_is_less_equal(Number x, Number y)
{
    return bid64_quiet_less_equal(x.x, y.x);
}

bool number_is_greater_equal(Number x, Number y)
{
    return bid64_quiet_greater_equal(x.x, y.x);
}

std::string number_to_string(Number x)
{
    char buf[40];
    bid64_to_string(buf, x.x);

    // TODO: This hack converts a canonical string like +42E+0 to just 42
    if (strlen(buf) >= 3 && strcmp(&buf[strlen(buf)-3], "E+0") == 0) {
        buf[strlen(buf)-3] = '\0';
    }
    const char *r = buf;
    if (*r == '+') {
        r++;
    }

    return r;
}

Number number_from_string(const std::string &s)
{
    Number r;
    r.x = bid64_from_string(const_cast<char *>(s.c_str()));
    return r;
}

Number number_from_uint32(uint32_t x)
{
    Number r;
    r.x = bid64_from_uint32(x);
    return r;
}

#endif
