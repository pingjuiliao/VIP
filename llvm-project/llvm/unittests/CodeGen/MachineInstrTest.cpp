//===- MachineInstrTest.cpp -----------------------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "llvm/CodeGen/MachineBasicBlock.h"
#include "llvm/CodeGen/MachineInstr.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineMemOperand.h"
#include "llvm/CodeGen/MachineModuleInfo.h"
#include "llvm/CodeGen/TargetFrameLowering.h"
#include "llvm/CodeGen/TargetInstrInfo.h"
#include "llvm/CodeGen/TargetLowering.h"
#include "llvm/CodeGen/TargetSubtargetInfo.h"
#include "llvm/IR/DebugInfoMetadata.h"
#include "llvm/IR/ModuleSlotTracker.h"
#include "llvm/MC/MCAsmInfo.h"
#include "llvm/MC/MCSymbol.h"
#include "llvm/Support/TargetRegistry.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Target/TargetOptions.h"
#include "gtest/gtest.h"

using namespace llvm;

namespace {
// Add a few Bogus backend classes so we can create MachineInstrs without
// depending on a real target.
class BogusTargetLowering : public TargetLowering {
public:
  BogusTargetLowering(TargetMachine &TM) : TargetLowering(TM) {}
};

class BogusFrameLowering : public TargetFrameLowering {
public:
  BogusFrameLowering()
      : TargetFrameLowering(TargetFrameLowering::StackGrowsDown, Align(4), 4) {}

  void emitPrologue(MachineFunction &MF,
                    MachineBasicBlock &MBB) const override {}
  void emitEpilogue(MachineFunction &MF,
                    MachineBasicBlock &MBB) const override {}
  bool hasFP(const MachineFunction &MF) const override { return false; }
};

static TargetRegisterClass *const BogusRegisterClasses[] = {nullptr};

class BogusRegisterInfo : public TargetRegisterInfo {
public:
  BogusRegisterInfo()
      : TargetRegisterInfo(nullptr, BogusRegisterClasses, BogusRegisterClasses,
                           nullptr, nullptr, LaneBitmask(~0u), nullptr) {
    InitMCRegisterInfo(nullptr, 0, 0, 0, nullptr, 0, nullptr, 0, nullptr,
                       nullptr, nullptr, nullptr, nullptr, 0, nullptr, nullptr);
  }

  const MCPhysReg *
  getCalleeSavedRegs(const MachineFunction *MF) const override {
    return nullptr;
  }
  ArrayRef<const uint32_t *> getRegMasks() const override { return None; }
  ArrayRef<const char *> getRegMaskNames() const override { return None; }
  BitVector getReservedRegs(const MachineFunction &MF) const override {
    return BitVector();
  }
  const RegClassWeight &
  getRegClassWeight(const TargetRegisterClass *RC) const override {
    static RegClassWeight Bogus{1, 16};
    return Bogus;
  }
  unsigned getRegUnitWeight(unsigned RegUnit) const override { return 1; }
  unsigned getNumRegPressureSets() const override { return 0; }
  const char *getRegPressureSetName(unsigned Idx) const override {
    return "bogus";
  }
  unsigned getRegPressureSetLimit(const MachineFunction &MF,
                                  unsigned Idx) const override {
    return 0;
  }
  const int *
  getRegClassPressureSets(const TargetRegisterClass *RC) const override {
    static const int Bogus[] = {0, -1};
    return &Bogus[0];
  }
  const int *getRegUnitPressureSets(unsigned RegUnit) const override {
    static const int Bogus[] = {0, -1};
    return &Bogus[0];
  }

  Register getFrameRegister(const MachineFunction &MF) const override {
    return 0;
  }
  void eliminateFrameIndex(MachineBasicBlock::iterator MI, int SPAdj,
                           unsigned FIOperandNum,
                           RegScavenger *RS = nullptr) const override {}
};

class BogusSubtarget : public TargetSubtargetInfo {
public:
  BogusSubtarget(TargetMachine &TM)
      : TargetSubtargetInfo(Triple(""), "", "", {}, {}, nullptr, nullptr,
                            nullptr, nullptr, nullptr, nullptr),
        FL(), TL(TM) {}
  ~BogusSubtarget() override {}

  const TargetFrameLowering *getFrameLowering() const override { return &FL; }

  const TargetLowering *getTargetLowering() const override { return &TL; }

  const TargetInstrInfo *getInstrInfo() const override { return &TII; }

  const TargetRegisterInfo *getRegisterInfo() const override { return &TRI; }

private:
  BogusFrameLowering FL;
  BogusRegisterInfo TRI;
  BogusTargetLowering TL;
  TargetInstrInfo TII;
};

class BogusTargetMachine : public LLVMTargetMachine {
public:
  BogusTargetMachine()
      : LLVMTargetMachine(Target(), "", Triple(""), "", "", TargetOptions(),
                          Reloc::Static, CodeModel::Small, CodeGenOpt::Default),
        ST(*this) {}

  ~BogusTargetMachine() override {}

  const TargetSubtargetInfo *getSubtargetImpl(const Function &) const override {
    return &ST;
  }

private:
  BogusSubtarget ST;
};

std::unique_ptr<MCContext> createMCContext(MCAsmInfo *AsmInfo) {
  return std::make_unique<MCContext>(
      AsmInfo, nullptr, nullptr, nullptr, nullptr, false);
}

std::unique_ptr<BogusTargetMachine> createTargetMachine() {
  return std::make_unique<BogusTargetMachine>();
}

std::unique_ptr<MachineFunction> createMachineFunction() {
  LLVMContext Ctx;
  Module M("Module", Ctx);
  auto Type = FunctionType::get(Type::getVoidTy(Ctx), false);
  auto F = Function::Create(Type, GlobalValue::ExternalLinkage, "Test", &M);

  auto TM = createTargetMachine();
  unsigned FunctionNum = 42;
  MachineModuleInfo MMI(TM.get());
  const TargetSubtargetInfo &STI = *TM->getSubtargetImpl(*F);

  return std::make_unique<MachineFunction>(*F, *TM, STI, FunctionNum, MMI);
}

// This test makes sure that MachineInstr::isIdenticalTo handles Defs correctly
// for various combinations of IgnoreDefs, and also that it is symmetrical.
TEST(IsIdenticalToTest, DifferentDefs) {
  auto MF = createMachineFunction();

  unsigned short NumOps = 2;
  unsigned char NumDefs = 1;
  MCOperandInfo OpInfo[] = {
      {0, 0, MCOI::OPERAND_REGISTER, 0},
      {0, 1 << MCOI::OptionalDef, MCOI::OPERAND_REGISTER, 0}};
  MCInstrDesc MCID = {
      0, NumOps,  NumDefs, 0,      0, 1ULL << MCID::HasOptionalDef,
      0, nullptr, nullptr, OpInfo, 0, nullptr};

  // Create two MIs with different virtual reg defs and the same uses.
  unsigned VirtualDef1 = -42; // The value doesn't matter, but the sign does.
  unsigned VirtualDef2 = -43;
  unsigned VirtualUse = -44;

  auto MI1 = MF->CreateMachineInstr(MCID, DebugLoc());
  MI1->addOperand(*MF, MachineOperand::CreateReg(VirtualDef1, /*isDef*/ true));
  MI1->addOperand(*MF, MachineOperand::CreateReg(VirtualUse, /*isDef*/ false));

  auto MI2 = MF->CreateMachineInstr(MCID, DebugLoc());
  MI2->addOperand(*MF, MachineOperand::CreateReg(VirtualDef2, /*isDef*/ true));
  MI2->addOperand(*MF, MachineOperand::CreateReg(VirtualUse, /*isDef*/ false));

  // Check that they are identical when we ignore virtual register defs, but not
  // when we check defs.
  ASSERT_FALSE(MI1->isIdenticalTo(*MI2, MachineInstr::CheckDefs));
  ASSERT_FALSE(MI2->isIdenticalTo(*MI1, MachineInstr::CheckDefs));

  ASSERT_TRUE(MI1->isIdenticalTo(*MI2, MachineInstr::IgnoreVRegDefs));
  ASSERT_TRUE(MI2->isIdenticalTo(*MI1, MachineInstr::IgnoreVRegDefs));

  // Create two MIs with different virtual reg defs, and a def or use of a
  // sentinel register.
  unsigned SentinelReg = 0;

  auto MI3 = MF->CreateMachineInstr(MCID, DebugLoc());
  MI3->addOperand(*MF, MachineOperand::CreateReg(VirtualDef1, /*isDef*/ true));
  MI3->addOperand(*MF, MachineOperand::CreateReg(SentinelReg, /*isDef*/ true));

  auto MI4 = MF->CreateMachineInstr(MCID, DebugLoc());
  MI4->addOperand(*MF, MachineOperand::CreateReg(VirtualDef2, /*isDef*/ true));
  MI4->addOperand(*MF, MachineOperand::CreateReg(SentinelReg, /*isDef*/ false));

  // Check that they are never identical.
  ASSERT_FALSE(MI3->isIdenticalTo(*MI4, MachineInstr::CheckDefs));
  ASSERT_FALSE(MI4->isIdenticalTo(*MI3, MachineInstr::CheckDefs));

  ASSERT_FALSE(MI3->isIdenticalTo(*MI4, MachineInstr::IgnoreVRegDefs));
  ASSERT_FALSE(MI4->isIdenticalTo(*MI3, MachineInstr::IgnoreVRegDefs));
}

// Check that MachineInstrExpressionTrait::isEqual is symmetric and in sync with
// MachineInstrExpressionTrait::getHashValue
void checkHashAndIsEqualMatch(MachineInstr *MI1, MachineInstr *MI2) {
  bool IsEqual1 = MachineInstrExpressionTrait::isEqual(MI1, MI2);
  bool IsEqual2 = MachineInstrExpressionTrait::isEqual(MI2, MI1);

  ASSERT_EQ(IsEqual1, IsEqual2);

  auto Hash1 = MachineInstrExpressionTrait::getHashValue(MI1);
  auto Hash2 = MachineInstrExpressionTrait::getHashValue(MI2);

  ASSERT_EQ(IsEqual1, Hash1 == Hash2);
}

// This test makes sure that MachineInstrExpressionTraits::isEqual is in sync
// with MachineInstrExpressionTraits::getHashValue.
TEST(MachineInstrExpressionTraitTest, IsEqualAgreesWithGetHashValue) {
  auto MF = createMachineFunction();

  unsigned short NumOps = 2;
  unsigned char NumDefs = 1;
  MCOperandInfo OpInfo[] = {
      {0, 0, MCOI::OPERAND_REGISTER, 0},
      {0, 1 << MCOI::OptionalDef, MCOI::OPERAND_REGISTER, 0}};
  MCInstrDesc MCID = {
      0, NumOps,  NumDefs, 0,      0, 1ULL << MCID::HasOptionalDef,
      0, nullptr, nullptr, OpInfo, 0, nullptr};

  // Define a series of instructions with different kinds of operands and make
  // sure that the hash function is consistent with isEqual for various
  // combinations of them.
  unsigned VirtualDef1 = -42;
  unsigned VirtualDef2 = -43;
  unsigned VirtualReg = -44;
  unsigned SentinelReg = 0;
  unsigned PhysicalReg = 45;

  auto VD1VU = MF->CreateMachineInstr(MCID, DebugLoc());
  VD1VU->addOperand(*MF,
                    MachineOperand::CreateReg(VirtualDef1, /*isDef*/ true));
  VD1VU->addOperand(*MF,
                    MachineOperand::CreateReg(VirtualReg, /*isDef*/ false));

  auto VD2VU = MF->CreateMachineInstr(MCID, DebugLoc());
  VD2VU->addOperand(*MF,
                    MachineOperand::CreateReg(VirtualDef2, /*isDef*/ true));
  VD2VU->addOperand(*MF,
                    MachineOperand::CreateReg(VirtualReg, /*isDef*/ false));

  auto VD1SU = MF->CreateMachineInstr(MCID, DebugLoc());
  VD1SU->addOperand(*MF,
                    MachineOperand::CreateReg(VirtualDef1, /*isDef*/ true));
  VD1SU->addOperand(*MF,
                    MachineOperand::CreateReg(SentinelReg, /*isDef*/ false));

  auto VD1SD = MF->CreateMachineInstr(MCID, DebugLoc());
  VD1SD->addOperand(*MF,
                    MachineOperand::CreateReg(VirtualDef1, /*isDef*/ true));
  VD1SD->addOperand(*MF,
                    MachineOperand::CreateReg(SentinelReg, /*isDef*/ true));

  auto VD2PU = MF->CreateMachineInstr(MCID, DebugLoc());
  VD2PU->addOperand(*MF,
                    MachineOperand::CreateReg(VirtualDef2, /*isDef*/ true));
  VD2PU->addOperand(*MF,
                    MachineOperand::CreateReg(PhysicalReg, /*isDef*/ false));

  auto VD2PD = MF->CreateMachineInstr(MCID, DebugLoc());
  VD2PD->addOperand(*MF,
                    MachineOperand::CreateReg(VirtualDef2, /*isDef*/ true));
  VD2PD->addOperand(*MF,
                    MachineOperand::CreateReg(PhysicalReg, /*isDef*/ true));

  checkHashAndIsEqualMatch(VD1VU, VD2VU);
  checkHashAndIsEqualMatch(VD1VU, VD1SU);
  checkHashAndIsEqualMatch(VD1VU, VD1SD);
  checkHashAndIsEqualMatch(VD1VU, VD2PU);
  checkHashAndIsEqualMatch(VD1VU, VD2PD);

  checkHashAndIsEqualMatch(VD2VU, VD1SU);
  checkHashAndIsEqualMatch(VD2VU, VD1SD);
  checkHashAndIsEqualMatch(VD2VU, VD2PU);
  checkHashAndIsEqualMatch(VD2VU, VD2PD);

  checkHashAndIsEqualMatch(VD1SU, VD1SD);
  checkHashAndIsEqualMatch(VD1SU, VD2PU);
  checkHashAndIsEqualMatch(VD1SU, VD2PD);

  checkHashAndIsEqualMatch(VD1SD, VD2PU);
  checkHashAndIsEqualMatch(VD1SD, VD2PD);

  checkHashAndIsEqualMatch(VD2PU, VD2PD);
}

TEST(MachineInstrPrintingTest, DebugLocPrinting) {
  auto MF = createMachineFunction();

  MCOperandInfo OpInfo{0, 0, MCOI::OPERAND_REGISTER, 0};
  MCInstrDesc MCID = {0, 1,       1,       0,       0, 0,
                      0, nullptr, nullptr, &OpInfo, 0, nullptr};

  LLVMContext Ctx;
  DIFile *DIF = DIFile::getDistinct(Ctx, "filename", "");
  DISubprogram *DIS = DISubprogram::getDistinct(
      Ctx, nullptr, "", "", DIF, 0, nullptr, 0, nullptr, 0, 0, DINode::FlagZero,
      DISubprogram::SPFlagZero, nullptr);
  DILocation *DIL = DILocation::get(Ctx, 1, 5, DIS);
  DebugLoc DL(DIL);
  MachineInstr *MI = MF->CreateMachineInstr(MCID, DL);
  MI->addOperand(*MF, MachineOperand::CreateReg(0, /*isDef*/ true));

  std::string str;
  raw_string_ostream OS(str);
  MI->print(OS, /*IsStandalone*/true, /*SkipOpers*/false, /*SkipDebugLoc*/false,
            /*AddNewLine*/false);
  ASSERT_TRUE(
      StringRef(OS.str()).startswith("$noreg = UNKNOWN debug-location "));
  ASSERT_TRUE(
      StringRef(OS.str()).endswith("filename:1:5"));
}

TEST(MachineInstrSpan, DistanceBegin) {
  auto MF = createMachineFunction();
  auto MBB = MF->CreateMachineBasicBlock();

  MCInstrDesc MCID = {0, 0,       0,       0,       0, 0,
                      0, nullptr, nullptr, nullptr, 0, nullptr};

  auto MII = MBB->begin();
  MachineInstrSpan MIS(MII, MBB);
  ASSERT_TRUE(MIS.empty());

  auto MI = MF->CreateMachineInstr(MCID, DebugLoc());
  MBB->insert(MII, MI);
  ASSERT_TRUE(std::distance(MIS.begin(), MII) == 1);
}

TEST(MachineInstrSpan, DistanceEnd) {
  auto MF = createMachineFunction();
  auto MBB = MF->CreateMachineBasicBlock();

  MCInstrDesc MCID = {0, 0,       0,       0,       0, 0,
                      0, nullptr, nullptr, nullptr, 0, nullptr};

  auto MII = MBB->end();
  MachineInstrSpan MIS(MII, MBB);
  ASSERT_TRUE(MIS.empty());

  auto MI = MF->CreateMachineInstr(MCID, DebugLoc());
  MBB->insert(MII, MI);
  ASSERT_TRUE(std::distance(MIS.begin(), MII) == 1);
}

TEST(MachineInstrExtraInfo, AddExtraInfo) {
  auto MF = createMachineFunction();
  MCInstrDesc MCID = {0, 0,       0,       0,       0, 0,
                      0, nullptr, nullptr, nullptr, 0, nullptr};

  auto MI = MF->CreateMachineInstr(MCID, DebugLoc());
  auto MAI = MCAsmInfo();
  auto MC = createMCContext(&MAI);
  auto MMO = MF->getMachineMemOperand(MachinePointerInfo(),
                                      MachineMemOperand::MOLoad, 8, 8);
  SmallVector<MachineMemOperand *, 2> MMOs;
  MMOs.push_back(MMO);
  MCSymbol *Sym1 = MC->createTempSymbol("pre_label", false);
  MCSymbol *Sym2 = MC->createTempSymbol("post_label", false);
  LLVMContext Ctx;
  MDNode *MDN = MDNode::getDistinct(Ctx, None);

  ASSERT_TRUE(MI->memoperands_empty());
  ASSERT_FALSE(MI->getPreInstrSymbol());
  ASSERT_FALSE(MI->getPostInstrSymbol());
  ASSERT_FALSE(MI->getHeapAllocMarker());

  MI->setMemRefs(*MF, MMOs);
  ASSERT_TRUE(MI->memoperands().size() == 1);
  ASSERT_FALSE(MI->getPreInstrSymbol());
  ASSERT_FALSE(MI->getPostInstrSymbol());
  ASSERT_FALSE(MI->getHeapAllocMarker());

  MI->setPreInstrSymbol(*MF, Sym1);
  ASSERT_TRUE(MI->memoperands().size() == 1);
  ASSERT_TRUE(MI->getPreInstrSymbol() == Sym1);
  ASSERT_FALSE(MI->getPostInstrSymbol());
  ASSERT_FALSE(MI->getHeapAllocMarker());

  MI->setPostInstrSymbol(*MF, Sym2);
  ASSERT_TRUE(MI->memoperands().size() == 1);
  ASSERT_TRUE(MI->getPreInstrSymbol() == Sym1);
  ASSERT_TRUE(MI->getPostInstrSymbol() == Sym2);
  ASSERT_FALSE(MI->getHeapAllocMarker());

  MI->setHeapAllocMarker(*MF, MDN);
  ASSERT_TRUE(MI->memoperands().size() == 1);
  ASSERT_TRUE(MI->getPreInstrSymbol() == Sym1);
  ASSERT_TRUE(MI->getPostInstrSymbol() == Sym2);
  ASSERT_TRUE(MI->getHeapAllocMarker() == MDN);
}

TEST(MachineInstrExtraInfo, ChangeExtraInfo) {
  auto MF = createMachineFunction();
  MCInstrDesc MCID = {0, 0,       0,       0,       0, 0,
                      0, nullptr, nullptr, nullptr, 0, nullptr};

  auto MI = MF->CreateMachineInstr(MCID, DebugLoc());
  auto MAI = MCAsmInfo();
  auto MC = createMCContext(&MAI);
  auto MMO = MF->getMachineMemOperand(MachinePointerInfo(),
                                      MachineMemOperand::MOLoad, 8, 8);
  SmallVector<MachineMemOperand *, 2> MMOs;
  MMOs.push_back(MMO);
  MCSymbol *Sym1 = MC->createTempSymbol("pre_label", false);
  MCSymbol *Sym2 = MC->createTempSymbol("post_label", false);
  LLVMContext Ctx;
  MDNode *MDN = MDNode::getDistinct(Ctx, None);

  MI->setMemRefs(*MF, MMOs);
  MI->setPreInstrSymbol(*MF, Sym1);
  MI->setPostInstrSymbol(*MF, Sym2);
  MI->setHeapAllocMarker(*MF, MDN);

  MMOs.push_back(MMO);

  MI->setMemRefs(*MF, MMOs);
  ASSERT_TRUE(MI->memoperands().size() == 2);
  ASSERT_TRUE(MI->getPreInstrSymbol() == Sym1);
  ASSERT_TRUE(MI->getPostInstrSymbol() == Sym2);
  ASSERT_TRUE(MI->getHeapAllocMarker() == MDN);

  MI->setPostInstrSymbol(*MF, Sym1);
  ASSERT_TRUE(MI->memoperands().size() == 2);
  ASSERT_TRUE(MI->getPreInstrSymbol() == Sym1);
  ASSERT_TRUE(MI->getPostInstrSymbol() == Sym1);
  ASSERT_TRUE(MI->getHeapAllocMarker() == MDN);
}

TEST(MachineInstrExtraInfo, RemoveExtraInfo) {
  auto MF = createMachineFunction();
  MCInstrDesc MCID = {0, 0,       0,       0,       0, 0,
                      0, nullptr, nullptr, nullptr, 0, nullptr};

  auto MI = MF->CreateMachineInstr(MCID, DebugLoc());
  auto MAI = MCAsmInfo();
  auto MC = createMCContext(&MAI);
  auto MMO = MF->getMachineMemOperand(MachinePointerInfo(),
                                      MachineMemOperand::MOLoad, 8, 8);
  SmallVector<MachineMemOperand *, 2> MMOs;
  MMOs.push_back(MMO);
  MMOs.push_back(MMO);
  MCSymbol *Sym1 = MC->createTempSymbol("pre_label", false);
  MCSymbol *Sym2 = MC->createTempSymbol("post_label", false);
  LLVMContext Ctx;
  MDNode *MDN = MDNode::getDistinct(Ctx, None);

  MI->setMemRefs(*MF, MMOs);
  MI->setPreInstrSymbol(*MF, Sym1);
  MI->setPostInstrSymbol(*MF, Sym2);
  MI->setHeapAllocMarker(*MF, MDN);

  MI->setPostInstrSymbol(*MF, nullptr);
  ASSERT_TRUE(MI->memoperands().size() == 2);
  ASSERT_TRUE(MI->getPreInstrSymbol() == Sym1);
  ASSERT_FALSE(MI->getPostInstrSymbol());
  ASSERT_TRUE(MI->getHeapAllocMarker() == MDN);

  MI->setHeapAllocMarker(*MF, nullptr);
  ASSERT_TRUE(MI->memoperands().size() == 2);
  ASSERT_TRUE(MI->getPreInstrSymbol() == Sym1);
  ASSERT_FALSE(MI->getPostInstrSymbol());
  ASSERT_FALSE(MI->getHeapAllocMarker());

  MI->setPreInstrSymbol(*MF, nullptr);
  ASSERT_TRUE(MI->memoperands().size() == 2);
  ASSERT_FALSE(MI->getPreInstrSymbol());
  ASSERT_FALSE(MI->getPostInstrSymbol());
  ASSERT_FALSE(MI->getHeapAllocMarker());

  MI->setMemRefs(*MF, {});
  ASSERT_TRUE(MI->memoperands_empty());
  ASSERT_FALSE(MI->getPreInstrSymbol());
  ASSERT_FALSE(MI->getPostInstrSymbol());
  ASSERT_FALSE(MI->getHeapAllocMarker());
}

static_assert(is_trivially_copyable<MCOperand>::value, "trivially copyable");

} // end namespace
