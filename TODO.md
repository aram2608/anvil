# TODO: Builtins then Functions

Order matters: builtins first because `@print` needs the calling convention
(args in consecutive registers, a call opcode) but none of the frame machinery.
User functions then inherit conventions that already work.

## User functions

### 1. Proto object

```cpp
struct Proto {
  Block block;
  uint8_t arity;
  uint8_t max_regs; // frame size, compiler knows this after compiling the body
};
```

- `Kind::Function`, `Proto *p` in the Value union (still 16 bytes)
- ownership: Block is currently copied by value into the VM; protos need stable
  addresses. Module/script owns `std::vector<std::unique_ptr<Proto>>`, Values
  point in. This list plus the string table is the future GC root set.

### 2. Nested compilation

Function literal spins up a fresh function context sharing source/ast/strtable.
Push/pop a FuncState instead of recursing with a whole new Compiler; too much
state lives in Compiler members to thread through constructors.

```cpp
struct FuncState {
  Block block;
  uint32_t next_reg = 0;
  std::vector<Local> locals;
  uint32_t depth = 0;
};
// Compiler holds std::vector<FuncState> and the current one is .back()
```

Syntax leaning: function literal as an expression, fits everything-is-an-expression:

```
add := fn(a, b) { a + b };
```

### 3. Frames in the VM

Register windows over one big register stack, Lua style.

```cpp
struct Frame {
  uint32_t return_pc;
  uint32_t base;        // window start into regs_
  uint32_t result_reg;  // caller register that receives the return value
  const Proto *proto;
};
std::vector<Frame> frames_;
```

- every `regs_[n]` in the interpreter loop becomes `regs_[base + n]`
- main script is frame 0 with base 0, so nothing observable changes until the
  first call happens

```
Call  A  B  C    ; A = reg holding the function, args at A+1 .. A+B
```

```cpp
case Code::Op::Call: {
  // callee window starts where the args already are: base + A + 1
  // args are its registers 0..B-1 for free, this is why consecutive
  // args from phase 1 pay off
  frames_.push_back({pc_, base + Code::GetA(i) + 1, base + Code::GetA(i), proto});
  pc_ = 0; // switch executing block to proto->block
} break;
```

`Ret` reworked: copy result into `frames_.back().result_reg`, pop the frame,
restore pc and block. Empty frame stack means program result (what MockRun
returns today).

### 4. Guards

- [ ] frame depth cap, throw RunTimeError "stack overflow" instead of walking off regs_
- [ ] regs_ becomes growable or gets a high water check per call (base + max_regs <= capacity)

