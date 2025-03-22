CACHED_METHOD(INIT,          "__init__",          _init)
CACHED_METHOD(GET,           "__getitem__",       _getter)
CACHED_METHOD(SET,           "__setitem__",       _setter)
CACHED_METHOD(REPR,          "__repr__",          _reprer)
CACHED_METHOD(STR,           "__str__",           _tostr)
CACHED_METHOD(CALL,          "__call__",          _call)
CACHED_METHOD(EQ,            "__eq__",            _eq)
CACHED_METHOD(LEN,           "__len__",           _len)
CACHED_METHOD(ENTER,         "__enter__",         _enter)
CACHED_METHOD(EXIT,          "__exit__",          _exit)
CACHED_METHOD(DELITEM,       "__delitem__",       _delitem)
CACHED_METHOD(ITER,          "__iter__",          _iter)
CACHED_METHOD(GETATTR,       "__getattr__",       _getattr)
CACHED_METHOD(DIR,           "__dir__",           _dir)
CACHED_METHOD(CONTAINS,      "__contains__",      _contains)
CACHED_METHOD(DESCGET,       "__get__",           _descget)
CACHED_METHOD(DESCSET,       "__set__",           _descset)
CACHED_METHOD(CLASSGETITEM,  "__class_getitem__", _classgetitem)
CACHED_METHOD(HASH,          "__hash__",          _hash)
CACHED_METHOD(BOOL,          "__bool__",          _bool)

#define BINOPTRIO(name) \
    CACHED_METHOD(name, "__" #name "__", _ ## name) \
    CACHED_METHOD(R ## name, "__r" #name "__", _r ## name) \
    CACHED_METHOD(I ## name, "__i" #name "__", _i ## name)

BINOPTRIO(add)
BINOPTRIO(sub)
BINOPTRIO(mul)
BINOPTRIO(pow)
BINOPTRIO(or)
BINOPTRIO(xor)
BINOPTRIO(and)
BINOPTRIO(mod)
BINOPTRIO(lshift)
BINOPTRIO(rshift)
BINOPTRIO(floordiv)
BINOPTRIO(truediv)
BINOPTRIO(matmul)

CACHED_METHOD(LT, "__lt__", _lt)
CACHED_METHOD(GT, "__gt__", _gt)
CACHED_METHOD(LE, "__le__", _le)
CACHED_METHOD(GE, "__ge__", _ge)
CACHED_METHOD(INVERT, "__invert__", _invert)
CACHED_METHOD(NEGATE, "__neg__", _negate)
CACHED_METHOD(SETNAME, "__set_name__", _set_name)
CACHED_METHOD(POS, "__pos__", _pos)
CACHED_METHOD(SETATTR, "__setattr__", _setattr)
CACHED_METHOD(FORMAT, "__format__", _format)
CACHED_METHOD(NEW, "__new__", _new)

/* These are not methods */
SPECIAL_ATTRS(CLASS,     "__class__")
SPECIAL_ATTRS(NAME,      "__name__")
SPECIAL_ATTRS(DOC,       "__doc__")
SPECIAL_ATTRS(BASE,      "__base__")
SPECIAL_ATTRS(FILE,      "__file__")
/* These should probably also be cached */
SPECIAL_ATTRS(INT,       "__int__")
SPECIAL_ATTRS(CHR,       "__chr__")
SPECIAL_ATTRS(ORD,       "__ord__")
SPECIAL_ATTRS(FLOAT,     "__float__")
SPECIAL_ATTRS(STRSTRIP,  " \t\n\r")
SPECIAL_ATTRS(HEX,       "__hex__")
SPECIAL_ATTRS(OCT,       "__oct__")
SPECIAL_ATTRS(BIN,       "__bin__")
SPECIAL_ATTRS(ABS,       "__abs__")
SPECIAL_ATTRS(FUNC,      "__func__")
SPECIAL_ATTRS(BLDCLS,    "__build_class__")
SPECIAL_ATTRS(MAIN,      "__main__")
