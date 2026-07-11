#ifndef BYTECODE_HPP_
#define BYTECODE_HPP_

#include <cassert>
#include <cstdint>
#include <string_view>

// Opcode Design borrowed from the Lua Programming Language
/******************************************************************************
 * Copyright (C) 1994-2026 Lua.org, PUC-Rio.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 ******************************************************************************/

/*===========================================================================
  We assume that instructions are unsigned 32-bit integers.
  All instructions have an opcode in the first 7 bits.
  Instructions can have the following formats:

        3 3 2 2 2 2 2 2 2 2 2 2 1 1 1 1 1 1 1 1 1 1 0 0 0 0 0 0 0 0 0 0
        1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
iABC          C(8)     |      B(8)     |k|     A(8)      |   Op(7)     |
ivABC         vC(10)     |     vB(6)   |k|     A(8)      |   Op(7)     |
iABx                Bx(17)               |     A(8)      |   Op(7)     |
iAsBx              sBx (signed)(17)      |     A(8)      |   Op(7)     |
iAx                           Ax(25)                     |   Op(7)     |
isJ                           sJ (signed)(25)            |   Op(7)     |

  ('v' stands for "variant", 's' for "signed", 'x' for "extended".)
  A signed argument is represented in excess K: The represented value is
  the written unsigned value minus K, where K is half (rounded down) the
  maximum value for the corresponding unsigned argument.
===========================================================================*/
namespace Anvil {

namespace Code {

using Inst = uint32_t;

// clang-format off
#define OPCODE_ENUM(Name, Str) Name,
enum class Op {
  #include "compiler/bytecode.def"
};
#undef OPCODE_ENUM
// clang-format on

enum class Mode { iABC, ivABC, iABx, iAsBx, iAx, isJ };

// clang-format off
struct OpMetadata {
  uint8_t raw;

  enum Mask : uint8_t {
    ModeMask = 0x07, // Bits 0-2: Instruction mode
    SetsA = 0x08,    // Bit 3:   Sets register A?
    IsTest = 0x10,   // Bit 4:   Is a test/conditional jump?
    UsesIT = 0x20,   // Bit 5:   Input Target (uses register/constant)
    UsesOT = 0x40,   // Bit 6:   Output Target (returns value to register)
    UsesMM = 0x80    // Bit 7:   Uses MetaMethod (e.g., __add, __index)
  };

  constexpr Mode mode() const { return static_cast<Mode>(raw & ModeMask); }
  constexpr bool sets_a() const { return (raw & SetsA) != 0; }
  constexpr bool is_test() const { return (raw & IsTest) != 0; }
  constexpr bool uses_it() const { return (raw & UsesIT) != 0; }
  constexpr bool uses_ot() const { return (raw & UsesOT) != 0; }
  constexpr bool uses_mm() const { return (raw & UsesMM) != 0; }

  static constexpr OpMetadata Create(Mode m, bool sets_a, bool is_test,
                                     bool it = false, bool ot = false,
                                     bool mm = false) {
    return OpMetadata{ static_cast<uint8_t>(
            static_cast<uint8_t>(m) |
            (sets_a ? SetsA : 0)    |
            (is_test ? IsTest : 0)  |
            (it ? UsesIT : 0)       |
            (ot ? UsesOT : 0)       |
            (mm ? UsesMM : 0)
    )};
  }
};
// clang-format on

static_assert(sizeof(OpMetadata) == 1, "Must stay exactly 1 byte for L1 cache");

// clang-format off
#define OPCODE_ENUM(Name, Str) #Str,
inline constexpr std::string_view kOpNames[] = {
    #include "compiler/bytecode.def"
};
#undef OPCODE_ENUM
// clang-format on

constexpr std::string_view GetOpName(Op op) {
  return kOpNames[static_cast<uint32_t>(op)];
}

// clang-format off
inline constexpr OpMetadata kOpInfo[] = {
    //                                 Mode,        seta,  test,  it,    ot,    metam
    /* Op::Move  */ OpMetadata::Create(Mode::iABC,  true,  false, false, true,  false),
    /* Op::LoadK */ OpMetadata::Create(Mode::iABx,  true,  false, false, true,  false),
    /* Op::Add   */ OpMetadata::Create(Mode::iABC,  true,  false, true,  true,  true ),
    /* Op::Sub   */ OpMetadata::Create(Mode::iABC,  true,  false, true,  true,  true ),
    /* Op::Mult  */ OpMetadata::Create(Mode::iABC,  true,  false, true,  true,  true ),
    /* Op::Div   */ OpMetadata::Create(Mode::iABC,  true,  false, true,  true,  true ),
    /* Op::Ret   */ OpMetadata::Create(Mode::iABC,  false, false, false, false, false),
    /* Op::Test  */ OpMetadata::Create(Mode::iABC,  false, true,  false, false, false),
    /* Op::Jmp   */ OpMetadata::Create(Mode::isJ,   false, false, false, false, false),
    /* Op::LT    */ OpMetadata::Create(Mode::iABC,  true,  false, true,  true,  true ),
    /* Op::LE    */ OpMetadata::Create(Mode::iABC,  true,  false, true,  true,  true ),
    /* Op::GT    */ OpMetadata::Create(Mode::iABC,  true,  false, true,  true,  true ),
    /* Op::GE    */ OpMetadata::Create(Mode::iABC,  true,  false, true,  true,  true ),
    /* Op::EQ    */ OpMetadata::Create(Mode::iABC,  true,  false, true,  true,  true ),
    /* Op::NE    */ OpMetadata::Create(Mode::iABC,  true,  false, true,  true,  true ),
    /* Op::BAnd  */ OpMetadata::Create(Mode::iABC,  true,  false, true,  true,  true ),
    /* Op::BOr   */ OpMetadata::Create(Mode::iABC,  true,  false, true,  true,  true ),
    /* Op::BXor  */ OpMetadata::Create(Mode::iABC,  true,  false, true,  true,  true ),
    /* Op::BNot  */ OpMetadata::Create(Mode::iABC,  true,  false, true,  true,  true ),
    /* Op::CallB */ OpMetadata::Create(Mode::iABC,  true,  false, false, false, true),
};
// clang-format on

// If any new Ops are added to the bytecode.def file this will ensure that
// the kOpInfo tables remains in sync.
static_assert(std::size(kOpInfo) == std::size(kOpNames));

constexpr Mode GetMode(Op op) {
  return kOpInfo[static_cast<uint32_t>(op)].mode();
}

constexpr bool OpUsesMetaMethod(Op op) {
  return kOpInfo[static_cast<uint32_t>(op)].uses_mm();
}

constexpr bool OpHasOutputTarget(Op op) {
  return kOpInfo[static_cast<uint32_t>(op)].uses_ot();
}

constexpr bool OpSetsA(Op op) {
  return kOpInfo[static_cast<uint32_t>(op)].sets_a();
}

constexpr bool OpIsTest(Op op) {
  return kOpInfo[static_cast<uint32_t>(op)].is_test();
}

constexpr uint32_t kSizeC = 8;
constexpr uint32_t kSizevC = 10;
constexpr uint32_t kSizeB = 8;
constexpr uint32_t kSizevB = 6;
constexpr uint32_t kSizeBx = (kSizeC + kSizeB + 1);
constexpr uint32_t kSizeA = 8;
constexpr uint32_t kOpSize = 7;
constexpr uint32_t kSizeAx = (kSizeBx + kSizeA);
constexpr uint32_t kSizesJ = (kSizeBx + kSizeA);
constexpr uint32_t kPosOp = 0;
constexpr uint32_t kPosA = (kPosOp + kOpSize);
constexpr uint32_t kPosK = (kPosA + kSizeA);
constexpr uint32_t kPosB = (kPosK + 1);
constexpr uint32_t kPosvB = (kPosK + 1);
constexpr uint32_t kPosC = (kPosB + kSizeB);
constexpr uint32_t kPosvC = (kPosvB + kSizevB);
constexpr uint32_t kPosBx = kPosK;
constexpr uint32_t kPosAx = kPosA;
constexpr uint32_t kPossJ = kPosA;

// Limits for opcode arguments. Signed arguments are stored in excess-K:
// the stored (unsigned) value minus the offset is the represented value.
constexpr uint32_t kMaxA = (1u << kSizeA) - 1;
constexpr uint32_t kMaxB = (1u << kSizeB) - 1;
constexpr uint32_t kMaxvB = (1u << kSizevB) - 1;
constexpr uint32_t kMaxC = (1u << kSizeC) - 1;
constexpr uint32_t kMaxvC = (1u << kSizevC) - 1;
constexpr uint32_t kMaxBx = (1u << kSizeBx) - 1;
constexpr uint32_t kMaxAx = (1u << kSizeAx) - 1;
constexpr uint32_t kMaxsJ = (1u << kSizesJ) - 1;
constexpr int32_t kOffsetsC = kMaxC >> 1;
constexpr int32_t kOffsetsBx = kMaxBx >> 1;
constexpr int32_t kOffsetsJ = kMaxsJ >> 1;

// Registers live in A (8 bits), so kMaxA is the stack ceiling and doubles
// as the "no register" sentinel (one past the last valid register).
constexpr uint32_t kMaxRegs = kMaxA;
constexpr uint32_t kNoReg = kMaxA;

constexpr uint32_t Mask1(uint32_t n, uint32_t p) {
  // ~(Inst)0            1111'1111'1111'1111'1111'1111'1111'1111   0xFFFFFFFF
  // << n   (ie n=6)     1111'1111'1111'1111'1111'1111'1100'0000   0xFFFFFFC0
  // 6 zeros pushed in at bottom
  // ~(...)              0000'0000'0000'0000'0000'0000'0011'1111   0x0000003F
  // flip: 6 ones at bottom, rest 0
  // << p   (p=0)        0000'0000'0000'0000'0000'0000'0011'1111   0x0000003F
  // slide the field up to position p
  return ((~((~(Inst)0) << (n))) << (p));
}

constexpr uint32_t Mask0(uint32_t n, uint32_t p) {
  // ~(Inst)0            1111'1111'1111'1111'1111'1111'1111'1111   0xFFFFFFFF
  // << n   (ie n=6)     1111'1111'1111'1111'1111'1111'1100'0000   0xFFFFFFC0
  // 6 zeros pushed in at bottom
  // ~(...)              0000'0000'0000'0000'0000'0000'0011'1111   0x0000003F
  // flip: 6 ones at bottom, rest 0
  // << p   (p=0)        0000'0000'0000'0000'0000'0000'0011'1111   0x0000003F
  // slide the field up to position p
  // ~(...)              1111'1111'1111'1111'1111'1111'1100'0000   0xFFFFFFC0
  // flips 1's to 0's
  return ~Mask1(n, p);
}

// Signed <-> excess-K conversion for the 8-bit B/C fields.
constexpr uint32_t Int2sC(int32_t i) {
  return static_cast<uint32_t>(i + kOffsetsC);
}
constexpr int32_t sC2Int(uint32_t u) {
  return static_cast<int32_t>(u) - kOffsetsC;
}

constexpr uint32_t GetArg(Inst i, uint32_t pos, uint32_t size) {
  return (i >> pos) & Mask1(size, 0);
}

inline void SetArg(Inst &i, uint32_t v, uint32_t pos, uint32_t size) {
  i = (i & Mask0(size, pos)) | ((v << pos) & Mask1(size, pos));
}

constexpr Op GetOp(Inst i) { return Op(((i) >> kPosOp) & Mask1(kOpSize, 0)); }

inline void SetOp(Inst &i, Op op) {
  SetArg(i, static_cast<uint32_t>(op), kPosOp, kOpSize);
}

constexpr bool CheckMode(Inst i, Mode m) { return GetMode(GetOp(i)) == m; }

constexpr uint32_t GetA(Inst i) { return GetArg(i, kPosA, kSizeA); }

constexpr uint32_t GetK(Inst i) { return GetArg(i, kPosK, 1); }
constexpr bool TestK(Inst i) { return (i & (1u << kPosK)) != 0; }

constexpr uint32_t GetB(Inst i) {
  assert(CheckMode(i, Mode::iABC));
  return GetArg(i, kPosB, kSizeB);
}

constexpr uint32_t GetvB(Inst i) {
  assert(CheckMode(i, Mode::ivABC));
  return GetArg(i, kPosvB, kSizevB);
}

constexpr int32_t GetsB(Inst i) { return sC2Int(GetB(i)); }

constexpr uint32_t GetC(Inst i) {
  assert(CheckMode(i, Mode::iABC));
  return GetArg(i, kPosC, kSizeC);
}

constexpr uint32_t GetvC(Inst i) {
  assert(CheckMode(i, Mode::ivABC));
  return GetArg(i, kPosvC, kSizevC);
}

constexpr int32_t GetsC(Inst i) { return sC2Int(GetC(i)); }

constexpr uint32_t GetBx(Inst i) {
  assert(CheckMode(i, Mode::iABx));
  return GetArg(i, kPosBx, kSizeBx);
}

constexpr int32_t GetsBx(Inst i) {
  assert(CheckMode(i, Mode::iAsBx));
  return static_cast<int32_t>(GetArg(i, kPosBx, kSizeBx)) - kOffsetsBx;
}

constexpr uint32_t GetAx(Inst i) {
  assert(CheckMode(i, Mode::iAx));
  return GetArg(i, kPosAx, kSizeAx);
}

constexpr int32_t GetsJ(Inst i) {
  assert(CheckMode(i, Mode::isJ));
  return static_cast<int32_t>(GetArg(i, kPossJ, kSizesJ)) - kOffsetsJ;
}

inline void SetA(Inst &i, uint32_t v) { SetArg(i, v, kPosA, kSizeA); }
inline void SetK(Inst &i, uint32_t v) { SetArg(i, v, kPosK, 1); }
inline void SetB(Inst &i, uint32_t v) { SetArg(i, v, kPosB, kSizeB); }
inline void SetvB(Inst &i, uint32_t v) { SetArg(i, v, kPosvB, kSizevB); }
inline void SetC(Inst &i, uint32_t v) { SetArg(i, v, kPosC, kSizeC); }
inline void SetvC(Inst &i, uint32_t v) { SetArg(i, v, kPosvC, kSizevC); }
inline void SetBx(Inst &i, uint32_t v) { SetArg(i, v, kPosBx, kSizeBx); }
inline void SetAx(Inst &i, uint32_t v) { SetArg(i, v, kPosAx, kSizeAx); }

inline void SetsBx(Inst &i, int32_t v) {
  SetBx(i, static_cast<uint32_t>(v + kOffsetsBx));
}

inline void SetsJ(Inst &i, int32_t v) {
  SetArg(i, static_cast<uint32_t>(v + kOffsetsJ), kPossJ, kSizesJ);
}

constexpr Inst CreateABCk(Op o, uint32_t a, uint32_t b, uint32_t c,
                          uint32_t k = 0) {
  assert(a <= kMaxA && b <= kMaxB && c <= kMaxC && k <= 1);
  return (static_cast<Inst>(o) << kPosOp) | (a << kPosA) | (b << kPosB) |
         (c << kPosC) | (k << kPosK);
}

constexpr Inst CreatevABCk(Op o, uint32_t a, uint32_t b, uint32_t c,
                           uint32_t k = 0) {
  assert(a <= kMaxA && b <= kMaxvB && c <= kMaxvC && k <= 1);
  return (static_cast<Inst>(o) << kPosOp) | (a << kPosA) | (b << kPosvB) |
         (c << kPosvC) | (k << kPosK);
}

constexpr Inst CreateABx(Op o, uint32_t a, uint32_t bx) {
  assert(a <= kMaxA && bx <= kMaxBx);
  return (static_cast<Inst>(o) << kPosOp) | (a << kPosA) | (bx << kPosBx);
}

constexpr Inst CreateAsBx(Op o, uint32_t a, int32_t sbx) {
  assert(a <= kMaxA && sbx >= -kOffsetsBx &&
         sbx <= static_cast<int32_t>(kMaxBx) - kOffsetsBx);
  return (static_cast<Inst>(o) << kPosOp) | (a << kPosA) |
         (static_cast<uint32_t>(sbx + kOffsetsBx) << kPosBx);
}

constexpr Inst CreateAx(Op o, uint32_t ax) {
  assert(ax <= kMaxAx);
  return (static_cast<Inst>(o) << kPosOp) | (ax << kPosAx);
}

constexpr Inst CreatesJ(Op o, int32_t sj, uint32_t k = 0) {
  assert(sj >= -kOffsetsJ && sj <= static_cast<int32_t>(kMaxsJ) - kOffsetsJ &&
         k <= 1);
  return (static_cast<Inst>(o) << kPosOp) |
         (static_cast<uint32_t>(sj + kOffsetsJ) << kPossJ) | (k << kPosK);
}

} // namespace Code

} // namespace Anvil
#endif
