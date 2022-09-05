#include <string>
#include <vector>
#include <memory>
#include <optional>
#include <stdint.h>
using namespace std;

using i32 = int32_t;
using i16 = int16_t;
using i8  = int8_t;
using u32 = uint32_t;
using u16 = uint16_t;
using u8  = uint8_t;
using usize = size_t;

#define he_assert(expr) if(!(expr)) {printf("assertion fail at file:%s,line:%d",__FILE__,__LINE__);__debugbreak();}
#define he_countof(arr) sizeof(arr) / sizeof(arr[0])

template<typename T>
using ptr = shared_ptr<T>;