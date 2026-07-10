# Anvil TODO

- **Scoping: compile-time resolution, locals are registers.** Lua/clox-bytecode
  style, NOT the jlox `Environment`-with-`enclosing` runtime chain. Compiler
  keeps `vector<Local>{name_token, depth}` + `scope_depth_`; resolution walks
  backward (shadowing for free); undeclared variable = compile-time error.

## Next up: variables (`:=` declare, `=` reassign)

Parser:
- [ ] `Node::Kind::Ident` + `Identifier` case in `ParseAtom`
      (lexer already emits `Token::Kind::Identifier`; precedence table already
      has `ColonEqual`/`Equal` entries)
- [ ] Tests: declare, reassign, shadowing, use-in-expression

Compiler:
- [ ] `Local` struct + `locals_` vector + `scope_depth_`
- [ ] Declare: compile RHS, leave register allocated, push binding —
      the RHS register *is* the variable's slot
- [ ] Read: `ResolveLocal` walks `locals_` backward, returns register index
      directly (no load instruction needed)
- [ ] Reassign: compile RHS, `Move local_reg, rhs_reg`
- [ ] Scope enter/exit in `CompileBlock`: bump depth on entry; on exit pop
      locals at current depth, `FreeReg(locals_.size())`
- [ ] Block value crosses scope exit: `Move` last expression's value down to
      block base before popping locals (same dance as the `if` codegen)
- [ ] Per-statement temp cleanup: `FreeReg(locals_.size())` after each
      non-final statement in `CompileExpressions` (currently temporaries leak
      upward across statements)
- [ ] LHS validation: `Assign`/`Reassign` must have an `Ident` LHS —
      `1 + 2 = 3;` parses today, needs a compile error
- [ ] Undeclared-variable error (resolve fails) — needs a compiler error
      mechanism (currently only the parser collects errors)

## Open design questions

- **Redeclaration in same scope**: `x := 1; x := 2;` — error (clox) or allowed
  shadowing anywhere (Lua)? Leaning error: friendlier, and the backward walk
  makes the same-depth duplicate check trivial.
- **Name comparison**: `Local` stores a token index; comparing means slicing
  source. Add a `string_view` compare (avoid `SliceFromToken`'s allocation).
- **Compiler error reporting**: parser has `errors_`; compiler currently only
  has `DieLoudly`. Variables introduce the first user-facing compile errors
  (undeclared var, bad assignment target) — need a real error path, not abort.
- **What is `Test`'s truthiness for Void/non-bool values?** Matters once
  variables let arbitrary values reach `if` conditions.

## Known debts (deferred on purpose)

- Constant table dedup — every literal/void pushes a new constant
  (`1 + 1` stores `1` twice). Intern in `PushConstant` eventually; Void is the
  easy singleton case.
- `std::stoi`/`std::stod` throw on oversized literals — user input can crash
  the compiler. `std::from_chars` + a proper error.
- `CompileBinOp` switch duplication (own TODO in code) — table or helper.
- `ParseAtom` unexpected-token produces two errors per mistake
  (UnexpectedToken + suppressed follow-ons are handled, but the atom itself
  doesn't sync). Acceptable cascade for now.
- Multi-return (`ReturnMulti`) parses but compiler asserts — blocked on tuples.
- Bare `return;` unsupported — blocked on deciding void-function semantics;
  `CompileReturnSimple`'s `std::get<Node::Index>` will throw if parser ever
  emits monostate data. Emit `Ret` with B=1 when it lands.

## Later milestones

- **Functions**: call frames, per-function register windows (`next_reg_` is
  global now), `Ret` B field becomes real (result copy to caller frame),
  calling convention between `Call` and `Ret`, reachability ("did every path
  return?"), implicit last-value return falls out of Ruby rule.
- **Closures**: compile-time resolution means captured variables need
  upvalues (Lua mechanism). Known cost, standard road, pay when functions land.
- **Tuples / multi-value**: unblocks `ReturnMulti` (`return 1, 2;`).
- **Dead code elimination**: stray `Move`s after `Ret` in branches, code after
  guaranteed returns. Cosmetic until functions.
