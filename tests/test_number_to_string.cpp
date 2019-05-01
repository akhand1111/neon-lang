#include <assert.h>
#include <iostream>
#include <iso646.h>
#include <random>

#include "number.h"

int main()
{
    std::random_device rd;
    std::mt19937_64 r(rd());
    for (int i = 0; i < 100000; i++) {
        BID_UINT128 n;
        n.w[0] = r();
        n.w[1] = r();
        char buf1[50];
        bid128_to_string(buf1, n);
        std::string buf2 = number_to_string(n);
        Number x = bid128_from_string(buf1);
        Number y = bid128_from_string(const_cast<char *>(buf2.c_str()));
        if (not (number_is_equal(x, y) || (number_is_nan(x) && number_is_nan(y)))) {
            std::cout << buf1 << " " << buf2 << std::endl;
            assert(false);
        }
    }
    assert(number_to_string(number_from_string("1234.5678e5")) == "123456780");
    assert(number_to_string(number_from_string("1234.5678e4")) == "12345678");
    assert(number_to_string(number_from_string("1234.5678e3")) == "1234567.8");
    assert(number_to_string(number_from_string("1234.5678e2")) == "123456.78");
    assert(number_to_string(number_from_string("1234.5678e1")) == "12345.678");
    assert(number_to_string(number_from_string("1234.5678e0")) == "1234.5678");
    assert(number_to_string(number_from_string("1234.5678e-1")) == "123.45678");
    assert(number_to_string(number_from_string("1234.5678e-2")) == "12.345678");
    assert(number_to_string(number_from_string("1234.5678e-3")) == "1.2345678");
    assert(number_to_string(number_from_string("1234.5678e-4")) == "0.12345678");
    assert(number_to_string(number_from_string("1234.5678e-5")) == "0.012345678");
    assert(number_to_string(number_from_string("-12345678e-36")) == "-1.2345678e-29");
    assert(number_to_string(number_from_string("+12345678e-56")) == "1.2345678e-49");
    assert(number_to_string(number_from_string("+12345678e56")) == "1.2345678e63");
    assert(number_to_string(number_from_string("-12345678e56")) == "-1.2345678e63");
    assert(number_to_string(number_from_string("-12345678e26")) == "-1234567800000000000000000000000000");
    assert(number_to_string(number_from_string("-12345678e-26")) == "-0.00000000000000000012345678");
    assert(number_to_string(number_from_string("+12345678e-26")) == "0.00000000000000000012345678");
    assert(number_to_string(number_from_string("+12345678e26")) == "1234567800000000000000000000000000");
    assert(number_to_string(number_from_string("+1234567800000000000000000000000001E-13")) == "123456780000000000000.0000000000001");

    assert(number_to_string(number_from_string("-1234567800000000000000000000000001E-34")) == "-0.1234567800000000000000000000000001");
    assert(number_to_string(number_from_string("-1234567800000000000000000000000001E-35")) == "-1.234567800000000000000000000000001e-2");
    assert(number_to_string(number_from_string("-12345678000000000000000000000000E2")) == "-1234567800000000000000000000000000");
    assert(number_to_string(number_from_string("-12345678000000000000000000000000E3")) == "-1.2345678e34");
    assert(number_to_string(number_from_string("+12345678000000000000000000000000E2")) == "1234567800000000000000000000000000");
    assert(number_to_string(number_from_string("+12345678000000000000000000000000E3")) == "1.2345678e34");
    assert(number_to_string(number_from_string("+1000000000000000000000000000000000E+167")) == "1e200");
    assert(number_to_string(number_from_string("+1100000000000000000000000000000000E+167")) == "1.1e200");
    assert(number_to_string(number_from_string("1000000000000000000000000000000000")) == "1000000000000000000000000000000000");
    assert(number_to_string(number_from_string("+1E+34")) == "1e34");
    assert(number_to_string(number_from_string("+11E+34")) == "1.1e35");
    assert(number_to_string(number_from_string("+1E+9999")) == "+Inf");

    assert(number_to_string(number_from_string("99999999999999999999999999999999994.0")) == "9.999999999999999999999999999999999e34");
    assert(number_to_string(number_from_string("+1E-34")) == "0.0000000000000000000000000000000001");
    assert(number_to_string(number_from_string("-1E-34")) == "-0.0000000000000000000000000000000001");
    assert(number_to_string(number_from_string("+1E-35")) == "1e-35");
    assert(number_to_string(number_from_string("-1E-35")) == "-1e-35");
    assert(number_to_string(number_from_string("+0E-6176")) == "0");
    assert(number_to_string(number_from_string("+8018797208429722826939072854263270E-32")) == "80.1879720842972282693907285426327");
    assert(number_to_string(number_from_string("+707486673985408982210122026333411E-33")) == "0.707486673985408982210122026333411");

}
