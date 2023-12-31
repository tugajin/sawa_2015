#ifndef MISC_H
#define MISC_H

#define BREAK_POINT do{ printf("\nBREAK\n");\
                        char _aaa_[1];\
                        std::cin>>_aaa_;\
                        std::cout<<_aaa_<<std::endl;\
                      }while(0);

#define enum_op(T)										\
	inline void operator += (T& lhs, const int rhs) { lhs  = static_cast<T>(static_cast<int>(lhs) + rhs); } \
	inline void operator += (T& lhs, const T   rhs) { lhs += static_cast<int>(rhs); } \
	inline void operator -= (T& lhs, const int rhs) { lhs  = static_cast<T>(static_cast<int>(lhs) - rhs); } \
	inline void operator -= (T& lhs, const T   rhs) { lhs -= static_cast<int>(rhs); } \
	inline void operator *= (T& lhs, const int rhs) { lhs  = static_cast<T>(static_cast<int>(lhs) * rhs); } \
	inline void operator /= (T& lhs, const int rhs) { lhs  = static_cast<T>(static_cast<int>(lhs) / rhs); } \
	inline T operator + (const T   lhs, const int rhs) { return static_cast<T>(static_cast<int>(lhs) + rhs); } \
	inline T operator + (const T   lhs, const T   rhs) { return lhs + static_cast<int>(rhs); } \
	inline T operator - (const T   lhs, const int rhs) { return static_cast<T>(static_cast<int>(lhs) - rhs); } \
	inline T operator - (const T   lhs, const T   rhs) { return lhs - static_cast<int>(rhs); } \
	inline T operator * (const T   lhs, const int rhs) { return static_cast<T>(static_cast<int>(lhs) * rhs); } \
	inline T operator * (const int lhs, const T   rhs) { return rhs * lhs; } \
	inline T operator * (const T   lhs, const T   rhs) { return lhs * static_cast<int>(rhs); } \
	inline T operator / (const T   lhs, const int rhs) { return static_cast<T>(static_cast<int>(lhs) / rhs); } \
	inline T operator - (const T   rhs) { return static_cast<T>(-static_cast<int>(rhs)); } \
	inline T operator ++ (T& lhs) { lhs += 1; return lhs; } /* 前置 */	\
	inline T operator -- (T& lhs) { lhs -= 1; return lhs; } /* 前置 */	\
	inline T operator ++ (T& lhs, int) { const T temp = lhs; lhs += 1; return temp; } /* 後置 */ \
	inline T operator -- (T& lhs, int) { const T temp = lhs; lhs -= 1; return temp; }  /* 後置 */

#endif
