#include <iostream>

class Foo
{
public:
    Foo() : lasterr(ERR_EVE)
    {
    }

private:
    int lasterr;
    Foo(const Foo &);
    Foo &operator=(const Foo &);

    /* define error codes */
#ifndef ENUMERATE_ERRORS
#define ENUMERATE_ERRORS                      \
    DECLARE_ERROR(ERR_ALICE, "error : alice") \
    DECLARE_ERROR(ERR_BOB, "error : bob")     \
    DECLARE_ERROR(ERR_CARL, "error : carl")   \
    DECLARE_ERROR(ERR_DAVE, "error : dave")   \
    DECLARE_ERROR(ERR_EVE, "error : eve")     \
    /* you may add a new error above */
private:
    enum ERR_CODE
    {
        ERR_NONE = 0,
#define DECLARE_ERROR(errcode, errstring) errcode,
        ENUMERATE_ERRORS
#undef DECLARE_ERROR
            ERR_AMOUNT,
    };
public:
    const char *LastError() const
    {
        static const char *errmap[] = {
            "none",
#define DECLARE_ERROR(errcode, errstring) errstring,
            ENUMERATE_ERRORS
#undef DECLARE_ERROR
        };
        if ((lasterr < 0) || (lasterr >= ERR_AMOUNT))
        {
            return nullptr;
        }
        return errmap[lasterr];
    }
#undef ENUMERATE_ERRORS
#endif
};

int main(int argc, char **argv)
{
    (void)argc;
    (void)argv;

    Foo obj;
    std::cout << obj.LastError() << std::endl;
    return 0;
}