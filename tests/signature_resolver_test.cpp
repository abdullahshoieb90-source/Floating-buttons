#include <bedrocktools/memory/Signatures.hpp>
#include <bedrocktools/sdk/Offsets.hpp>
#include <cassert>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <unordered_map>

namespace {
bool failResolution = false;
std::size_t patternCount = 0;
}

namespace pl::memory {
std::unordered_map<std::string, std::uintptr_t> resolveSignatures(
    std::span<const std::string> patterns, std::string_view library) {
    assert(library == "test.so");
    patternCount = patterns.size();
    std::unordered_map<std::string, std::uintptr_t> results;
    for (const auto& pattern : patterns) {
        assert(!pattern.empty());
        if (!failResolution) results.emplace(pattern, 0x1000);
    }
    return results;
}
std::uintptr_t resolveVtableFunction(std::string_view rtti, std::size_t slot, std::string_view library) {
    assert(rtti == "16PlayerListPacket" && slot == 17 && library == "test.so");
    return failResolution ? 0 : 0x2000;
}
}

int main() {
    using namespace bedrocktools::memory;
    using namespace bedrocktools::sdk::offsets;
    static_assert(Packet::Size == 0x30 && CommandRequestPacketPayload::mCommand == 0);
    static_assert(CommandRequestPacketPayload::mOrigin == 24 && CommandRequestPacketPayload::mInternalSource == 84);
    static_assert(ClientInstance::mLevelRenderer == 0x1B8);
    assert(resolveAll("test.so"));
    assert(patternCount > 100);
    assert(resolve(SignatureId::VersionString) == 0x1000);
    assert(resolve(SignatureId::GameModeAttackInternal) == 0x1000);
    assert(resolve(SignatureId::MinecraftPacketsCreatePacket) == 0x1000);
    assert(resolve(SignatureId::LoopbackPacketSenderSendToServer) == 0x1000);
    assert(resolve(SignatureId::ClientInstanceGetPacketSender) == 0x1000);
    assert(resolve(SignatureId::PlayerListPacketRead) == 0x2000);
    assert(resolve(SignatureId::RenderPotionEffects) == 0);
    assert(resolve(SignatureId::ItemInHandRenderOffhandItem) == 0);
    assert(resolve(SignatureId::Count) == 0);
    failResolution = true;
    assert(!resolveAll("test.so"));
    assert(resolve(SignatureId::VersionString) == 0);
    assert(resolve(SignatureId::PlayerListPacketRead) == 0);
    failResolution = false;
    assert(resolveAll("test.so"));
    clear();
    assert(resolve(SignatureId::GameModeAttackInternal) == 0);
}
