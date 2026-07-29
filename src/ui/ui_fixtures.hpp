#pragma once

#include "ui/ui_model.hpp"

#include <cstddef>

namespace trophy {

ProfileData profile_for_fixture(UiFixture fixture, AppLanguage lang);
const TrophyEntry *trophy_entries_for_fixture(UiFixture fixture, std::size_t *count);
const char *fixture_label(UiFixture fixture, AppLanguage lang);
UiFixture fixture_from_index(int index);
int fixture_index(UiFixture fixture);

} // namespace trophy
