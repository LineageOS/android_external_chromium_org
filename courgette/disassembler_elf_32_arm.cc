// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "courgette/disassembler_elf_32_arm.h"

#include <algorithm>
#include <string>
#include <vector>

#include "base/basictypes.h"
#include "base/logging.h"

#include "courgette/assembly_program.h"
#include "courgette/courgette.h"
#include "courgette/encoded_program.h"

namespace courgette {

CheckBool DisassemblerElf32ARM::Compress(ARM_RVA type, uint32 arm_op, RVA rva,
                                         uint16* c_op, uint32* addr) {
  // This method takes an ARM or thumb opcode, extracts the relative
  // target address from it (addr), and creates a corresponding
  // Courgette opcode (c_op).
  //
  // Details on ARM the opcodes, and how the relative targets are
  // computed were taken from the "ARM Architecture Reference Manual",
  // section A4.1.5 and the "Thumb-2 supplement", section 4.6.12.
  // ARM_OFF24 is for the ARM opcode.  The rest are for thumb opcodes.
  switch (type) {
    case ARM_OFF8: {
      // The offset is given by lower 8 bits of the op.  It is a 9-bit
      // offset, shifted right one bit and signed extended.
      uint32 temp = (arm_op & 0x00FF) << 1;
      if (temp & 0x0100)
        temp |= 0xFFFFFE00;
      temp += 4;  // Offset from _next_ PC.
      fflush(stdout);

      (*addr) = temp;
      (*c_op) = (arm_op >> 8) | 0x1000;
      break;
    }
    case ARM_OFF11: {
      // The offset is given by lower 11 bits of the op, and is a
      // 12-bit offset, shifted right one bit and sign extended.
      uint32 temp = (arm_op & 0x07FF) << 1;
      if (temp & 0x00000800)
        temp |= 0xFFFFF000;
      temp += 4;  // Offset from _next_ PC.

      (*addr) = temp;
      (*c_op) = (arm_op >> 11) | 0x2000;
      break;
    }
    case ARM_OFF24: {
      // The offset is given by the lower 24-bits of the op, shifted
      // left 2 bits, and sign extended.
      uint32 temp = (arm_op & 0x00FFFFFF) << 2;
      if (temp & 0x02000000)
        temp |= 0xFC000000;
      temp += 8;

      (*addr) = temp;
      (*c_op) = (arm_op >> 24) | 0x3000;
      break;
    }
    case ARM_OFF25: {
      uint32 temp = 0;
      temp |= (arm_op & 0x000007FF) << 1;  // imm11
      temp |= (arm_op & 0x03FF0000) >> 4;  // imm10

      uint32 S   = (arm_op & (1 << 26)) >> 26;
      uint32 j2  = (arm_op & (1 << 11)) >> 11;
      uint32 j1  = (arm_op & (1 << 13)) >> 13;
      bool bit12 = ((arm_op & (1 << 12)) >> 12) != 0;
      bool bit14 = ((arm_op & (1 << 14)) >> 14) != 0;

      uint32 i2  = ~(j2 ^ S) & 1;
      uint32 i1  = ~(j1 ^ S) & 1;
      bool toARM =  bit14 && !bit12;

      temp |= (S << 24) | (i1 << 23) | (i2 << 22);

      if (temp & 0x01000000) // sign extension
        temp |= 0xFE000000;
      uint32 prefetch;
      if (toARM) {
        // Align PC on 4-byte boundary
        uint32 align4byte = (rva % 4) ? 2 : 4;
        prefetch = align4byte;
      } else {
        prefetch = 4;
      }
      temp += prefetch;
      (*addr) = temp;

      uint32 temp2 = 0x4000;
      temp2 |= (arm_op & (1 << 12)) >> 12;
      temp2 |= (arm_op & (1 << 14)) >> 13;
      temp2 |= (arm_op & (1 << 15)) >> 13;
      temp2 |= (arm_op & 0xF8000000) >> 24;
      temp2 |= (prefetch & 0x0000000F) << 8;
      (*c_op) = temp2;
      break;
    }
    case ARM_OFF21: {
      uint32 temp = 0;
      temp |= (arm_op & 0x000007FF) << 1;  // imm11
      temp |= (arm_op & 0x003F0000) >> 4;  // imm6

      uint32 S   = (arm_op & (1 << 26)) >> 26;
      uint32 j2  = (arm_op & (1 << 11)) >> 11;
      uint32 j1  = (arm_op & (1 << 13)) >> 13;

      temp |= (S << 20) | (j1 << 19) | (j2 << 18);

      if (temp & 0x00100000)  // sign extension
        temp |= 0xFFE00000;
      temp += 4;
      (*addr) = temp;

      uint32 temp2 = 0x5000;
      temp2 |= (arm_op & 0x03C00000) >> 22;  // just save the cond
      (*c_op) = temp2;
      break;
    }
    default:
      return false;
  }
  return true;
}

CheckBool DisassemblerElf32ARM::Decompress(ARM_RVA type, uint16 c_op,
                                           uint32 addr, uint32* arm_op) {
  // Reverses the process in the compress() method.  Takes the
  // Courgette op and relative address and reconstructs the original
  // ARM or thumb op.
  switch (type) {
    case ARM_OFF8:
      (*arm_op) = ((c_op & 0x0FFF) << 8) | (((addr - 4) >> 1) & 0x000000FF);
      break;
    case ARM_OFF11:
      (*arm_op) = ((c_op & 0x0FFF) << 11) | (((addr - 4) >> 1) & 0x000007FF);
      break;
    case ARM_OFF24:
      (*arm_op) = ((c_op & 0x0FFF) << 24) | (((addr - 8) >> 2) & 0x00FFFFFF);
      break;
    case ARM_OFF25: {
      uint32 temp = 0;
      temp |= (c_op & (1 << 0)) << 12;
      temp |= (c_op & (1 << 1)) << 13;
      temp |= (c_op & (1 << 2)) << 13;
      temp |= (c_op & (0xF8000000 >> 24)) << 24;

      uint32 prefetch = (c_op & 0x0F00) >> 8;
      addr -= prefetch;

      addr &= 0x01FFFFFF;

      uint32 S  = (addr & (1 << 24)) >> 24;
      uint32 i1 = (addr & (1 << 23)) >> 23;
      uint32 i2 = (addr & (1 << 22)) >> 22;

      uint32 j1 = ((~i1) ^ S) & 1;
      uint32 j2 = ((~i2) ^ S) & 1;

      temp |= S << 26;
      temp |= j2 << 11;
      temp |= j1 << 13;

      temp |= (addr & (0x000007FF << 1)) >> 1;
      temp |= (addr & (0x03FF0000 >> 4)) << 4;

      (*arm_op) = temp;
      break;
    }
    case ARM_OFF21: {
      uint32 temp = 0xF0008000;
      temp |= (c_op & (0x03C00000 >> 22)) << 22;

      addr -= 4;
      addr &= 0x001FFFFF;

      uint32 S  = (addr & (1 << 20)) >> 20;
      uint32 j1 = (addr & (1 << 19)) >> 19;
      uint32 j2 = (addr & (1 << 18)) >> 18;

      temp |= S << 26;
      temp |= j2 << 11;
      temp |= j1 << 13;

      temp |= (addr & (0x000007FF << 1)) >> 1;
      temp |= (addr & (0x003F0000 >> 4)) << 4;

      (*arm_op) = temp;
      break;
    }
    default:
      return false;
  }
  return true;
}

uint16 DisassemblerElf32ARM::TypedRVAARM::op_size() const {
  switch (type_) {
    case ARM_OFF8:
      return 2;
    case ARM_OFF11:
      return 2;
    case ARM_OFF24:
      return 4;
    case ARM_OFF25:
      return 4;
    case ARM_OFF21:
      return 4;
    default:
      return -1;
  }
}

CheckBool DisassemblerElf32ARM::TypedRVAARM::ComputeRelativeTarget(
    const uint8* op_pointer) {
  arm_op_ = op_pointer;
  switch (type_) {
    case ARM_OFF8:
      // Fall through
    case ARM_OFF11: {
      RVA relative_target;
      CheckBool ret = Compress(type_, Read16LittleEndian(op_pointer), rva(),
                               &c_op_, &relative_target);
      set_relative_target(relative_target);
      return ret;
    }
    case ARM_OFF24: {
      RVA relative_target;
      CheckBool ret = Compress(type_, Read32LittleEndian(op_pointer), rva(),
                               &c_op_, &relative_target);
      set_relative_target(relative_target);
      return ret;
    }
    case ARM_OFF25:
      // Fall through
    case ARM_OFF21: {
      // A thumb-2 op is 32 bits stored as two 16-bit words
      uint32 pval = (Read16LittleEndian(op_pointer) << 16)
        | Read16LittleEndian(op_pointer + 2);
      RVA relative_target;
      CheckBool ret = Compress(type_, pval, rva(), &c_op_, &relative_target);
      set_relative_target(relative_target);
      return ret;
    }
   default:
     return false;
  }
}

CheckBool DisassemblerElf32ARM::TypedRVAARM::EmitInstruction(
    AssemblyProgram* program,
    RVA target_rva) {
  return program->EmitRel32ARM(c_op(),
                               program->FindOrMakeRel32Label(target_rva),
                               arm_op_,
                               op_size());
}

DisassemblerElf32ARM::DisassemblerElf32ARM(const void* start, size_t length)
  : DisassemblerElf32(start, length) {
}

// Convert an ELF relocation struction into an RVA
CheckBool DisassemblerElf32ARM::RelToRVA(Elf32_Rel rel, RVA* result) const {

  // The rightmost byte of r_info is the type...
  elf32_rel_arm_type_values type =
      (elf32_rel_arm_type_values)(unsigned char)rel.r_info;

  // The other 3 bytes of r_info are the symbol
  uint32 symbol =  rel.r_info >> 8;

  switch(type)
  {
    case R_ARM_RELATIVE:
      if (symbol != 0)
        return false;

      // This is a basic ABS32 relocation address
      *result = rel.r_offset;
      return true;

    default:
      return false;
  }
}

CheckBool DisassemblerElf32ARM::ParseRelocationSection(
    const Elf32_Shdr *section_header,
      AssemblyProgram* program) {
  // This method compresses a contiguous stretch of R_ARM_RELATIVE
  // entries in the relocation table with a Courgette relocation table
  // instruction.  It skips any entries at the beginning that appear
  // in a section that Courgette doesn't support, e.g. INIT.
  // Specifically, the entries should be
  //   (1) In the same relocation table
  //   (2) Are consecutive
  //   (3) Are sorted in memory address order
  //
  // Happily, this is normally the case, but it's not required by spec
  // so we check, and just don't do it if we don't match up.
  //
  // The expectation is that one relocation section will contain
  // all of our R_ARM_RELATIVE entries in the expected order followed
  // by assorted other entries we can't use special handling for.

  bool match = true;

  // Walk all the bytes in the section, matching relocation table or not
  size_t file_offset = section_header->sh_offset;
  size_t section_end = section_header->sh_offset + section_header->sh_size;

  Elf32_Rel *section_relocs_iter =
      (Elf32_Rel *)OffsetToPointer(section_header->sh_offset);

  uint32 section_relocs_count = section_header->sh_size /
                                section_header->sh_entsize;

  if (abs32_locations_.size() > section_relocs_count)
    match = false;

  if (!abs32_locations_.empty()) {
    std::vector<RVA>::iterator reloc_iter = abs32_locations_.begin();

    for (uint32 i = 0; i < section_relocs_count; i++) {
      if (section_relocs_iter->r_offset == *reloc_iter)
        break;

      if (!ParseSimpleRegion(file_offset, file_offset + sizeof(Elf32_Rel),
                             program))
        return false;

      file_offset += sizeof(Elf32_Rel);
      ++section_relocs_iter;
    }

    while (match && (reloc_iter != abs32_locations_.end())) {
      if (section_relocs_iter->r_info != R_ARM_RELATIVE ||
          section_relocs_iter->r_offset != *reloc_iter)
        match = false;

      section_relocs_iter++;
      reloc_iter++;
      file_offset += sizeof(Elf32_Rel);
    }

    if (match) {
      // Skip over relocation tables
      if (!program->EmitElfARMRelocationInstruction())
        return false;
    }
  }

  return ParseSimpleRegion(file_offset, section_end, program);
}

CheckBool DisassemblerElf32ARM::ParseRel32RelocsFromSection(
    const Elf32_Shdr* section_header) {

  uint32 start_file_offset = section_header->sh_offset;
  uint32 end_file_offset = start_file_offset + section_header->sh_size;

  const uint8* start_pointer = OffsetToPointer(start_file_offset);
  const uint8* end_pointer = OffsetToPointer(end_file_offset);

  // Quick way to convert from Pointer to RVA within a single Section is to
  // subtract 'pointer_to_rva'.
  const uint8* const adjust_pointer_to_rva = start_pointer -
                                             section_header->sh_addr;

  // Find the rel32 relocations.
  const uint8* p = start_pointer;
  bool on_32bit = 1; // 32-bit ARM ops appear on 32-bit boundaries, so track it
  while (p < end_pointer) {
    // Heuristic discovery of rel32 locations in instruction stream: are the
    // next few bytes the start of an instruction containing a rel32
    // addressing mode?

    TypedRVAARM* rel32_rva = NULL;
    RVA target_rva = 0;
    bool found = false;

    // 16-bit thumb ops
    if (!found && (p + 3) <= end_pointer) {
      uint16 pval = Read16LittleEndian(p);
      if ((pval & 0xF000) == 0xD000) {
        RVA rva = static_cast<RVA>(p - adjust_pointer_to_rva);

        rel32_rva = new TypedRVAARM(ARM_OFF8, rva);
        if (!rel32_rva->ComputeRelativeTarget((uint8*) p)) {
          return false;
        }
        target_rva = rel32_rva->rva() + rel32_rva->relative_target();
        found = true;
      } else if ((pval & 0xF800) == 0xE000) {
        RVA rva = static_cast<RVA>(p - adjust_pointer_to_rva);

        rel32_rva = new TypedRVAARM(ARM_OFF11, rva);
        if (!rel32_rva->ComputeRelativeTarget((uint8*) p)) {
          return false;
        }
        target_rva = rel32_rva->rva() + rel32_rva->relative_target();
        found = true;
      }
    }

    // thumb-2 ops comprised of two 16-bit words
    if (!found && (p + 5) <= end_pointer) {
      // This is really two 16-bit words, not one 32-bit word.
      uint32 pval = (Read16LittleEndian(p) << 16) | Read16LittleEndian(p + 2);
      if ((pval & 0xF8008000) == 0xF0008000) {
        // Covers thumb-2's 32-bit conditional/unconditional branches

        if ( (pval & (1 << 14)) || (pval & (1 << 12)) ) {
          // A branch, with link, or with link and exchange.
          RVA rva = static_cast<RVA>(p - adjust_pointer_to_rva);

          rel32_rva = new TypedRVAARM(ARM_OFF25, rva);
          if (!rel32_rva->ComputeRelativeTarget((uint8*) p)) {
            return false;
          }
          target_rva = rel32_rva->rva() + rel32_rva->relative_target();
          found = true;
        } else {
          // TODO(paulgazz) make sure cond is not 111
          // A conditional branch instruction
          RVA rva = static_cast<RVA>(p - adjust_pointer_to_rva);

          rel32_rva = new TypedRVAARM(ARM_OFF21, rva);
          if (!rel32_rva->ComputeRelativeTarget((uint8*) p)) {
            return false;
          }
          target_rva = rel32_rva->rva() + rel32_rva->relative_target();
          found = true;
        }
      }
    }

    // 32-bit ARM ops
    if (!found && on_32bit && (p + 5) <= end_pointer) {
      uint32 pval = Read32LittleEndian(p);
      if ((pval & 0x0E000000) == 0x0A000000) {
        // Covers both 0x0A 0x0B ARM relative branches
        RVA rva = static_cast<RVA>(p - adjust_pointer_to_rva);

        rel32_rva = new TypedRVAARM(ARM_OFF24, rva);
        if (!rel32_rva->ComputeRelativeTarget((uint8*) p)) {
          return false;
        }
        target_rva = rel32_rva->rva() + rel32_rva->relative_target();
        found = true;
      }
    }

    if (found && IsValidRVA(target_rva)) {
      rel32_locations_.push_back(rel32_rva);
#if COURGETTE_HISTOGRAM_TARGETS
      ++rel32_target_rvas_[target_rva];
#endif
      p += rel32_rva->op_size();

      // A tricky way to update the on_32bit flag.  Here is the truth table:
      // on_32bit | on_32bit   size is 4
      // ---------+---------------------
      // 1        | 0          0
      // 0        | 0          1
      // 0        | 1          0
      // 1        | 1          1
      on_32bit = (~(on_32bit ^ (rel32_rva->op_size() == 4))) != 0;
    } else {
      // Move 2 bytes at a time, but track 32-bit boundaries
      p += 2;
      on_32bit = ((on_32bit + 1) % 2) != 0;
    }
  }

  return true;
}

}  // namespace courgette
