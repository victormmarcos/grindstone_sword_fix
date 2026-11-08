// Grindstone Sword Fix
// Copyright (C) 2026 victormmarcos
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program. If not, see <https://www.gnu.org/licenses/>.

#include "GrindstoneSwordFix/AnimObjectFix.h"

namespace GSF::AnimObjectFix
{
    namespace
    {
        // Skyrim.esm records.
        constexpr RE::FormID kAnimObjectIronSword = 0x0010FD00;  // ANIO used by the grindstone idle
        constexpr RE::FormID kIronSword = 0x00012EB7;            // WEAP "Iron Sword"

        // The anim object nif carries Prn=AnimObjectR (right hand). A weapon
        // nif carries Prn=WeaponSword (the hip sheath), so after the swap the
        // object may land on the sheath node and has to be moved to the hand.
        constexpr std::string_view kHandNode = "AnimObjectR"sv;
        constexpr std::string_view kScabbardNode = "Scb"sv;

        // Model of AnimObjectIronSword, set at kDataLoaded. Compared by pointer
        // inside the hook, so other anim objects are never touched.
        RE::TESModel* g_targetModel = nullptr;
        bool g_loggedFirstAttach = false;

        [[nodiscard]] std::string_view NameOf(const RE::NiAVObject* a_object)
        {
            if (!a_object) {
                return "<none>"sv;
            }
            const char* name = a_object->name.c_str();
            return name ? std::string_view{ name } : std::string_view{};
        }

        [[nodiscard]] RE::NiNode* FindHandNode(RE::TESObjectREFR* a_ref, RE::NiAVObject* a_root)
        {
            const RE::BSFixedString handName{ kHandNode.data() };
            if (a_root) {
                if (const auto node = a_root->GetObjectByName(handName)) {
                    return node->AsNode();
                }
            }
            if (a_ref) {
                if (const auto root3D = a_ref->Get3D()) {
                    if (const auto node = root3D->GetObjectByName(handName)) {
                        return node->AsNode();
                    }
                }
            }
            return nullptr;
        }

        void FixAttachedSword(RE::NiAVObject* a_sword, RE::TESObjectREFR* a_ref, RE::NiAVObject* a_root)
        {
            // Hide the scabbard that weapon meshes carry. The engine hides it
            // for drawn weapons, but not for anim objects.
            if (const auto scabbard = a_sword->GetObjectByName(RE::BSFixedString{ kScabbardNode.data() })) {
                scabbard->SetAppCulled(true);
            }

            const auto parent = a_sword->parent;
            if (!g_loggedFirstAttach) {
                g_loggedFirstAttach = true;
                SKSE::log::info(
                    "First grindstone sword attached: object='{}', parent='{}', actor={:08X}.",
                    NameOf(a_sword),
                    NameOf(parent),
                    a_ref ? a_ref->GetFormID() : 0u);
            }

            if (parent && NameOf(parent) == kHandNode) {
                return;
            }

            const auto hand = FindHandNode(a_ref, a_root);
            if (!hand) {
                SKSE::log::warn(
                    "Could not find node '{}' for actor {:08X}; sword left on '{}'.",
                    kHandNode,
                    a_ref ? a_ref->GetFormID() : 0u,
                    NameOf(parent));
                return;
            }

            // Keep the object alive while it changes parent.
            RE::NiPointer<RE::NiAVObject> keepAlive{ a_sword };
            if (parent) {
                parent->DetachChild(a_sword);
            }
            hand->AttachChild(a_sword, true);

            RE::NiUpdateData updateData;
            a_sword->Update(updateData);
        }

        struct LoadAndAttachAddon
        {
            // Engine call inside AnimationObjects::Load. The 2nd and 4th
            // arguments (biped slot and BipedAnim smart pointer) are passed
            // through untouched, so they are kept opaque here.
            static RE::NiAVObject* thunk(
                RE::TESModel* a_model,
                std::uint32_t a_bipedSlot,
                RE::TESObjectREFR* a_ref,
                void* a_biped,
                RE::NiAVObject* a_root)
            {
                const auto result = func(a_model, a_bipedSlot, a_ref, a_biped, a_root);
                if (result && g_targetModel && a_model == g_targetModel) {
                    FixAttachedSword(result, a_ref, a_root);
                }
                return result;
            }

            static inline REL::Relocation<decltype(thunk)> func;
        };
    }

    bool InstallHook()
    {
        // AnimationObjects::Load, call to the "load and attach addon" function.
        // Same call site used by powerofthree's AnimObject Swapper.
        const auto address =
            REL::RelocationID(42420, 43576).address() + REL::Relocate(0x22A, 0x21F);

        if (*reinterpret_cast<const std::uint8_t*>(address) != 0xE8) {
            SKSE::log::error(
                "Unexpected code at the AnimationObjects::Load call site ({:X}); hook not installed.",
                address);
            return false;
        }

        SKSE::AllocTrampoline(14);
        auto& trampoline = SKSE::GetTrampoline();
        LoadAndAttachAddon::func = trampoline.write_call<5>(address, LoadAndAttachAddon::thunk);
        SKSE::log::info("Installed AnimationObjects::Load hook.");
        return true;
    }

    void ApplySwordModel()
    {
        const auto animObject = RE::TESForm::LookupByID<RE::TESObjectANIO>(kAnimObjectIronSword);
        const auto ironSword = RE::TESForm::LookupByID<RE::TESObjectWEAP>(kIronSword);
        if (!animObject || !ironSword) {
            SKSE::log::error(
                "Missing records: AnimObjectIronSword={}, IronSword={}. Nothing changed.",
                animObject != nullptr,
                ironSword != nullptr);
            return;
        }

        const char* swordModel = ironSword->GetModel();
        if (!swordModel || !*swordModel) {
            SKSE::log::error("Iron Sword has no model path. Nothing changed.");
            return;
        }

        const char* oldModel = animObject->GetModel();
        SKSE::log::info(
            "AnimObjectIronSword model: '{}' -> '{}' (Iron Sword from '{}').",
            oldModel ? oldModel : "",
            swordModel,
            ironSword->GetFile(0) ? ironSword->GetFile(0)->GetFilename() : "?"sv);

        animObject->SetModel(swordModel);
        g_targetModel = animObject;
    }
}
