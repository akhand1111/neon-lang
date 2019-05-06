#include "number.h"

#include <assert.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cell.h"


/*
 * Number / string functions
 */

char *number_to_string(Number x)
{
    static char buf[50];
    number_toString(x, buf, sizeof(buf));
    return buf;
}

void number_toString(Number x, char *buf, size_t len)
{
    memset(buf, 0x00, len);
    const int PRECISION = 34;

    assert(len != 0);

    char val[50] = { 0 };
    bid128_to_string(val, x);

    char *v, *s = v = &val[1];

    // Calculate the length of the string, locating the exponent value at the same time.
    int slen = 0;
    while (*s != '\0' && (*s != 'E')) {
        s++;
        slen++;
    }

    // Store the position of the exponent marker.
    char *E = s;

    // If we didn't find an exponent marker, then just return the buffer that it is.
    if (*s != 'E') {
        // Number is possibly +Nan, or +Inf.
        strncpy(buf, val, len);
        buf[len-1] = '\0';
        return;
    }

    // Get the actual exponent, by walking past the eponent marker, then converting to decimal value.
    int exponent = atoi(++s);

    // Place a NULL char at the exponent marker location, effectively terminating the string there.
    *E = '\0';

    // Now, we setup a (p)ointer to be one byte BEFORE the NULL we just added to the string, this
    // effectively places us at the end of the numerical string.
    char *p = E - 1;

    // We're going to start walking from right to left, looking for the last significant digit; the last digit in the string that isn't zero.
    while ((p >= v) && (*p == '0')) p--;

    // Once found, we can note the position of it in the string.  Since we know that v is a pointer to the start of the string, and p is the last digit found.
    const int last_significant_digit = (int)(p - v);

    if (last_significant_digit == -1) {
        buf[0] = '0';
        buf[1] = '\0';
        return;
    }

    // Once we know the last siginificant digit, we can calculate how many trailing zero existed.
    const int trailing_zeros = (int)slen - last_significant_digit-1;
    // Since we're going to chop of some trailing zeros, we need to update the length of our string.
    slen -= trailing_zeros;
    exponent += trailing_zeros;
    char mantissa[50] = { 0 };
    char r[50] = { 0 };
    // Copy the mantissa out to the last significant digit.
    memcpy(mantissa, v, last_significant_digit+1);
    // Guarantee the string is null terminated, despite the buffer starting off NULL.
    mantissa[last_significant_digit+1] = '\0';

    if (exponent != 0) {
        if (exponent > 0 && slen + exponent <= PRECISION) {
            // We have an exponent value we need to deal with, but the length combined with the exponent
            // is less than our desired max precision, so we move the mantissa value into (r)esult, the length
            // of the significant mantissa, adding zeros as necessary.
            memcpy(r, mantissa, last_significant_digit+1);
            for (int i = last_significant_digit+1; i < ((last_significant_digit+1) + exponent); i++, slen++) {
                r[i] = '0';
            }
        } else if (exponent < 0 && -exponent < slen) {
            // Our exponent is negative, and is less than the length of the mantissa, so we're
            // going to insert the decimal into slen+exponent location.
            memcpy(r, mantissa, slen+exponent);
            r[slen+exponent] = '.';
            memcpy(&r[slen+exponent+1], &mantissa[slen+exponent], slen - (slen+exponent));
        } else if (exponent < 0 && -exponent == slen) {
            // Our exponent is negative, and is the length of the mantissa, so we're going to start
            // the number with "0.", and then add the rest of the mantissa on the right side of the decimal.
            memcpy(r, "0.", 2);
            memcpy(&r[2], mantissa, slen+=2);
        } else if (exponent < 0 && slen - exponent <= PRECISION + 1) {
            // Our exponent is negative, and the mantissa length, minus the exponent less or equal to our max PRECISION
            // then we need to insert zero's to the right of the decimal.
            int count = 2;
            // Start out by setting [r]esult to "0."
            memcpy(r, "0.", count);
            // Now, we copy the necessary number of zeros after the decimal point...
            for (int i = 0; i < -exponent-slen;i++, count++) {
                r[count] = '0';
            }
            // Then copy the remainder of the mantissa to our [r]esult.
            memcpy(&r[count], mantissa, slen);
            // Ensure that [r]esult is null terminated.  I'm not 100% convinced this is necessary.
            slen += count;
            r[slen] = '\0';
        } else {
            // Our exponent is positive, therefore we need to add the decimal point on the right of the number.
            exponent += last_significant_digit;
            if (last_significant_digit >= 1) {
                // Our mantissa is at least 2 digits long, so copy the first digit to [r]esult
                r[0] = mantissa[0];
                // add the decimal point
                r[1] = '.';
                // copy the remainder of the mantissa to [r]esult
                memcpy(&r[2], &mantissa[1], last_significant_digit);
                slen++;
            } else {
                // The mantissa is only one significant digit, so copy that to the output, then apply the exponent value.
                memcpy(r, mantissa, last_significant_digit+1);
            }
            // Now, create the actual exponent value.
            char exp[36];
            int explen = sprintf(exp, "e%d", exponent);
            memcpy(&r[slen], exp, explen+1); // Include the terminating NULL char given to us by sprintf()
            slen += explen;
        }
    } else {
        // Exponent is zero, or just copy the mantissa, up to the last significant digit.
        memcpy(r, mantissa, last_significant_digit+1); // Include terminating NULL char.
    }
    if (val[0] == '-') {
        // The number is a negative number, so include the sign on the end of it.
        memcpy(&buf[1], r, slen+1); // Be sure to include the terminating NULL char.
        buf[0] = '-';
        return;
    }
    memcpy(buf, r, slen+1); // Be sure to include the terminating NULL char.
}

Number number_from_string(char *s)
{
    return bid128_from_string(s);
}


/*
 * Number math functions
 */

Number number_add(Number x, Number y)
{
    return bid128_add(x, y);
}

Number number_subtract(Number x, Number y)
{
    return bid128_sub(x, y);
}

Number number_divide(Number x, Number y)
{
    return bid128_div(x, y);
}

Number number_modulo(Number x, Number y)
{
    Number m = bid128_abs(y);
    if (bid128_isSigned(x)) {
        Number q = bid128_round_integral_positive(bid128_div(bid128_abs(x), m));
        x = bid128_add(x, bid128_mul(m, q));
    }
    Number r = bid128_fmod(x, m);
    if (bid128_isSigned(y) && !bid128_isZero(r)) {
        r = bid128_sub(r, m);
    }
    return r;
}

Number number_multiply(Number x, Number y)
{
    return bid128_mul(x, y);
}

Number number_negate(Number x)
{
    return bid128_negate(x);
}

Number number_pow(Number x, Number y)
{
    if (number_is_integer(y) && !number_is_negative(y)) {
        uint32_t iy = number_to_uint32(y);
        Number r = number_from_uint32(1);
        while (iy != 0) {
            if (iy & 1) {
                r = number_multiply(r, x);
            }
            x = number_multiply(x, x);
            iy >>= 1;
        }
        return r;
    }
    return bid128_pow(x, y);
}

Number number_abs(Number x)
{
    return bid128_abs(x);
}

Number number_sign(Number x)
{
    return bid128_copySign(bid128_from_uint32(1), x);
}

Number number_ceil(Number x)
{
    return bid128_round_integral_positive(x);
}

Number number_floor(Number x)
{
    return bid128_round_integral_negative(x);
}

Number number_trunc(Number x)
{
    return bid128_round_integral_zero(x);
}

Number number_exp(Number x)
{
    return bid128_exp(x);
}

Number number_log(Number x)
{
    return bid128_log(x);
}

Number number_sqrt(Number x)
{
    return bid128_sqrt(x);
}

Number number_acos(Number x)
{
    return bid128_acos(x);
}

Number number_asin(Number x)
{
    return bid128_asin(x);
}

Number number_atan(Number x)
{
    return bid128_atan(x);
}

Number number_cos(Number x)
{
    return bid128_cos(x);
}

Number number_sin(Number x)
{
    return bid128_sin(x);
}

Number number_tan(Number x)
{
    return bid128_tan(x);
}

Number number_acosh(Number x)
{
    return bid128_acosh(x);
}

Number number_asinh(Number x)
{
    return bid128_asinh(x);
}

Number number_atanh(Number x)
{
    return bid128_atanh(x);
}

Number number_atan2(Number y, Number x)
{
    return bid128_atan2(y, x);
}

Number number_cbrt(Number x)
{
    return bid128_cbrt(x);
}

Number number_cosh(Number x)
{
    return bid128_cosh(x);
}

Number number_erf(Number x)
{
    return bid128_erf(x);
}

Number number_erfc(Number x)
{
    return bid128_erfc(x);
}

Number number_exp2(Number x)
{
    return bid128_exp2(x);
}

Number number_expm1(Number x)
{
    return bid128_expm1(x);
}

Number number_frexp(Number x, int *exp)
{
    return bid128_frexp(x, exp);
}

Number number_hypot(Number x, Number y)
{
    return bid128_hypot(x, y);
}

Number number_ldexp(Number x, int exp)
{
    return bid128_ldexp(x, exp);
}

Number number_lgamma(Number x)
{
    return bid128_lgamma(x);
}

Number number_log10(Number x)
{
    return bid128_log10(x);
}

Number number_log1p(Number x)
{
    return bid128_log1p(x);
}

Number number_log2(Number x)
{
    return bid128_log2(x);
}

Number number_nearbyint(Number x)
{
    return bid128_nearbyint(x);
}

Number number_sinh(Number x)
{
    return bid128_sinh(x);
}

Number number_tanh(Number x)
{
    return bid128_tanh(x);
}

Number number_tgamma(Number x)
{
    return bid128_tgamma(x);
}


/*
 * Number Test functions
 */

BOOL number_is_integer(Number x)
{
    Number i = bid128_round_integral_zero(x);
    return bid128_quiet_equal(x, i) != 0;
}

BOOL number_is_nan(Number x)
{
    return bid128_isNaN(x) != 0;
}

BOOL number_is_negative(Number x)
{
    return bid128_isSigned(x) != 0;
}

BOOL number_is_zero(Number x)
{
    return bid128_isZero(x) != 0;
}

/*
 * Number TO functions
 */

int32_t number_to_sint32(Number x)
{
    return bid128_to_int32_int(x);
}

uint32_t number_to_uint32(Number x)
{
    return bid128_to_uint32_int(x);
}

int64_t number_to_sint64(Number x)
{ 
    return bid128_to_int64_int(x);
}

uint64_t number_to_uint64(Number x)
{
    return bid128_to_uint64_int(x);
}


/*
 * Number FROM functions
 */

Number number_from_sint32(int32_t x)
{
    return bid128_from_int32(x);
}

Number number_from_uint32(uint32_t x)
{
    return bid128_from_uint32(x);
}

Number number_from_uint64(uint64_t x)
{
    return bid128_from_uint64(x);
}

Number number_from_sint64(int64_t x)
{
    return bid128_from_int64(x);
}


/*
 * Number comparison functions
 */

BOOL number_is_equal(Number x, Number y)
{
    return bid128_quiet_equal(x, y);
}

BOOL number_is_not_equal(Number x, Number y)
{
    return bid128_quiet_not_equal(x, y) != 0;
}

BOOL number_is_less(Number x, Number y)
{
    return bid128_quiet_less(x, y);
}

BOOL number_is_greater(Number x, Number y)
{
    return bid128_quiet_greater(x, y) != 0;
}

BOOL number_is_less_equal(Number x, Number y)
{
    return bid128_quiet_less_equal(x, y) != 0;
}

BOOL number_is_greater_equal(Number x, Number y)
{
    return bid128_quiet_greater_equal(x, y) != 0;
}

BOOL number_is_odd(Number x)
{
    return !bid128_isZero(bid128_fmod(x, bid128_from_uint32(2)));
}

//#define __NUMBER_TESTS
#ifdef __NUMBER_TESTS
#include "time.h"
void main()
{
    srand((unsigned int)time(NULL));

    for (int i = 0; i < 100000; i++) {
        union {
            BID_UINT128 n;
            uint8_t bytes[16];
        } bid128_bytes;

        for (int x = 0; x < 16; x++) {
            bid128_bytes.bytes[x] = rand() & 0xFF;
        }
        char buf1[50];
        bid128_to_string(buf1, bid128_bytes.n);
        char *buf2 = number_to_string(bid128_bytes.n);
        Number x = bid128_from_string(buf1);
        Number y = bid128_from_string(buf2);
        if (!(number_is_equal(x, y) || (number_is_nan(x) && number_is_nan(y)))) {
            printf("%s %s\n", buf1, buf2);
            assert(FALSE);
        }
    }

    Number x;
    x = number_from_uint32(9);
    assert(strcasecmp(number_to_string(x), "9") == 0);
    x = number_from_uint32(20);
    assert(strcasecmp(number_to_string(x), "20") == 0);
    assert(strcasecmp(number_to_string(number_from_string("1234.5678e5")), "123456780") == 0);
    assert(strcasecmp(number_to_string(number_from_string("1234.5678e4")), "12345678") == 0);
    assert(strcasecmp(number_to_string(number_from_string("1234.5678e3")), "1234567.8") == 0);
    assert(strcasecmp(number_to_string(number_from_string("1234.5678e2")), "123456.78") == 0);
    assert(strcasecmp(number_to_string(number_from_string("1234.5678e1")), "12345.678") == 0);
    assert(strcasecmp(number_to_string(number_from_string("1234.5678e0")), "1234.5678") == 0);
    assert(strcasecmp(number_to_string(number_from_string("1234.5678e-1")), "123.45678") == 0);
    assert(strcasecmp(number_to_string(number_from_string("1234.5678e-2")), "12.345678") == 0);
    assert(strcasecmp(number_to_string(number_from_string("1234.5678e-3")), "1.2345678") == 0);
    assert(strcasecmp(number_to_string(number_from_string("1234.5678e-4")), "0.12345678") == 0);
    assert(strcasecmp(number_to_string(number_from_string("1234.5678e-5")), "0.012345678") == 0);
    assert(strcasecmp(number_to_string(number_from_string("-1234.5678e-2")), "-12.345678") == 0);
    assert(strcasecmp(number_to_string(number_from_string("-12345678e5")), "-1234567800000") == 0);
    assert(strcasecmp(number_to_string(number_from_string("-12345678e-10")), "-0.0012345678") == 0);

    assert(strcasecmp(number_to_string(number_from_string("-12345678e-36")), "-1.2345678e-29") == 0);
    assert(strcasecmp(number_to_string(number_from_string("+12345678e-56")), "1.2345678e-49") == 0);
    assert(strcasecmp(number_to_string(number_from_string("+12345678e56")), "1.2345678e63") == 0);
    assert(strcasecmp(number_to_string(number_from_string("-12345678e56")), "-1.2345678e63") == 0);
    assert(strcasecmp(number_to_string(number_from_string("-12345678e26")), "-1234567800000000000000000000000000") == 0);
    assert(strcasecmp(number_to_string(number_from_string("-12345678e-26")), "-0.00000000000000000012345678") == 0);
    assert(strcasecmp(number_to_string(number_from_string("+12345678e-26")), "0.00000000000000000012345678") == 0);
    assert(strcasecmp(number_to_string(number_from_string("+12345678e26")), "1234567800000000000000000000000000") == 0);
    assert(strcasecmp(number_to_string(number_from_string("+1234567800000000000000000000000001E-13")), "123456780000000000000.0000000000001") == 0);

    assert(strcasecmp(number_to_string(number_from_string("-1234567800000000000000000000000001E-34")), "-0.1234567800000000000000000000000001") == 0);
    assert(strcasecmp(number_to_string(number_from_string("-1234567800000000000000000000000001E-35")), "-1.234567800000000000000000000000001e-2") == 0);
    assert(strcasecmp(number_to_string(number_from_string("-12345678000000000000000000000000E2")), "-1234567800000000000000000000000000") == 0);
    assert(strcasecmp(number_to_string(number_from_string("-12345678000000000000000000000000E3")), "-1.2345678e34") == 0);
    assert(strcasecmp(number_to_string(number_from_string("+12345678000000000000000000000000E2")), "1234567800000000000000000000000000") == 0);
    assert(strcasecmp(number_to_string(number_from_string("+12345678000000000000000000000000E3")), "1.2345678e34") == 0);
    assert(strcasecmp(number_to_string(number_from_string("+1000000000000000000000000000000000E+167")), "1e200") == 0);
    assert(strcasecmp(number_to_string(number_from_string("+1100000000000000000000000000000000E+167")), "1.1e200") == 0);
    assert(strcasecmp(number_to_string(number_from_string("1000000000000000000000000000000000")), "1000000000000000000000000000000000") == 0);
    assert(strcasecmp(number_to_string(number_from_string("+1E+34")), "1e34") == 0);
    assert(strcasecmp(number_to_string(number_from_string("+11E+34")), "1.1e35") == 0);
    assert(strcasecmp(number_to_string(number_from_string("+1E+9999")), "+Inf") == 0);

    assert(strcasecmp(number_to_string(number_from_string("99999999999999999999999999999999994.0")), "9.999999999999999999999999999999999e34") == 0);
    assert(strcasecmp(number_to_string(number_from_string("+1E-34")), "0.0000000000000000000000000000000001") == 0);
    assert(strcasecmp(number_to_string(number_from_string("-1E-34")), "-0.0000000000000000000000000000000001") == 0);
    assert(strcasecmp(number_to_string(number_from_string("+1E-35")), "1e-35") == 0);
    assert(strcasecmp(number_to_string(number_from_string("-1E-35")), "-1e-35") == 0);
    assert(strcasecmp(number_to_string(number_from_string("+0E-6176")), "0") == 0);
    assert(strcasecmp(number_to_string(number_from_string("+8018797208429722826939072854263270E-32")), "80.1879720842972282693907285426327") == 0);
    assert(strcasecmp(number_to_string(number_from_string("+707486673985408982210122026333411E-33")), "0.707486673985408982210122026333411") == 0);
}
#endif
