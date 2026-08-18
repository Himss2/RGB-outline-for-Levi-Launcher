// Tambahkan di paling bawah file src/resolver.cpp dalam namespace outline::resolver:

std::uintptr_t resolveAdrlTarget(
    std::uintptr_t pcAddress,
    std::uint32_t adrpInst,
    std::uint32_t addInst
) {
    if (!pcAddress) return 0;

    // 1. Validasi Opcode ADRP (Bitmask 0x9F000000 == 0x90000000)
    if ((adrpInst & 0x9F000000) != 0x90000000) {
        return 0;
    }

    // 2. Ekstrak immediate 21-bit dari ADRP
    std::int64_t immlo = (adrpInst >> 29) & 0x3;
    std::int64_t immhi = (adrpInst >> 5) & 0x7FFFF;
    std::int64_t imm = (immhi << 2) | immlo;

    // Sign extension jika bernilai negatif
    if (imm & (1ULL << 20)) {
        imm |= ~((1ULL << 21) - 1);
    }

    // Hitung Page Base (Alignment 4KB / 0x1000)
    std::uintptr_t pcPage = pcAddress & ~0xFFFULL;
    std::uintptr_t targetPage = pcPage + static_cast<std::uintptr_t>(imm << 12);

    // 3. Validasi Opcode ADD Immediate 64-bit
    if ((addInst & 0x11000000) != 0x11000000) {
        return targetPage;
    }

    std::uint32_t imm12 = (addInst >> 10) & 0xFFF;
    std::uint32_t shift = (addInst >> 22) & 0x3;
    if (shift == 1) {
        imm12 <<= 12;
    }

    return targetPage + imm12;
}

std::uintptr_t fetchDynamicMaterialPtr(std::uintptr_t callSiteAddress) {
    if (!callSiteAddress) return 0;

    // Cek ketersediaan memori terbaca sebelum dereference
    if (!readableRangeContains(callSiteAddress - 8, sizeof(std::uint32_t) * 2)) {
        return 0;
    }

    const auto* codePtr = reinterpret_cast<const std::uint32_t*>(callSiteAddress - 8);

    std::uint32_t adrpOpcode = codePtr[0];
    std::uint32_t addOpcode  = codePtr[1];

    return resolveAdrlTarget(
        callSiteAddress - 8,
        adrpOpcode,
        addOpcode
    );
}

} // namespace outline::resolver
