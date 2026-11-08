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
#include "GrindstoneSwordFix/RuntimeCompatibility.h"
#include "GrindstoneSwordFix/Version.h"

namespace
{
    constexpr REL::Version kPluginVersion{ GSF::Version::kMajor, GSF::Version::kMinor, GSF::Version::kPatch, 0 };
    constexpr const char* kPluginName = "Grindstone Sword Fix";

    [[nodiscard]] bool SupportsRuntime(const REL::Version& a_version) noexcept
    {
        return GSF::RuntimeCompatibility::Supports(a_version.pack());
    }

    void InitializeLogging()
    {
        std::vector<std::filesystem::path> candidates;
        if (const auto directory = SKSE::log::log_directory()) {
            candidates.push_back(*directory / "GrindstoneSwordFix.log");
        }
        try {
            const auto fallback = std::filesystem::temp_directory_path() / "GrindstoneSwordFix.log";
            if (candidates.empty() || candidates.front() != fallback) {
                candidates.push_back(fallback);
            }
        } catch (const std::exception&) {
        }

        for (const auto& path : candidates) {
            try {
                auto sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(path.string(), true);
                auto logger = std::make_shared<spdlog::logger>("GrindstoneSwordFix", std::move(sink));
                spdlog::set_default_logger(std::move(logger));
                spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] %v");
                spdlog::set_level(spdlog::level::info);
                spdlog::flush_on(spdlog::level::info);
                return;
            } catch (const std::exception&) {
                // Logging must never stop the plugin from loading.
            }
        }
    }

    void OnSKSEMessage(SKSE::MessagingInterface::Message* a_message)
    {
        if (a_message && a_message->type == SKSE::MessagingInterface::kDataLoaded) {
            GSF::AnimObjectFix::ApplySwordModel();
        }
    }
}

SKSE_PLUGIN_VERSION = []() constexpr {
    SKSE::PluginVersionData data;
    data.PluginVersion(kPluginVersion);
    data.PluginName(kPluginName);
    data.AuthorName("Grindstone Sword Fix");
    data.UsesNoStructs();
    for (std::size_t index = 0; index < GSF::RuntimeCompatibility::Versions.size(); ++index) {
        data.compatibleVersions[index] = GSF::RuntimeCompatibility::Versions[index];
    }
    return data;
}();

SKSE_EXPORT bool SKSEPlugin_Query(const SKSE::QueryInterface* a_skse, SKSE::PluginInfo* a_info)
{
    if (!a_skse || !a_info) {
        return false;
    }
    a_info->infoVersion = SKSE::PluginInfo::kVersion;
    a_info->name = kPluginName;
    a_info->version = kPluginVersion.pack();
    return !a_skse->IsEditor() && SupportsRuntime(a_skse->RuntimeVersion());
}

SKSE_PLUGIN_LOAD(const SKSE::LoadInterface* a_skse)
{
    if (!a_skse || a_skse->IsEditor() || !SupportsRuntime(a_skse->RuntimeVersion())) {
        return false;
    }
    SKSE::Init(a_skse);
    InitializeLogging();
    SKSE::log::info(
        "{} {} is loading. Skyrim={}, SKSE={}.",
        kPluginName,
        kPluginVersion.string(),
        a_skse->RuntimeVersion().string(),
        REL::Version::unpack(a_skse->SKSEVersion()).string());

    if (!GSF::AnimObjectFix::InstallHook()) {
        // Without the hook the swap would put the sword on the hip; skip it.
        return true;
    }

    const auto messaging = SKSE::GetMessagingInterface();
    if (!messaging || !messaging->RegisterListener(OnSKSEMessage)) {
        SKSE::log::critical("Could not register the SKSE messaging listener.");
        return false;
    }
    return true;
}
