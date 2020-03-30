#ifndef RUA_DYFN
#define RUA_DYFN(name) find(#name).as<decltype(&name)>()
#endif
