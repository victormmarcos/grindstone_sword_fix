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

#pragma once

namespace GSF::AnimObjectFix
{
    // Patches the call inside AnimationObjects::Load that loads and attaches
    // an anim object. Call once at kPostLoad. Returns false if the call site
    // does not look as expected (the hook is then skipped, nothing breaks).
    bool InstallHook();

    // Points the grindstone anim object (AnimObjectIronSword) at the model of
    // the Iron Sword that is installed (winning WEAP record). Call at kDataLoaded.
    void ApplySwordModel();
}
