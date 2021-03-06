#pragma once

namespace mcl { namespace fp {

template<>
struct EnableKaratsuba<Ltag> {
#if MCL_SIZEOF_UNIT == 4
	static const size_t minMulN = 10;
	static const size_t minSqrN = 10;
#else
	static const size_t minMulN = 8;
	static const size_t minSqrN = 6;
#endif
};

#if MCL_SIZEOF_UNIT == 4
	#define MCL_GMP_IS_FASTER_THAN_LLVM // QQQ : check later
#endif

#ifdef MCL_GMP_IS_FASTER_THAN_LLVM
#define MCL_DEF_MUL(n, tag, suf)
#else
#define MCL_DEF_MUL(n, tag, suf) \
template<>const void3u MulPreCore<n, tag>::f = &mcl_fpDbl_mulPre ## n ## suf; \
template<>const void2u SqrPreCore<n, tag>::f = &mcl_fpDbl_sqrPre ## n ## suf;
#endif

#define MCL_DEF_LLVM_FUNC2(n, tag, suf) \
template<>const u3u AddPre<n, tag>::f = &mcl_mcl_fp_addPre ## n ## suf; \
template<>const u3u SubPre<n, tag>::f = &mcl_mcl_fp_subPre ## n ## suf; \
template<>const void2u Shr1<n, tag>::f = &mcl_mcl_fp_shr1_ ## n ## suf; \
MCL_DEF_MUL(n, tag, suf) \
template<>const void2uI MulUnitPre<n, tag>::f = &mcl_mcl_fp_mulUnitPre ## n ## suf; \
template<>const void4u Add<n, true, tag>::f = &mcl_mcl_fp_add ## n ## suf; \
template<>const void4u Add<n, false, tag>::f = &mcl_mcl_fp_addNF ## n ## suf; \
template<>const void4u Sub<n, true, tag>::f = &mcl_mcl_fp_sub ## n ## suf; \
template<>const void4u Sub<n, false, tag>::f = &mcl_mcl_fp_subNF ## n ## suf; \
template<>const void4u Mont<n, true, tag>::f = &mcl_mcl_fp_mont ## n ## suf; \
template<>const void4u Mont<n, false, tag>::f = &mcl_mcl_fp_montNF ## n ## suf; \
template<>const void3u MontRed<n, tag>::f = &mcl_mcl_fp_montRed ## n ## suf; \
template<>const void4u DblAdd<n, tag>::f = &mcl_fpDbl_add ## n ## suf; \
template<>const void4u DblSub<n, tag>::f = &mcl_fpDbl_sub ## n ## suf; \

#if MCL_LLVM_BMI2 == 1
#define MCL_DEF_LLVM_FUNC(n) \
	MCL_DEF_LLVM_FUNC2(n, Ltag, L) \
	MCL_DEF_LLVM_FUNC2(n, LBMI2tag, Lbmi2)
#else
#define MCL_DEF_LLVM_FUNC(n) \
	MCL_DEF_LLVM_FUNC2(n, Ltag, L)
#endif

MCL_DEF_LLVM_FUNC(1)
MCL_DEF_LLVM_FUNC(2)
MCL_DEF_LLVM_FUNC(3)
MCL_DEF_LLVM_FUNC(4)
#if MCL_MAX_UNIT_SIZE >= 6
MCL_DEF_LLVM_FUNC(5)
MCL_DEF_LLVM_FUNC(6)
#endif
#if MCL_MAX_UNIT_SIZE >= 8
MCL_DEF_LLVM_FUNC(7)
MCL_DEF_LLVM_FUNC(8)
#endif
#if MCL_MAX_UNIT_SIZE >= 9
MCL_DEF_LLVM_FUNC(9)
#endif
#if MCL_MAX_UNIT_SIZE >= 10
MCL_DEF_LLVM_FUNC(10)
#endif
#if MCL_MAX_UNIT_SIZE >= 12
MCL_DEF_LLVM_FUNC(11)
MCL_DEF_LLVM_FUNC(12)
#endif
#if MCL_MAX_UNIT_SIZE >= 14
MCL_DEF_LLVM_FUNC(13)
MCL_DEF_LLVM_FUNC(14)
#endif
#if MCL_MAX_UNIT_SIZE >= 16
MCL_DEF_LLVM_FUNC(15)
#if MCL_SIZEOF_UNIT == 4
MCL_DEF_LLVM_FUNC(16)
#else
/// QQQ : check speed
template<>const void3u MontRed<16, Ltag>::f = &mcl_mcl_fp_montRed16L;
template<>const void3u MontRed<16, LBMI2tag>::f = &mcl_mcl_fp_montRed16Lbmi2;
#endif
#endif
#if MCL_MAX_UNIT_SIZE >= 17
MCL_DEF_LLVM_FUNC(17)
#endif

} } // mcl::fp

