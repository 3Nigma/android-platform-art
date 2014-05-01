/*
 * Copyright (C) 2013 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef ART_COMPILER_DEX_MIR_GRAPH_H_
#define ART_COMPILER_DEX_MIR_GRAPH_H_

#include <stdint.h>

#include "dex_file.h"
#include "dex_instruction.h"
#include "compiler_ir.h"
#include "invoke_type.h"
#include "mir_field_info.h"
#include "mir_method_info.h"
#include "utils/arena_bit_vector.h"
#include "utils/growable_array.h"
#include "reg_location.h"
#include "reg_storage.h"

namespace art {

class GlobalValueNumbering;

enum InstructionAnalysisAttributePos {
  kUninterestingOp = 0,
  kArithmeticOp,
  kFPOp,
  kSingleOp,
  kDoubleOp,
  kIntOp,
  kLongOp,
  kBranchOp,
  kInvokeOp,
  kArrayOp,
  kHeavyweightOp,
  kSimpleConstOp,
  kMoveOp,
  kSwitch
};

#define AN_NONE (1 << kUninterestingOp)
#define AN_MATH (1 << kArithmeticOp)
#define AN_FP (1 << kFPOp)
#define AN_LONG (1 << kLongOp)
#define AN_INT (1 << kIntOp)
#define AN_SINGLE (1 << kSingleOp)
#define AN_DOUBLE (1 << kDoubleOp)
#define AN_FLOATMATH (1 << kFPOp)
#define AN_BRANCH (1 << kBranchOp)
#define AN_INVOKE (1 << kInvokeOp)
#define AN_ARRAYOP (1 << kArrayOp)
#define AN_HEAVYWEIGHT (1 << kHeavyweightOp)
#define AN_SIMPLECONST (1 << kSimpleConstOp)
#define AN_MOVE (1 << kMoveOp)
#define AN_SWITCH (1 << kSwitch)
#define AN_COMPUTATIONAL (AN_MATH | AN_ARRAYOP | AN_MOVE | AN_SIMPLECONST)

enum DataFlowAttributePos {
  kUA = 0,
  kUB,
  kUC,
  kAWide,
  kBWide,
  kCWide,
  kDA,
  kIsMove,
  kSetsConst,
  kFormat35c,
  kFormat3rc,
  kFormatExtended,       // Extended format for extended MIRs.
  kNullCheckSrc0,        // Null check of uses[0].
  kNullCheckSrc1,        // Null check of uses[1].
  kNullCheckSrc2,        // Null check of uses[2].
  kNullCheckOut0,        // Null check out outgoing arg0.
  kDstNonNull,           // May assume dst is non-null.
  kRetNonNull,           // May assume retval is non-null.
  kNullTransferSrc0,     // Object copy src[0] -> dst.
  kNullTransferSrcN,     // Phi null check state transfer.
  kRangeCheckSrc1,       // Range check of uses[1].
  kRangeCheckSrc2,       // Range check of uses[2].
  kRangeCheckSrc3,       // Range check of uses[3].
  kFPA,
  kFPB,
  kFPC,
  kCoreA,
  kCoreB,
  kCoreC,
  kRefA,
  kRefB,
  kRefC,
  kUsesMethodStar,       // Implicit use of Method*.
  kUsesIField,           // Accesses an instance field (IGET/IPUT).
  kUsesSField,           // Accesses a static field (SGET/SPUT).
  kDoLVN,                // Worth computing local value numbers.
};

#define DF_NOP                  UINT64_C(0)
#define DF_UA                   (UINT64_C(1) << kUA)
#define DF_UB                   (UINT64_C(1) << kUB)
#define DF_UC                   (UINT64_C(1) << kUC)
#define DF_A_WIDE               (UINT64_C(1) << kAWide)
#define DF_B_WIDE               (UINT64_C(1) << kBWide)
#define DF_C_WIDE               (UINT64_C(1) << kCWide)
#define DF_DA                   (UINT64_C(1) << kDA)
#define DF_IS_MOVE              (UINT64_C(1) << kIsMove)
#define DF_SETS_CONST           (UINT64_C(1) << kSetsConst)
#define DF_FORMAT_35C           (UINT64_C(1) << kFormat35c)
#define DF_FORMAT_3RC           (UINT64_C(1) << kFormat3rc)
#define DF_FORMAT_EXTENDED      (UINT64_C(1) << kFormatExtended)
#define DF_NULL_CHK_0           (UINT64_C(1) << kNullCheckSrc0)
#define DF_NULL_CHK_1           (UINT64_C(1) << kNullCheckSrc1)
#define DF_NULL_CHK_2           (UINT64_C(1) << kNullCheckSrc2)
#define DF_NULL_CHK_OUT0        (UINT64_C(1) << kNullCheckOut0)
#define DF_NON_NULL_DST         (UINT64_C(1) << kDstNonNull)
#define DF_NON_NULL_RET         (UINT64_C(1) << kRetNonNull)
#define DF_NULL_TRANSFER_0      (UINT64_C(1) << kNullTransferSrc0)
#define DF_NULL_TRANSFER_N      (UINT64_C(1) << kNullTransferSrcN)
#define DF_RANGE_CHK_1          (UINT64_C(1) << kRangeCheckSrc1)
#define DF_RANGE_CHK_2          (UINT64_C(1) << kRangeCheckSrc2)
#define DF_RANGE_CHK_3          (UINT64_C(1) << kRangeCheckSrc3)
#define DF_FP_A                 (UINT64_C(1) << kFPA)
#define DF_FP_B                 (UINT64_C(1) << kFPB)
#define DF_FP_C                 (UINT64_C(1) << kFPC)
#define DF_CORE_A               (UINT64_C(1) << kCoreA)
#define DF_CORE_B               (UINT64_C(1) << kCoreB)
#define DF_CORE_C               (UINT64_C(1) << kCoreC)
#define DF_REF_A                (UINT64_C(1) << kRefA)
#define DF_REF_B                (UINT64_C(1) << kRefB)
#define DF_REF_C                (UINT64_C(1) << kRefC)
#define DF_UMS                  (UINT64_C(1) << kUsesMethodStar)
#define DF_IFIELD               (UINT64_C(1) << kUsesIField)
#define DF_SFIELD               (UINT64_C(1) << kUsesSField)
#define DF_LVN                  (UINT64_C(1) << kDoLVN)

#define DF_HAS_USES             (DF_UA | DF_UB | DF_UC)

#define DF_HAS_DEFS             (DF_DA)

#define DF_HAS_NULL_CHKS        (DF_NULL_CHK_0 | \
                                 DF_NULL_CHK_1 | \
                                 DF_NULL_CHK_2 | \
                                 DF_NULL_CHK_OUT0)

#define DF_HAS_RANGE_CHKS       (DF_RANGE_CHK_1 | \
                                 DF_RANGE_CHK_2 | \
                                 DF_RANGE_CHK_3)

#define DF_HAS_NR_CHKS          (DF_HAS_NULL_CHKS | \
                                 DF_HAS_RANGE_CHKS)

#define DF_A_IS_REG             (DF_UA | DF_DA)
#define DF_B_IS_REG             (DF_UB)
#define DF_C_IS_REG             (DF_UC)
#define DF_IS_GETTER_OR_SETTER  (DF_IS_GETTER | DF_IS_SETTER)
#define DF_USES_FP              (DF_FP_A | DF_FP_B | DF_FP_C)
#define DF_NULL_TRANSFER        (DF_NULL_TRANSFER_0 | DF_NULL_TRANSFER_N)
enum OatMethodAttributes {
  kIsLeaf,            // Method is leaf.
  kHasLoop,           // Method contains simple loop.
};

#define METHOD_IS_LEAF          (1 << kIsLeaf)
#define METHOD_HAS_LOOP         (1 << kHasLoop)

// Minimum field size to contain Dalvik v_reg number.
#define VREG_NUM_WIDTH 16

#define INVALID_SREG (-1)
#define INVALID_VREG (0xFFFFU)
#define INVALID_OFFSET (0xDEADF00FU)

#define MIR_IGNORE_NULL_CHECK           (1 << kMIRIgnoreNullCheck)
#define MIR_NULL_CHECK_ONLY             (1 << kMIRNullCheckOnly)
#define MIR_IGNORE_RANGE_CHECK          (1 << kMIRIgnoreRangeCheck)
#define MIR_RANGE_CHECK_ONLY            (1 << kMIRRangeCheckOnly)
#define MIR_IGNORE_CLINIT_CHECK         (1 << kMIRIgnoreClInitCheck)
#define MIR_INLINED                     (1 << kMIRInlined)
#define MIR_INLINED_PRED                (1 << kMIRInlinedPred)
#define MIR_CALLEE                      (1 << kMIRCallee)
#define MIR_IGNORE_SUSPEND_CHECK        (1 << kMIRIgnoreSuspendCheck)
#define MIR_DUP                         (1 << kMIRDup)

#define BLOCK_NAME_LEN 80

typedef uint16_t BasicBlockId;
static const BasicBlockId NullBasicBlockId = 0;
static constexpr bool kLeafOptimization = false;

/*
 * In general, vreg/sreg describe Dalvik registers that originated with dx.  However,
 * it is useful to have compiler-generated temporary registers and have them treated
 * in the same manner as dx-generated virtual registers.  This struct records the SSA
 * name of compiler-introduced temporaries.
 */
struct CompilerTemp {
  int32_t v_reg;      // Virtual register number for temporary.
  int32_t s_reg_low;  // SSA name for low Dalvik word.
};

enum CompilerTempType {
  kCompilerTempVR,                // A virtual register temporary.
  kCompilerTempSpecialMethodPtr,  // Temporary that keeps track of current method pointer.
};

// When debug option enabled, records effectiveness of null and range check elimination.
struct Checkstats {
  int32_t null_checks;
  int32_t null_checks_eliminated;
  int32_t range_checks;
  int32_t range_checks_eliminated;
};

// Dataflow attributes of a basic block.
struct BasicBlockDataFlow {
  ArenaBitVector* use_v;
  ArenaBitVector* def_v;
  ArenaBitVector* live_in_v;
  ArenaBitVector* phi_v;
  int32_t* vreg_to_ssa_map_exit;
  ArenaBitVector* ending_check_v;  // For null check and class init check elimination.
};

/*
 * Normalized use/def for a MIR operation using SSA names rather than vregs.  Note that
 * uses/defs retain the Dalvik convention that long operations operate on a pair of 32-bit
 * vregs.  For example, "ADD_LONG v0, v2, v3" would have 2 defs (v0/v1) and 4 uses (v2/v3, v4/v5).
 * Following SSA renaming, this is the primary struct used by code generators to locate
 * operand and result registers.  This is a somewhat confusing and unhelpful convention that
 * we may want to revisit in the future.
 *
 * TODO:
 *  1. Add accessors for uses/defs and make data private
 *  2. Change fp_use/fp_def to a bit array (could help memory usage)
 *  3. Combine array storage into internal array and handled via accessors from 1.
 */
struct SSARepresentation {
  int32_t* uses;
  bool* fp_use;
  int32_t* defs;
  bool* fp_def;
  int16_t num_uses_allocated;
  int16_t num_defs_allocated;
  int16_t num_uses;
  int16_t num_defs;

  static uint32_t GetStartUseIndex(Instruction::Code opcode);
};

/*
 * The Midlevel Intermediate Representation node, which may be largely considered a
 * wrapper around a Dalvik byte code.
 */
struct MIR {
  /*
   * TODO: remove embedded DecodedInstruction to save space, keeping only opcode.  Recover
   * additional fields on as-needed basis.  Question: how to support MIR Pseudo-ops; probably
   * need to carry aux data pointer.
   */
  struct DecodedInstruction {
    uint32_t vA;
    uint32_t vB;
    uint64_t vB_wide;        /* for k51l */
    uint32_t vC;
    uint32_t arg[5];         /* vC/D/E/F/G in invoke or filled-new-array */
    Instruction::Code opcode;

    explicit DecodedInstruction():vA(0), vB(0), vB_wide(0), vC(0), opcode(Instruction::NOP) {
    }

    /*
     * Given a decoded instruction representing a const bytecode, it updates
     * the out arguments with proper values as dictated by the constant bytecode.
     */
    bool GetConstant(int64_t* ptr_value, bool* wide) const;

    static bool IsPseudoMirOp(Instruction::Code opcode) {
      return static_cast<int>(opcode) >= static_cast<int>(kMirOpFirst);
    }

    static bool IsPseudoMirOp(int opcode) {
      return opcode >= static_cast<int>(kMirOpFirst);
    }

    bool IsInvoke() const {
      return !IsPseudoMirOp(opcode) && ((Instruction::FlagsOf(opcode) & Instruction::kInvoke) == Instruction::kInvoke);
    }

    bool IsStore() const {
      return !IsPseudoMirOp(opcode) && ((Instruction::FlagsOf(opcode) & Instruction::kStore) == Instruction::kStore);
    }

    bool IsLoad() const {
      return !IsPseudoMirOp(opcode) && ((Instruction::FlagsOf(opcode) & Instruction::kLoad) == Instruction::kLoad);
    }

    bool IsConditionalBranch() const {
      return !IsPseudoMirOp(opcode) && (Instruction::FlagsOf(opcode) == (Instruction::kContinue | Instruction::kBranch));
    }

    /**
     * @brief Is the register C component of the decoded instruction a constant?
     */
    bool IsCFieldOrConstant() const {
      return !IsPseudoMirOp(opcode) && ((Instruction::FlagsOf(opcode) & Instruction::kRegCFieldOrConstant) == Instruction::kRegCFieldOrConstant);
    }

    /**
     * @brief Is the register C component of the decoded instruction a constant?
     */
    bool IsBFieldOrConstant() const {
      return !IsPseudoMirOp(opcode) && ((Instruction::FlagsOf(opcode) & Instruction::kRegBFieldOrConstant) == Instruction::kRegBFieldOrConstant);
    }

    bool IsCast() const {
      return !IsPseudoMirOp(opcode) && ((Instruction::FlagsOf(opcode) & Instruction::kCast) == Instruction::kCast);
    }

    /**
     * @brief Does the instruction clobber memory?
     * @details Clobber means that the instruction changes the memory not in a punctual way.
     *          Therefore any supposition on memory aliasing or memory contents should be disregarded
     *            when crossing such an instruction.
     */
    bool Clobbers() const {
      return !IsPseudoMirOp(opcode) && ((Instruction::FlagsOf(opcode) & Instruction::kClobber) == Instruction::kClobber);
    }

    bool IsLinear() const {
      return !IsPseudoMirOp(opcode) && (Instruction::FlagsOf(opcode) & (Instruction::kAdd | Instruction::kSubtract)) != 0;
    }
  } dalvikInsn;

  NarrowDexOffset offset;         // Offset of the instruction in code units.
  uint16_t optimization_flags;
  int16_t m_unit_index;           // From which method was this MIR included
  BasicBlockId bb;
  MIR* next;
  SSARepresentation* ssa_rep;
  union {
    // Incoming edges for phi node.
    BasicBlockId* phi_incoming;
    // Establish link from check instruction (kMirOpCheck) to the actual throwing instruction.
    MIR* throw_insn;
    // Branch condition for fused cmp or select.
    ConditionCode ccode;
    // IGET/IPUT lowering info index, points to MIRGraph::ifield_lowering_infos_. Due to limit on
    // the number of code points (64K) and size of IGET/IPUT insn (2), this will never exceed 32K.
    uint32_t ifield_lowering_info;
    // SGET/SPUT lowering info index, points to MIRGraph::sfield_lowering_infos_. Due to limit on
    // the number of code points (64K) and size of SGET/SPUT insn (2), this will never exceed 32K.
    uint32_t sfield_lowering_info;
    // INVOKE data index, points to MIRGraph::method_lowering_infos_.
    uint32_t method_lowering_info;
  } meta;

  explicit MIR():offset(0), optimization_flags(0), m_unit_index(0), bb(NullBasicBlockId),
                 next(nullptr), ssa_rep(nullptr) {
    memset(&meta, 0, sizeof(meta));
  }

  uint32_t GetStartUseIndex() const {
    return SSARepresentation::GetStartUseIndex(dalvikInsn.opcode);
  }

  MIR* Copy(CompilationUnit *c_unit);
  MIR* Copy(MIRGraph* mir_Graph);

  static void* operator new(size_t size, ArenaAllocator* arena) {
    return arena->Alloc(sizeof(MIR), kArenaAllocMIR);
  }
  static void operator delete(void* p) {}  // Nop.
};

struct SuccessorBlockInfo;

struct BasicBlock {
  BasicBlockId id;
  BasicBlockId dfs_id;
  NarrowDexOffset start_offset;     // Offset in code units.
  BasicBlockId fall_through;
  BasicBlockId taken;
  BasicBlockId i_dom;               // Immediate dominator.
  uint16_t nesting_depth;
  BBType block_type:4;
  BlockListType successor_block_list_type:4;
  bool visited:1;
  bool hidden:1;
  bool catch_entry:1;
  bool explicit_throw:1;
  bool conditional_branch:1;
  bool terminated_by_return:1;  // Block ends with a Dalvik return opcode.
  bool dominates_return:1;      // Is a member of return extended basic block.
  bool use_lvn:1;               // Run local value numbering on this block.
  MIR* first_mir_insn;
  MIR* last_mir_insn;
  BasicBlockDataFlow* data_flow_info;
  ArenaBitVector* dominators;
  ArenaBitVector* i_dominated;      // Set nodes being immediately dominated.
  ArenaBitVector* dom_frontier;     // Dominance frontier.
  GrowableArray<BasicBlockId>* predecessors;
  GrowableArray<SuccessorBlockInfo*>* successor_blocks;

  void AppendMIR(MIR* mir);
  void AppendMIRList(MIR* first_list_mir, MIR* last_list_mir);
  void AppendMIRList(const std::vector<MIR*>& insns);
  void PrependMIR(MIR* mir);
  void PrependMIRList(MIR* first_list_mir, MIR* last_list_mir);
  void PrependMIRList(const std::vector<MIR*>& to_add);
  void InsertMIRAfter(MIR* current_mir, MIR* new_mir);
  void InsertMIRListAfter(MIR* insert_after, MIR* first_list_mir, MIR* last_list_mir);
  MIR* FindPreviousMIR(MIR* mir);
  void InsertMIRBefore(MIR* insert_before, MIR* list);
  void InsertMIRListBefore(MIR* insert_before, MIR* first_list_mir, MIR* last_list_mir);
  bool RemoveMIR(MIR* mir);
  bool RemoveMIRList(MIR* first_list_mir, MIR* last_list_mir);

  BasicBlock* Copy(CompilationUnit* c_unit);
  BasicBlock* Copy(MIRGraph* mir_graph);

  /**
   * @brief Reset the optimization_flags field of each MIR.
   */
  void ResetOptimizationFlags(uint16_t reset_flags);

  /**
   * @brief Hide the BasicBlock.
   * @details Set it to kDalvikByteCode, set hidden to true, remove all MIRs,
   *          remove itself from any predecessor edges, remove itself from any
   *          child's predecessor growable array.
   */
  void Hide(CompilationUnit* c_unit);

  /**
   * @brief Is ssa_reg the last SSA definition of that VR in the block?
   */
  bool IsSSALiveOut(const CompilationUnit* c_unit, int ssa_reg);

  /**
   * @brief Replace the edge going to old_bb to now go towards new_bb.
   */
  bool ReplaceChild(BasicBlockId old_bb, BasicBlockId new_bb);

  /**
   * @brief Update the predecessor growable array from old_pred to new_pred.
   */
  void UpdatePredecessor(BasicBlockId old_pred, BasicBlockId new_pred);

  /**
   * @brief Used to obtain the next MIR that follows unconditionally.
   * @details The implementation does not guarantee that a MIR does not
   * follow even if this method returns nullptr.
   * @param mir_graph the MIRGraph.
   * @param current The MIR for which to find an unconditional follower.
   * @return Returns the following MIR if one can be found.
   */
  MIR* GetNextUnconditionalMir(MIRGraph* mir_graph, MIR* current);
  bool IsExceptionBlock() const;

  static void* operator new(size_t size, ArenaAllocator* arena) {
    return arena->Alloc(sizeof(BasicBlock), kArenaAllocBB);
  }
  static void operator delete(void* p) {}  // Nop.
};

/*
 * The "blocks" field in "successor_block_list" points to an array of elements with the type
 * "SuccessorBlockInfo".  For catch blocks, key is type index for the exception.  For switch
 * blocks, key is the case value.
 */
struct SuccessorBlockInfo {
  BasicBlockId block;
  int key;
};

/**
 * @class ChildBlockIterator
 * @brief Enable an easy iteration of the children.
 */
class ChildBlockIterator {
 public:
  /**
   * @brief Constructs a child iterator.
   * @param bb The basic whose children we need to iterate through.
   * @param mir_graph The MIRGraph used to get the basic block during iteration.
   */
  ChildBlockIterator(BasicBlock* bb, MIRGraph* mir_graph);
  BasicBlock* Next();

 private:
  BasicBlock* basic_block_;
  MIRGraph* mir_graph_;
  bool visited_fallthrough_;
  bool visited_taken_;
  bool have_successors_;
  GrowableArray<SuccessorBlockInfo*>::Iterator successor_iter_;
};

/*
 * Collection of information describing an invoke, and the destination of
 * the subsequent MOVE_RESULT (if applicable).  Collected as a unit to enable
 * more efficient invoke code generation.
 */
struct CallInfo {
  int num_arg_words;    // Note: word count, not arg count.
  RegLocation* args;    // One for each word of arguments.
  RegLocation result;   // Eventual target of MOVE_RESULT.
  int opt_flags;
  InvokeType type;
  uint32_t dex_idx;
  uint32_t index;       // Method idx for invokes, type idx for FilledNewArray.
  uintptr_t direct_code;
  uintptr_t direct_method;
  RegLocation target;    // Target of following move_result.
  bool skip_this;
  bool is_range;
  DexOffset offset;      // Offset in code units.
  MIR* mir;
};


const RegLocation bad_loc = {kLocDalvikFrame, 0, 0, 0, 0, 0, 0, 0, 0, RegStorage(), INVALID_SREG,
                             INVALID_SREG};

/**
 * @brief Abstraction that is used to represent the control flow graph of a method.
 */
class ControlFlowGraph {
 public:
  /**
   * @brief Used to construct a control flow graph from a code item.
   * @param arena The arena to use for allocations. If blocks will be merged
   * into MIRGraph, it makes sense to use its arena.
   * @param code_item The code item for which to create the CFG.
   * @param start_offset The start offset at which to create the CFG. In normal cases
   * this should be zero to handle the whole method.
   * @param supress_exception_edges Whether exception edges should be supressed.
   */
  explicit ControlFlowGraph(ArenaAllocator* arena, const DexFile::CodeItem* code_item,
                            DexOffset start_offset, bool supress_exception_edges);

  /**
   * @return Returns the entry block.
   */
  BasicBlock* GetEntryBlock() const {
    return entry_block_;
  }

  /**
   * @return Returns the exit block.
   */
  BasicBlock* GetExitBlock() const {
    return exit_block_;
  }

  /**
   * @brief Used to obtain the single return mir of the CFG.
   * @details Caller must guarantee that there is just one by calling GetNumReturns.
   * @return Returns the return mir.
   */
  MIR* GetReturnMir() const {
    CHECK_EQ(return_mirs_.size(), 1u);
    return return_mirs_[0];
  }

  /**
   * @return Returns the number of return mirs in graph.
   */
  size_t GetNumReturns() const {
    return return_mirs_.size();
  }

  /**
   * @return Returns a list of all the return mirs.
   */
  std::vector<MIR*>& GetReturnMirs() {
    return return_mirs_;
  }

  /**
   * @return Returns the bitvector that represents offsets in try blocks.
   */
  ArenaBitVector* GetTryBlockAddr() const {
    return try_block_addr_;
  }

  /**
   * @return Returns the list of all of the blocks.
   */
  GrowableArray<BasicBlock*>& GetBlockList() {
    return block_list_;
  }

  /**
   * @param block_id The id of the block to get.
   * @return Returns the basic block with the given id.
   */
  BasicBlock* GetBasicBlock(BasicBlockId block_id) const {
    CHECK_LT(block_id, block_list_.Size());
    return (block_id == NullBasicBlockId) ? nullptr : block_list_.Get(block_id);
  }

  /**
   * @return Returns the number of virtual registers in method represented by graph.
   */
  uint32_t GetNumVRegs() const {
    return num_vregs_;
  }

  /**
   * @return Returns the number of "in" VRs in method represented by graph.
   */
  uint32_t GetNumIns() const {
    return num_ins_;
  }

  /**
   * @return Returns the number of bytecodes in graph.
   */
  uint32_t GetNumBytecodes() const {
    return num_bytecodes_;
  }

  /**
   * @return Returns the VR name of the first "in".
   */
  uint32_t GetFirstInNumber() const {
    return GetNumVRegs() - GetNumIns();
  }

  const DexFile::CodeItem* GetCodeItem() const {
    return current_code_item_;
  }

  /**
   * @brief Used to allocate a new empty basic block.
   * @param arena The arena in which to allocate it.
   * @param block_type The block type to create.
   * @param block_id The id of the new block.
   * @return Returns the newly created block.
   */
  static BasicBlock* NewMemBB(ArenaAllocator* arena, BBType block_type, int block_id);

  /**
   * @brief Used to allocate a new empty MIR.
   * @param arena The arena to use for allocation.
   * @return Returns the newly created MIR.
   */
  static MIR* NewMIR(ArenaAllocator* arena);

 private:
  /**
   * @brief Used to decode a MIR instruction pointed to.
   * @param code_ptr The code pointer at which to decode instruction.
   * @param decoded_instruction Updated by method to contain decoded instruction.
   * @return Returns the length of the instruction.
   */
  int ParseInsn(const uint16_t* code_ptr, MIR::DecodedInstruction* decoded_instruction);

  /**
   * @brief Used to find a block at the requested code offset.
   * @param code_offset The offset to look for.
   * @param split If offset is in middle of block, this represents if split can happen.
   * @param create If the block does not exist, this represents if it can be created.
   * @param immed_pred_block_p If provided, it will update pointer to hold newly created block
   * after split.
   * @return Returns the block with the corresponding offset if one was found.
   */
  BasicBlock* FindBlock(DexOffset code_offset, bool split, bool create, BasicBlock** immed_pred_block_p);

  /**
   * @brief Used to split a block into two.
   * @param code_offset Represents the offset at which the split should happen.
   * @param orig_block The original block to split.
   * @param immed_pred_block_p If provided, it will update pointed to hold newly created block
   * after split.
   * @return Returns the newly create block after split.
   */
  BasicBlock* SplitBlock(DexOffset code_offset, BasicBlock* orig_block, BasicBlock** immed_pred_block_p);

  bool IsBadMonitorExitCatch(NarrowDexOffset monitor_exit_offset, NarrowDexOffset catch_offset);

  /**
   * @brief Used to process all of the try/catch information for method.
   * @details try_block_addr_ is updated.
   */
  void ProcessTryCatchBlocks();

  /**
   * @brief Used to handle a branching MIR.
   * @param cur_block The current block being handled.
   * @param insn The branching MIR.
   * @param cur_offset The offset of the branching MIR.
   * @param width The width of the branching MIR.
   * @param flags Dex flags of branching MIR.
   * @param code_ptr The current code pointer of the dex code item.
   * @param code_end The end code pointer of the dex code item.
   * @return Returns current block (which may be updated after a split).
   */
  BasicBlock* ProcessCanBranch(BasicBlock* cur_block, MIR* insn, DexOffset cur_offset, int width,
                               int flags, const uint16_t* code_ptr, const uint16_t* code_end);

  /**
   * @brief Used to handle a switching MIR.
   * @param cur_block The current block being handled.
   * @param insn The switching MIR.
   * @param cur_offset The offset of the switching MIR.
   * @param width The width of the switching MIR.
   * @param flags Dex flags of switching MIR.
   * @return Returns current block (which may be updated after a split).
   */
  BasicBlock* ProcessCanSwitch(BasicBlock* cur_block, MIR* insn, DexOffset cur_offset, int width,
                               int flags);

  /**
   * @brief Used to handle a throwing MIR.
   * @param cur_block The current block being handled.
   * @param insn The throwing MIR.
   * @param cur_offset The offset of the throwing MIR.
   * @param width The width of the throwing MIR.
   * @param flags Dex flags of throwing MIR.
   * @param try_block_addr The bitvector that represents whether offset is in try block.
   * @param code_ptr The current code pointer of the dex code item.
   * @param code_end The end code pointer of the dex code item.
   * @param supress_exception_edges Whether the exception edges should be supressed.
   * @return Returns current block (which may be updated after a split).
   */
  BasicBlock* ProcessCanThrow(BasicBlock* cur_block, MIR* insn, DexOffset cur_offset, int width,
                              int flags, ArenaBitVector* try_block_addr, const uint16_t* code_ptr,
                              const uint16_t* code_end, bool supress_exception_edges);

  /**
   * @brief Used to create a block and automatically give it an id and insert in block list.
   * @param block_type The block type to create.
   * @return Returns the newly created block.
   */
  BasicBlock* CreateNewBB(BBType block_type) {
    BasicBlock* new_block = NewMemBB(arena_, block_type, block_list_.Size());
    block_list_.Insert(new_block);
    return new_block;
  }

  /**
   * @brief Helper to update fallthrough of a block.
   * @param parent The parent whose child to update.
   * @param new_child The new child to set as fallthrough.
   */
  void UpdateFallthrough(BasicBlock *parent, BasicBlock *new_child) {
    BasicBlock *old_child = GetBasicBlock(parent->fall_through);
    parent->fall_through = new_child->id;
    new_child->predecessors->Insert(parent->id);
    if (old_child != nullptr) {
      old_child->predecessors->Delete(parent->id);
    }
  }

  /**
   * @brief Helper to update taken of a block.
   * @param parent The parent whose child to update.
   * @param new_child The new child to set as taken.
   */
  void UpdateTaken(BasicBlock *parent, BasicBlock *new_child) {
    BasicBlock *old_child = GetBasicBlock(parent->taken);
    parent->taken = new_child->id;
    new_child->predecessors->Insert(parent->id);
    if (old_child != nullptr) {
      old_child->predecessors->Delete(parent->id);
    }
  }

  /**
   * @brief The single entry block.
   */
  BasicBlock* entry_block_;

  /**
   * @brief The single exit block.
   */
  BasicBlock* exit_block_;

  /**
   * @brief List of all of the return mirs.
   */
  std::vector<MIR*> return_mirs_;

  /**
   * @brief The code item (from dex file) that was used to create CFG.
   */
  const DexFile::CodeItem* current_code_item_;

  /**
   * @brief The arena used to allocate blocks and MIRs.
   */
  ArenaAllocator* arena_;

  /**
   * @brief Bitvector in which the bits set represent the dex offset. If bit is
   * set, it means that offset is in try block.
   */
  ArenaBitVector* try_block_addr_;

  /**
   * @brief Represents map of dex offset to corresponding block that contains it.
   */
  SafeMap<DexOffset, BasicBlock*> dex_pc_to_block_map_;

  /**
   * @brief Keeps track of block list of the control flow graph.
   */
  GrowableArray<BasicBlock*> block_list_;

  uint16_t num_vregs_;      /** !< Number of virtual registers of method represented by graph. */
  uint16_t num_ins_;        /** !< Number of ins of method represented by graph. */
  uint32_t num_bytecodes_;  /** !< Number of bytecodes in graph. */
};

class MIRGraph {
 public:
  MIRGraph(CompilationUnit* cu, ArenaAllocator* arena);
  ~MIRGraph();

  /*
   * Examine the graph to determine whether it's worthwile to spend the time compiling
   * this method.
   */
  bool SkipCompilation(std::string* skip_message);

  /*
   * Should we skip the compilation of this method based on its name?
   */
  bool SkipCompilationByName(const std::string& methodname);

  /*
   * Parse dex method and add MIR at current insert point.  Returns id (which is
   * actually the index of the method in the m_units_ array).
   */
  void IntegrateMethod(const DexFile::CodeItem* code_item, uint32_t access_flags,
                    InvokeType invoke_type, uint16_t class_def_idx,
                    uint32_t method_idx, jobject class_loader, const DexFile& dex_file);

  /**
   * @brief Used to merge a new method with its CFG into current MIRGraph.
   * @details All of the key MIRGraph data structures are updated to reflect the
   * newly integrated CFG. However, one key aspect of MergeCFG is that it does
   * not actually insert in a way that it is makes the blocks reachable (unless
   * it is the first method inserted). Thus, an inliner would have to explicitly
   * create those links.
   * @param m_unit The dex compilation unit for method.
   * @param control_flow_graph The control flow graph of method to integrate.
   * @return Returns whether integration was successful.
   */
  bool MergeCFG(DexCompilationUnit* m_unit, ControlFlowGraph& control_flow_graph);

  /* Find existing block */
  BasicBlock* FindBlock(DexOffset code_offset) {
    GrowableArray<BasicBlock*>::Iterator iter(&block_list_);
    for (BasicBlock* bb = iter.Next(); bb != NULL; bb = iter.Next()) {
      if (bb->start_offset == code_offset) {
        return bb;
      }
    }
    return nullptr;
  }

  const uint16_t* GetInsns(int m_unit_index) const {
    return GetCodeItem(m_unit_index)->insns_;
  }

  DexOffset GetDexCompilationUnitStartOffset(int m_unit_index) const {
    return m_unit_to_start_offset[m_unit_index];
  }

  /**
   * @brief This is used to get a MIR's original offset from its dex file.
   * @details This exists because integrating multiple methods is done by making
   * sure that the offsets are not overlapping (for better MIR tracking).
   * @param mir The mir whose original offset to get.
   * @return Returns the original offset from dex file.
   */
  DexOffset GetMirOriginalOffset(const MIR* mir) const {
    DexOffset real_offset = mir->offset - GetDexCompilationUnitStartOffset(mir->m_unit_index);
    return real_offset;
  }

  const Instruction* GetInstructionFor(const MIR* mir) const {
    return Instruction::At(GetInsns(mir->m_unit_index) + GetMirOriginalOffset(mir));
  }

  unsigned int GetNumBlocks() const {
    return num_blocks_;
  }

  /**
   * @brief Includes all methods in compilation unit when providing the num dalvik instructions.
   * @return Returns the cumulative sum of all insn sizes (in code units).
   */
  size_t GetNumDalvikInsns() const {
    size_t cumulative_size = 0u;
    for (auto it : m_units_) {
      const DexFile::CodeItem* code_item = it->GetCodeItem();
      cumulative_size += (code_item == nullptr ? 0 : code_item->insns_size_in_code_units_);
    }
    return cumulative_size;
  }

  /**
   * @brief Used to determine if MIR is in try block.
   * @param mir The mir to check.
   * @return Returns true if MIR is in a try block.
   */
  bool IsMirInTryBlock(MIR* mir) {
    return m_unit_to_try_block_addr[mir->m_unit_index]->IsBitSet(GetMirOriginalOffset(mir));
  }

  BasicBlock* GetEntryBlock() const {
    return entry_block_;
  }

  BasicBlock* GetExitBlock() const {
    return exit_block_;
  }

  BasicBlock* GetBasicBlock(unsigned int block_id) const {
    return (block_id == NullBasicBlockId) ? NULL : block_list_.Get(block_id);
  }

  size_t GetBasicBlockListCount() const {
    return block_list_.Size();
  }

  GrowableArray<BasicBlock*>* GetBlockList() {
    return &block_list_;
  }

  GrowableArray<BasicBlockId>* GetDfsOrder() {
    return dfs_order_;
  }

  GrowableArray<BasicBlockId>* GetDfsPostOrder() {
    return dfs_post_order_;
  }

  GrowableArray<BasicBlockId>* GetDomPostOrder() {
    return dom_post_order_traversal_;
  }

  int GetDefCount() const {
    return def_count_;
  }

  ArenaAllocator* GetArena() {
    return arena_;
  }

  void EnableOpcodeCounting() {
    opcode_count_ = static_cast<int*>(arena_->Alloc(kNumPackedOpcodes * sizeof(int),
                                                    kArenaAllocMisc));
  }

  void ShowOpcodeStats();

  DexCompilationUnit* GetCurrentDexCompilationUnit() const {
    // The callers of this really expect that they obtain the compilation unit
    // for the single method in compilation unit. Eventually when multiple
    // methods can reside, the caller should get the appropriate dex compilation
    // unit as the context indicates.
    return GetDexCompilationUnit(0);
  }

  DexCompilationUnit* GetDexCompilationUnit(uint16_t m_unit_index) const {
    return m_units_[m_unit_index];
  }

  DexCompilationUnit* GetDexCompilationUnitByMir(MIR* mir) const {
    return GetDexCompilationUnit(mir->m_unit_index);
  }

  const DexFile::CodeItem* GetCodeItem(uint16_t m_unit_index) const {
    return GetDexCompilationUnit(m_unit_index)->GetCodeItem();
  }

  uint16_t GetNumMethodsInlined() {
    if (m_units_.size() == 0) {
      return 0;
    }
    return m_units_.size() - 1;
  }

  /**
   * @brief Dump a CFG into a dot file format.
   * @param dir_prefix the directory the file will be created in.
   * @param all_blocks does the dumper use all the basic blocks or use the reachable blocks.
   * @param suffix does the filename require a suffix or not (default = nullptr).
   */
  void DumpCFG(const char* dir_prefix, bool all_blocks, const char* suffix = nullptr);

  bool HasFieldAccess() const {
    return (merged_df_flags_ & (DF_IFIELD | DF_SFIELD)) != 0u;
  }

  bool HasStaticFieldAccess() const {
    return (merged_df_flags_ & DF_SFIELD) != 0u;
  }

  bool HasInvokes() const {
    // NOTE: These formats include the rare filled-new-array/range.
    return (merged_df_flags_ & (DF_FORMAT_35C | DF_FORMAT_3RC)) != 0u;
  }

  void DoCacheFieldLoweringInfo();

  const MirIFieldLoweringInfo& GetIFieldLoweringInfo(MIR* mir) const {
    DCHECK_LT(mir->meta.ifield_lowering_info, ifield_lowering_infos_.Size());
    return ifield_lowering_infos_.GetRawStorage()[mir->meta.ifield_lowering_info];
  }

  const MirSFieldLoweringInfo& GetSFieldLoweringInfo(MIR* mir) const {
    DCHECK_LT(mir->meta.sfield_lowering_info, sfield_lowering_infos_.Size());
    return sfield_lowering_infos_.GetRawStorage()[mir->meta.sfield_lowering_info];
  }

  void DoCacheMethodLoweringInfo();

  const MirMethodLoweringInfo& GetMethodLoweringInfo(MIR* mir) {
    DCHECK_LT(mir->meta.method_lowering_info, method_lowering_infos_.Size());
    return method_lowering_infos_.GetRawStorage()[mir->meta.method_lowering_info];
  }

  void ComputeInlineIFieldLoweringInfo(uint16_t field_idx, MIR* invoke, MIR* iget_or_iput);

  void InitRegLocations();

  void RemapRegLocations();

  void DumpRegLocTable(RegLocation* table, int count);

  void BasicBlockOptimization();

  GrowableArray<BasicBlockId>* GetTopologicalSortOrder() {
    DCHECK(topological_order_ != nullptr);
    return topological_order_;
  }

  bool IsConst(int32_t s_reg) const {
    return is_constant_v_->IsBitSet(s_reg);
  }

  bool IsConst(RegLocation loc) const {
    return loc.orig_sreg < 0 ? false : IsConst(loc.orig_sreg);
  }

  int32_t ConstantValue(RegLocation loc) const {
    DCHECK(IsConst(loc));
    return constant_values_[loc.orig_sreg];
  }

  int32_t ConstantValue(int32_t s_reg) const {
    DCHECK(IsConst(s_reg));
    return constant_values_[s_reg];
  }

  int64_t ConstantValueWide(RegLocation loc) const {
    DCHECK(IsConst(loc));
    return (static_cast<int64_t>(constant_values_[loc.orig_sreg + 1]) << 32) |
        Low32Bits(static_cast<int64_t>(constant_values_[loc.orig_sreg]));
  }

  bool IsConstantNullRef(RegLocation loc) const {
    return loc.ref && loc.is_const && (ConstantValue(loc) == 0);
  }

  int GetNumSSARegs() const {
    return num_ssa_regs_;
  }

  void SetNumSSARegs(int new_num) {
     /*
      * TODO: It's theoretically possible to exceed 32767, though any cases which did
      * would be filtered out with current settings.  When orig_sreg field is removed
      * from RegLocation, expand s_reg_low to handle all possible cases and remove DCHECK().
      */
    DCHECK_EQ(new_num, static_cast<int16_t>(new_num));
    num_ssa_regs_ = new_num;
  }

  unsigned int GetNumReachableBlocks() const {
    return num_reachable_blocks_;
  }

  int GetUseCount(int vreg) const {
    return use_counts_.Get(vreg);
  }

  int GetRawUseCount(int vreg) const {
    return raw_use_counts_.Get(vreg);
  }

  int GetSSASubscript(int ssa_reg) const {
    return ssa_subscripts_->Get(ssa_reg);
  }

  RegLocation GetRawSrc(MIR* mir, int num) {
    DCHECK(num < mir->ssa_rep->num_uses);
    RegLocation res = reg_location_[mir->ssa_rep->uses[num]];
    return res;
  }

  RegLocation GetRawDest(MIR* mir) {
    DCHECK_GT(mir->ssa_rep->num_defs, 0);
    RegLocation res = reg_location_[mir->ssa_rep->defs[0]];
    return res;
  }

  RegLocation GetDest(MIR* mir) {
    RegLocation res = GetRawDest(mir);
    DCHECK(!res.wide);
    return res;
  }

  RegLocation GetSrc(MIR* mir, int num) {
    RegLocation res = GetRawSrc(mir, num);
    DCHECK(!res.wide);
    return res;
  }

  RegLocation GetDestWide(MIR* mir) {
    RegLocation res = GetRawDest(mir);
    DCHECK(res.wide);
    return res;
  }

  RegLocation GetSrcWide(MIR* mir, int low) {
    RegLocation res = GetRawSrc(mir, low);
    DCHECK(res.wide);
    return res;
  }

  RegLocation GetBadLoc() {
    return bad_loc;
  }

  int GetMethodSReg() const {
    return method_sreg_;
  }

  /**
   * @brief Used to obtain the number of compiler temporaries being used.
   * @return Returns the number of compiler temporaries.
   */
  size_t GetNumUsedCompilerTemps() const {
    size_t total_num_temps = compiler_temps_.Size();
    DCHECK_LE(num_non_special_compiler_temps_, total_num_temps);
    return total_num_temps;
  }

  /**
   * @brief Used to obtain the number of non-special compiler temporaries being used.
   * @return Returns the number of non-special compiler temporaries.
   */
  size_t GetNumNonSpecialCompilerTemps() const {
    return num_non_special_compiler_temps_;
  }

  /**
   * @brief Used to set the total number of available non-special compiler temporaries.
   * @details Can fail setting the new max if there are more temps being used than the new_max.
   * @param new_max The new maximum number of non-special compiler temporaries.
   * @return Returns true if the max was set and false if failed to set.
   */
  bool SetMaxAvailableNonSpecialCompilerTemps(size_t new_max) {
    if (new_max < GetNumNonSpecialCompilerTemps()) {
      return false;
    } else {
      max_available_non_special_compiler_temps_ = new_max;
      return true;
    }
  }

  /**
   * @brief Provides the number of non-special compiler temps available.
   * @details Even if this returns zero, special compiler temps are guaranteed to be available.
   * @return Returns the number of available temps.
   */
  size_t GetNumAvailableNonSpecialCompilerTemps();

  /**
   * @brief Used to obtain an existing compiler temporary.
   * @param index The index of the temporary which must be strictly less than the
   * number of temporaries.
   * @return Returns the temporary that was asked for.
   */
  CompilerTemp* GetCompilerTemp(size_t index) const {
    return compiler_temps_.Get(index);
  }

  /**
   * @brief Used to obtain the maximum number of compiler temporaries that can be requested.
   * @return Returns the maximum number of compiler temporaries, whether used or not.
   */
  size_t GetMaxPossibleCompilerTemps() const {
    return max_available_special_compiler_temps_ + max_available_non_special_compiler_temps_;
  }

  /**
   * @brief Used to obtain a new unique compiler temporary.
   * @param ct_type Type of compiler temporary requested.
   * @param wide Whether we should allocate a wide temporary.
   * @return Returns the newly created compiler temporary.
   */
  CompilerTemp* GetNewCompilerTemp(CompilerTempType ct_type, bool wide);

  bool MethodIsLeaf() {
    return attributes_ & METHOD_IS_LEAF;
  }

  RegLocation GetRegLocation(int index) {
    DCHECK((index >= 0) && (index < num_ssa_regs_));
    return reg_location_[index];
  }

  RegLocation GetMethodLoc() {
    return reg_location_[method_sreg_];
  }

  static bool IsBackedge(BasicBlock* from_bb, BasicBlock* target_bb) {
    if (from_bb == nullptr || target_bb == nullptr) {
      return false;
    }
    return (target_bb->start_offset <= from_bb->start_offset);
  }

  bool IsBackedge(BasicBlock* branch_bb, BasicBlockId target_bb_id) {
    return ((target_bb_id != NullBasicBlockId) &&
            (IsBackedge(branch_bb, GetBasicBlock(target_bb_id))));
  }

  bool IsBackwardsBranch(BasicBlock* branch_bb) {
    return IsBackedge(branch_bb, branch_bb->taken) || IsBackedge(branch_bb, branch_bb->fall_through);
  }

  void CountBranch(DexOffset from_offset, DexOffset target_offset) {
    if (target_offset <= from_offset) {
      backward_branches_++;
    } else {
      forward_branches_++;
    }
  }

  int GetBranchCount() {
    return backward_branches_ + forward_branches_;
  }

  // Is this vreg in the in set?
  bool IsInVReg(int vreg) {
    return (vreg >= cu_->num_regs);
  }

  void DumpCheckStats();
  MIR* FindMoveResult(BasicBlock* bb, MIR* mir);
  int SRegToVReg(int ssa_reg) const;
  void VerifyDataflow();
  void CheckForDominanceFrontier(BasicBlock* dom_bb, const BasicBlock* succ_bb);
  void EliminateNullChecksAndInferTypesStart();
  bool EliminateNullChecksAndInferTypes(BasicBlock* bb);
  void EliminateNullChecksAndInferTypesEnd();
  bool EliminateClassInitChecksGate();
  bool EliminateClassInitChecks(BasicBlock* bb);
  void EliminateClassInitChecksEnd();
  bool ApplyGlobalValueNumberingGate();
  bool ApplyGlobalValueNumbering(BasicBlock* bb);
  void ApplyGlobalValueNumberingEnd();
  /*
   * Type inference handling helpers.  Because Dalvik's bytecode is not fully typed,
   * we have to do some work to figure out the sreg type.  For some operations it is
   * clear based on the opcode (i.e. ADD_FLOAT v0, v1, v2), but for others (MOVE), we
   * may never know the "real" type.
   *
   * We perform the type inference operation by using an iterative  walk over
   * the graph, propagating types "defined" by typed opcodes to uses and defs in
   * non-typed opcodes (such as MOVE).  The Setxx(index) helpers are used to set defined
   * types on typed opcodes (such as ADD_INT).  The Setxx(index, is_xx) form is used to
   * propagate types through non-typed opcodes such as PHI and MOVE.  The is_xx flag
   * tells whether our guess of the type is based on a previously typed definition.
   * If so, the defined type takes precedence.  Note that it's possible to have the same sreg
   * show multiple defined types because dx treats constants as untyped bit patterns.
   * The return value of the Setxx() helpers says whether or not the Setxx() action changed
   * the current guess, and is used to know when to terminate the iterative walk.
   */
  bool SetFp(int index, bool is_fp);
  bool SetFp(int index);
  bool SetCore(int index, bool is_core);
  bool SetCore(int index);
  bool SetRef(int index, bool is_ref);
  bool SetRef(int index);
  bool SetWide(int index, bool is_wide);
  bool SetWide(int index);
  bool SetHigh(int index, bool is_high);
  bool SetHigh(int index);

  bool PuntToInterpreter() {
    return punt_to_interpreter_;
  }

  void SetPuntToInterpreter(bool val) {
    punt_to_interpreter_ = val;
  }

  char* GetDalvikDisassembly(const MIR* mir);
  void ReplaceSpecialChars(std::string& str);
  std::string GetSSAName(int ssa_reg);
  std::string GetSSANameWithConst(int ssa_reg, bool singles_only);
  void GetBlockName(BasicBlock* bb, char* name);
  const char* GetShortyFromTargetIdx(int);
  void DumpMIRGraph();
  CallInfo* NewMemCallInfo(BasicBlock* bb, MIR* mir, InvokeType type, bool is_range);
  MIR* NewMIR();
  MIR* AdvanceMIR(BasicBlock** p_bb, MIR* mir);
  BasicBlock* NextDominatedBlock(BasicBlock* bb);
  bool LayoutBlocks(BasicBlock* bb);
  void ComputeTopologicalSortOrder();
  BasicBlock* CreateNewBB(BBType block_type);
  BasicBlock* CreateNewBB(BBType block_type, int block_id);

  bool InlineSpecialMethodsGate();
  void InlineSpecialMethodsStart();
  void InlineSpecialMethods(BasicBlock* bb);
  void InlineSpecialMethodsEnd();

  /**
   * @brief Perform the initial preparation for the Method Uses.
   */
  void InitializeMethodUses();

  /**
   * @brief Perform the initial preparation for the Constant Propagation.
   */
  void InitializeConstantPropagation();

  /**
   * @brief Perform the initial preparation for the SSA Transformation.
   */
  void SSATransformationStart();

  /**
   * @brief Insert a the operands for the Phi nodes.
   * @param bb the considered BasicBlock.
   * @return true
   */
  bool InsertPhiNodeOperands(BasicBlock* bb);

  /**
   * @brief Perform the cleanup after the SSA Transformation.
   */
  void SSATransformationEnd();

  /**
   * @brief Perform constant propagation on a BasicBlock.
   * @param bb the considered BasicBlock.
   */
  void DoConstantPropagation(BasicBlock* bb);

  /**
   * @brief Count the uses in the BasicBlock
   * @param bb the BasicBlock
   */
  void CountUses(struct BasicBlock* bb);

  static uint64_t GetDataFlowAttributes(Instruction::Code opcode);
  static uint64_t GetDataFlowAttributes(MIR* mir);

  /**
   * @brief Combine BasicBlocks
   * @param the BasicBlock we are considering
   */
  void CombineBlocks(BasicBlock* bb);

  void ClearAllVisitedFlags();

  void AllocateSSAUseData(MIR *mir, int num_uses);
  void AllocateSSADefData(MIR *mir, int num_defs);
  void CalculateBasicBlockInformation();
  void InitializeBasicBlockData();
  void ComputeDFSOrders();
  void ComputeDefBlockMatrix();
  void ComputeDominators();
  void CompilerInitializeSSAConversion();
  void InsertPhiNodes();
  void DoDFSPreOrderSSARename(BasicBlock* block);

  /*
   * IsDebugBuild sanity check: keep track of the Dex PCs for catch entries so that later on
   * we can verify that all catch entries have native PC entries.
   */
  std::set<uint32_t> catches_;

  // TODO: make these private.
  RegLocation* reg_location_;                         // Map SSA names to location.
  SafeMap<unsigned int, unsigned int> block_id_map_;  // Block collapse lookup cache.

  static const char* extended_mir_op_names_[kMirOpLast - kMirOpFirst];
  static const uint32_t analysis_attributes_[kMirOpLast];

  void HandleSSADef(int* defs, int dalvik_reg, int reg_index);
  bool InferTypeAndSize(BasicBlock* bb, MIR* mir, bool changed);

  // Used for removing redudant suspend tests
  void AppendGenSuspendTestList(BasicBlock* bb) {
    if (gen_suspend_test_list_.Size() == 0 ||
        gen_suspend_test_list_.Get(gen_suspend_test_list_.Size() - 1) != bb) {
      gen_suspend_test_list_.Insert(bb);
    }
  }

  /* This is used to check if there is already a method call dominating the
   * source basic block of a backedge and being dominated by the target basic
   * block of the backedge.
   */
  bool HasSuspendTestBetween(BasicBlock* source, BasicBlockId target_id);

 protected:
  int FindCommonParent(int block1, int block2);
  void ComputeSuccLineIn(ArenaBitVector* dest, const ArenaBitVector* src1,
                         const ArenaBitVector* src2);
  void HandleLiveInUse(ArenaBitVector* use_v, ArenaBitVector* def_v,
                       ArenaBitVector* live_in_v, int dalvik_reg_id);
  void HandleDef(ArenaBitVector* def_v, int dalvik_reg_id);
  void HandleExtended(ArenaBitVector* use_v, ArenaBitVector* def_v,
                      ArenaBitVector* live_in_v,
                      const MIR::DecodedInstruction& d_insn);
  bool DoSSAConversion(BasicBlock* bb);
  int AddNewSReg(int v_reg);
  void HandleSSAUse(int* uses, int dalvik_reg, int reg_index);
  void DataFlowSSAFormat35C(MIR* mir);
  void DataFlowSSAFormat3RC(MIR* mir);
  void DataFlowSSAFormatExtended(MIR* mir);
  bool FindLocalLiveIn(BasicBlock* bb);
  bool VerifyPredInfo(BasicBlock* bb);
  BasicBlock* NeedsVisit(BasicBlock* bb);
  BasicBlock* NextUnvisitedSuccessor(BasicBlock* bb);
  void MarkPreOrder(BasicBlock* bb);
  void RecordDFSOrders(BasicBlock* bb);
  void ComputeDomPostOrderTraversal(BasicBlock* bb);
  void SetConstant(int32_t ssa_reg, int value);
  void SetConstantWide(int ssa_reg, int64_t value);
  int GetSSAUseCount(int s_reg);
  bool BasicBlockOpt(BasicBlock* bb);
  bool BuildExtendedBBList(struct BasicBlock* bb);
  bool FillDefBlockMatrix(BasicBlock* bb);
  void InitializeDominationInfo(BasicBlock* bb);
  bool ComputeblockIDom(BasicBlock* bb);
  bool ComputeBlockDominators(BasicBlock* bb);
  bool SetDominators(BasicBlock* bb);
  bool ComputeBlockLiveIns(BasicBlock* bb);
  bool ComputeDominanceFrontier(BasicBlock* bb);

  void CountChecks(BasicBlock* bb);
  void AnalyzeBlock(BasicBlock* bb, struct MethodStats* stats);
  bool ComputeSkipCompilation(struct MethodStats* stats, bool skip_default,
                              std::string* skip_message);

  CompilationUnit* const cu_;
  GrowableArray<int>* ssa_base_vregs_;
  GrowableArray<int>* ssa_subscripts_;
  // Map original Dalvik virtual reg i to the current SSA name.
  int* vreg_to_ssa_map_;            // length == method->registers_size
  int* ssa_last_defs_;              // length == method->registers_size
  ArenaBitVector* is_constant_v_;   // length == num_ssa_reg
  int* constant_values_;            // length == num_ssa_reg
  // Use counts of ssa names.
  GrowableArray<uint32_t> use_counts_;      // Weighted by nesting depth
  GrowableArray<uint32_t> raw_use_counts_;  // Not weighted
  unsigned int num_reachable_blocks_;
  unsigned int max_num_reachable_blocks_;
  GrowableArray<BasicBlockId>* dfs_order_;
  GrowableArray<BasicBlockId>* dfs_post_order_;
  GrowableArray<BasicBlockId>* dom_post_order_traversal_;
  GrowableArray<BasicBlockId>* topological_order_;
  int* i_dom_list_;
  ArenaBitVector** def_block_matrix_;    // num_dalvik_register x num_blocks.
  std::unique_ptr<ScopedArenaAllocator> temp_scoped_alloc_;
  uint16_t* temp_insn_data_;
  uint32_t temp_bit_vector_size_;
  ArenaBitVector* temp_bit_vector_;
  std::unique_ptr<GlobalValueNumbering> temp_gvn_;
  static const int kInvalidEntry = -1;
  GrowableArray<BasicBlock*> block_list_;
  BasicBlock* entry_block_;
  BasicBlock* exit_block_;
  unsigned int num_blocks_;

  /**
   * @brief List of dex compilation units for each method included in graph.
   */
  std::vector<DexCompilationUnit*> m_units_;

  /**
   * @brief Contains BitVectors that represent the address ranges that are inside a try block.
   * @details Keeps track of this information for all of the methods in MIRGraph. This structure
   * is indexed by the corresponding position in the m_units_. This keeps original MIR offsets
   * instead of renamed ones so GetMirOriginalOffset should be used to get the real offset.
   */
  std::vector<ArenaBitVector *> m_unit_to_try_block_addr;

  /**
   * @brief Keeps track of the start offsets for each method in MIRGraph.
   * @details All inlined callees contained renamed offsets corresponding to the start.
   * Getting the real offset means getting a MIR's m_unit and subtracting the start offset
   * from the actual MIR offset.
   */
  std::vector<DexOffset> m_unit_to_start_offset;

  int def_count_;                                // Used to estimate size of ssa name storage.
  int* opcode_count_;                            // Dex opcode coverage stats.
  int num_ssa_regs_;                             // Number of names following SSA transformation.
  std::vector<BasicBlockId> extended_basic_blocks_;  // Heads of block "traces".
  int method_sreg_;
  unsigned int attributes_;
  Checkstats* checkstats_;
  ArenaAllocator* arena_;
  int backward_branches_;
  int forward_branches_;
  GrowableArray<CompilerTemp*> compiler_temps_;
  size_t num_non_special_compiler_temps_;
  size_t max_available_non_special_compiler_temps_;
  size_t max_available_special_compiler_temps_;
  bool punt_to_interpreter_;                    // Difficult or not worthwhile - just interpret.
  uint64_t merged_df_flags_;
  GrowableArray<MirIFieldLoweringInfo> ifield_lowering_infos_;
  GrowableArray<MirSFieldLoweringInfo> sfield_lowering_infos_;
  GrowableArray<MirMethodLoweringInfo> method_lowering_infos_;
  static const uint64_t oat_data_flow_attributes_[kMirOpLast];
  GrowableArray<BasicBlock*> gen_suspend_test_list_;  // List of blocks containing suspend tests

  friend class ClassInitCheckEliminationTest;
  friend class GlobalValueNumberingTest;
  friend class LocalValueNumberingTest;
};

}  // namespace art

#endif  // ART_COMPILER_DEX_MIR_GRAPH_H_
