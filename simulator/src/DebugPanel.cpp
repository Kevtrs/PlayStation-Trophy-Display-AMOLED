#include "DebugPanel.h"

#include <SDL.h>

#include <cstdio>

namespace DebugPanel {

namespace {
constexpr int kRowHeight = 34;
int nextY_ = 8;

// Les steppers modifient directement un champ de DemoDataProvider (source
// de verite en mode demo), puis republient immediatement le resultat via
// AppController::debugSetDisplayedData() -- sans cela, la valeur
// resterait invisible jusqu'a la prochaine synchronisation (ou serait a
// nouveau ecrasee, voir HANDOFF_PROGRESS.md).
struct IntStepperCtx {
  int* value;
  int step, minV, maxV;
  lv_obj_t* label;
  const char* suffix;
  AppController* appController;
  DemoDataProvider* provider;
};
struct FloatStepperCtx {
  float* value;
  float step, minV, maxV;
  lv_obj_t* label;
  const char* suffix;
  AppController* appController;
  DemoDataProvider* provider;
};

void refreshIntLabel(IntStepperCtx* ctx) {
  char buf[32];
  std::snprintf(buf, sizeof(buf), "%d%s", *ctx->value, ctx->suffix);
  lv_label_set_text(ctx->label, buf);
}
void refreshFloatLabel(FloatStepperCtx* ctx) {
  char buf[32];
  std::snprintf(buf, sizeof(buf), "%.0f%s", *ctx->value, ctx->suffix);
  lv_label_set_text(ctx->label, buf);
}

void publishProviderData(AppController* appController, DemoDataProvider* provider) {
  appController->debugSetDisplayedData(provider->profile(), provider->stats());
}

void onIntStep(lv_event_t* e) {
  auto* ctx = static_cast<IntStepperCtx*>(lv_event_get_user_data(e));
  int delta = reinterpret_cast<intptr_t>(lv_obj_get_user_data(lv_event_get_target_obj(e)));
  *ctx->value += delta * ctx->step;
  if (*ctx->value < ctx->minV) *ctx->value = ctx->minV;
  if (*ctx->value > ctx->maxV) *ctx->value = ctx->maxV;
  refreshIntLabel(ctx);
  publishProviderData(ctx->appController, ctx->provider);
}
void onFloatStep(lv_event_t* e) {
  auto* ctx = static_cast<FloatStepperCtx*>(lv_event_get_user_data(e));
  int delta = reinterpret_cast<intptr_t>(lv_obj_get_user_data(lv_event_get_target_obj(e)));
  *ctx->value += delta * ctx->step;
  if (*ctx->value < ctx->minV) *ctx->value = ctx->minV;
  if (*ctx->value > ctx->maxV) *ctx->value = ctx->maxV;
  refreshFloatLabel(ctx);
  publishProviderData(ctx->appController, ctx->provider);
}

lv_obj_t* makeButton(lv_obj_t* parent, const char* text, lv_align_t align, int xOfs, int yOfs, intptr_t userData,
                     lv_event_cb_t cb, void* eventUserData) {
  lv_obj_t* btn = lv_btn_create(parent);
  lv_obj_set_size(btn, 32, 26);
  lv_obj_align(btn, align, xOfs, yOfs);
  lv_obj_set_user_data(btn, reinterpret_cast<void*>(userData));
  lv_obj_add_event_cb(btn, cb, LV_EVENT_CLICKED, eventUserData);
  lv_obj_t* lbl = lv_label_create(btn);
  lv_label_set_text(lbl, text);
  lv_obj_center(lbl);
  return btn;
}

void makeIntStepperRow(lv_obj_t* parent, const char* label, int* value, int step, int minV, int maxV,
                       AppController* appController, DemoDataProvider* provider, const char* suffix = "") {
  lv_obj_t* row = lv_obj_create(parent);
  lv_obj_set_size(row, 300, kRowHeight);
  lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_width(row, 0, 0);
  lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_align(row, LV_ALIGN_TOP_LEFT, 0, nextY_);
  nextY_ += kRowHeight + 2;

  lv_obj_t* lbl = lv_label_create(row);
  lv_label_set_text(lbl, label);
  lv_obj_align(lbl, LV_ALIGN_LEFT_MID, 4, 0);

  auto* ctx = new IntStepperCtx{value, step, minV, maxV, nullptr, suffix, appController, provider};
  lv_obj_t* valueLabel = lv_label_create(row);
  ctx->label = valueLabel;
  lv_obj_align(valueLabel, LV_ALIGN_RIGHT_MID, -74, 0);
  refreshIntLabel(ctx);

  makeButton(row, "-", LV_ALIGN_RIGHT_MID, -36, 0, -1, onIntStep, ctx);
  makeButton(row, "+", LV_ALIGN_RIGHT_MID, 0, 0, 1, onIntStep, ctx);
}

void makeFloatStepperRow(lv_obj_t* parent, const char* label, float* value, float step, float minV, float maxV,
                         AppController* appController, DemoDataProvider* provider, const char* suffix = "") {
  lv_obj_t* row = lv_obj_create(parent);
  lv_obj_set_size(row, 300, kRowHeight);
  lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_width(row, 0, 0);
  lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_align(row, LV_ALIGN_TOP_LEFT, 0, nextY_);
  nextY_ += kRowHeight + 2;

  lv_obj_t* lbl = lv_label_create(row);
  lv_label_set_text(lbl, label);
  lv_obj_align(lbl, LV_ALIGN_LEFT_MID, 4, 0);

  auto* ctx = new FloatStepperCtx{value, step, minV, maxV, nullptr, suffix, appController, provider};
  lv_obj_t* valueLabel = lv_label_create(row);
  ctx->label = valueLabel;
  lv_obj_align(valueLabel, LV_ALIGN_RIGHT_MID, -74, 0);
  refreshFloatLabel(ctx);

  makeButton(row, "-", LV_ALIGN_RIGHT_MID, -36, 0, -1, onFloatStep, ctx);
  makeButton(row, "+", LV_ALIGN_RIGHT_MID, 0, 0, 1, onFloatStep, ctx);
}

struct PresetCtx {
  AppController* appController;
  DemoDataProvider* provider;
};

void onUsernamePreset(lv_event_t* e) {
  auto* ctx = static_cast<PresetCtx*>(lv_event_get_user_data(e));
  const char* name = static_cast<const char*>(lv_obj_get_user_data(lv_event_get_target_obj(e)));
  ctx->provider->mutableProfile().username = name;
  publishProviderData(ctx->appController, ctx->provider);
}

void onWifiButton(lv_event_t* e) {
  auto* wifiStub = static_cast<WiFiManagerStub*>(lv_event_get_user_data(e));
  auto status = static_cast<WifiState>(reinterpret_cast<intptr_t>(lv_obj_get_user_data(lv_event_get_target_obj(e))));
  switch (status) {
    case WifiState::kDisconnected:
      wifiStub->simulateDisconnected();
      break;
    case WifiState::kConnecting:
      wifiStub->begin("DebugSSID", "");  // aboutit a kConnected apres un court delai simule
      break;
    case WifiState::kConnected:
      wifiStub->simulateConnected();
      break;
    case WifiState::kAccessPoint:
      wifiStub->simulateAccessPoint();
      break;
    case WifiState::kError:
      wifiStub->simulateError();
      break;
  }
}

void onShowroomStepButton(lv_event_t* e) {
  auto* scenario = static_cast<ShowroomScenario*>(lv_event_get_user_data(e));
  // SDL_GetTicks() -- meme base de temps que showroomScenario.tick() dans la
  // boucle principale (voir simulator/src/main.cpp) : ne jamais melanger avec
  // lv_tick_get(), qui n'a aucune garantie de rester rigoureusement identique.
  auto step = static_cast<ShowroomScenario::Step>(reinterpret_cast<intptr_t>(lv_obj_get_user_data(lv_event_get_target_obj(e))));
  scenario->triggerStep(step, SDL_GetTicks());
}

void onShowroomAutoButton(lv_event_t* e) {
  auto* scenario = static_cast<ShowroomScenario*>(lv_event_get_user_data(e));
  scenario->start(SDL_GetTicks());
}

void onAnimationsSwitch(lv_event_t* e) {
  auto* appController = static_cast<AppController*>(lv_event_get_user_data(e));
  lv_obj_t* sw = lv_event_get_target_obj(e);
  AppSettings settings = appController->state().settings;
  settings.animationsEnabled = lv_obj_has_state(sw, LV_STATE_CHECKED);
  appController->debugApplySettings(settings);
}

void onAutoRotateSwitch(lv_event_t* e) {
  auto* appController = static_cast<AppController*>(lv_event_get_user_data(e));
  lv_obj_t* sw = lv_event_get_target_obj(e);
  AppSettings settings = appController->state().settings;
  settings.autoRotateEnabled = lv_obj_has_state(sw, LV_STATE_CHECKED);
  appController->debugApplySettings(settings);
}

void onBrightnessSlider(lv_event_t* e) {
  auto* appController = static_cast<AppController*>(lv_event_get_user_data(e));
  AppSettings settings = appController->state().settings;
  settings.brightnessPercent = lv_slider_get_value(lv_event_get_target_obj(e));
  appController->debugApplySettings(settings);
}

void onLanguageDropdown(lv_event_t* e) {
  auto* appController = static_cast<AppController*>(lv_event_get_user_data(e));
  uint16_t sel = lv_dropdown_get_selected(lv_event_get_target_obj(e));
  AppSettings settings = appController->state().settings;
  settings.language = sel == 0 ? AppLanguage::kFrench : AppLanguage::kEnglish;
  appController->debugApplySettings(settings);
}

lv_obj_t* makeSectionLabel(lv_obj_t* parent, const char* text) {
  lv_obj_t* lbl = lv_label_create(parent);
  lv_label_set_text(lbl, text);
  lv_obj_set_style_text_color(lbl, lv_palette_main(LV_PALETTE_BLUE), 0);
  lv_obj_align(lbl, LV_ALIGN_TOP_LEFT, 0, nextY_);
  nextY_ += 22;
  return lbl;
}
}  // namespace

lv_obj_t* create(AppController* appController, DemoDataProvider* provider, WiFiManagerStub* wifiStub,
                 ShowroomScenario* showroomScenario) {
  lv_obj_t* panel = lv_obj_create(nullptr);
  lv_obj_set_style_bg_color(panel, lv_color_hex(0x1E1E28), 0);
  lv_obj_set_style_pad_all(panel, 10, 0);
  lv_obj_set_flex_flow(panel, LV_FLEX_FLOW_COLUMN);
  lv_obj_clear_flag(panel, LV_OBJ_FLAG_SCROLLABLE);

  TrophyStats& s = provider->mutableStats();
  ProfileData& p = provider->mutableProfile();
  nextY_ = 8;

  makeSectionLabel(panel, "Profil");
  lv_obj_t* presetRow = lv_obj_create(panel);
  lv_obj_set_size(presetRow, 320, 30);
  lv_obj_set_style_bg_opa(presetRow, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_width(presetRow, 0, 0);
  lv_obj_clear_flag(presetRow, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_align(presetRow, LV_ALIGN_TOP_LEFT, 0, nextY_);
  nextY_ += 34;
  static const char* kPresets[] = {"Kevin_Trophies", "DemoPlayer", "TestUser"};
  auto* presetCtx = new PresetCtx{appController, provider};
  for (int i = 0; i < 3; ++i) {
    lv_obj_t* btn = lv_btn_create(presetRow);
    lv_obj_set_size(btn, 95, 28);
    lv_obj_align(btn, LV_ALIGN_LEFT_MID, i * 105, 0);
    lv_obj_set_user_data(btn, const_cast<char*>(kPresets[i]));
    lv_obj_add_event_cb(btn, onUsernamePreset, LV_EVENT_CLICKED, presetCtx);
    lv_obj_t* lbl = lv_label_create(btn);
    lv_label_set_text(lbl, kPresets[i]);
    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_14, 0);
    lv_obj_center(lbl);
  }

  makeSectionLabel(panel, "Niveau et progression");
  makeIntStepperRow(panel, "Niveau PSN", &p.level, 1, 1, 999, appController, provider);
  makeIntStepperRow(panel, "Progression niveau", &p.levelProgressPercent, 5, 0, 100, appController, provider, "%");

  makeSectionLabel(panel, "Trophees");
  makeIntStepperRow(panel, "Platine", &s.platinum, 1, 0, 9999, appController, provider);
  makeIntStepperRow(panel, "Or", &s.gold, 5, 0, 99999, appController, provider);
  makeIntStepperRow(panel, "Argent", &s.silver, 5, 0, 99999, appController, provider);
  makeIntStepperRow(panel, "Bronze", &s.bronze, 10, 0, 999999, appController, provider);

  makeSectionLabel(panel, "Statistiques");
  makeIntStepperRow(panel, "Jeux termines", &s.gamesCompleted, 1, 0, 9999, appController, provider);
  makeFloatStepperRow(panel, "Taux de completion", &s.completionRatePercent, 1, 0, 100, appController, provider, "%");
  makeIntStepperRow(panel, "Rang mondial", &s.worldRank, 100, 0, 999999, appController, provider);
  makeFloatStepperRow(panel, "Temps de jeu", &s.playtimeHours, 10, 0, 999999, appController, provider, "h");

  makeSectionLabel(panel, "Reseau / etat (WiFiManagerStub)");
  lv_obj_t* wifiRow = lv_obj_create(panel);
  lv_obj_set_size(wifiRow, 320, 30);
  lv_obj_set_style_bg_opa(wifiRow, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_width(wifiRow, 0, 0);
  lv_obj_clear_flag(wifiRow, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_align(wifiRow, LV_ALIGN_TOP_LEFT, 0, nextY_);
  nextY_ += 34;
  struct WifiButtonSpec {
    WifiState state;
    const char* label;
  };
  // Ordre correspond a WifiState (voir data/NetworkStatus.h) : Deconnecte,
  // Connexion (transitoire, aboutit a Connecte apres un delai simule),
  // Connecte, Point d'acces (bascule de secours).
  static const WifiButtonSpec kWifiButtons[] = {
      {WifiState::kDisconnected, "Deconnecte"},
      {WifiState::kConnecting, "Connexion"},
      {WifiState::kConnected, "Connecte"},
      {WifiState::kAccessPoint, "Point d'acces"},
  };
  for (int i = 0; i < 4; ++i) {
    lv_obj_t* btn = lv_btn_create(wifiRow);
    lv_obj_set_size(btn, 72, 28);
    lv_obj_align(btn, LV_ALIGN_LEFT_MID, i * 80, 0);
    lv_obj_set_user_data(btn, reinterpret_cast<void*>(static_cast<intptr_t>(kWifiButtons[i].state)));
    lv_obj_add_event_cb(btn, onWifiButton, LV_EVENT_CLICKED, wifiStub);
    lv_obj_t* lbl = lv_label_create(btn);
    lv_label_set_text(lbl, kWifiButtons[i].label);
    lv_obj_set_style_text_font(lbl, &lv_font_montserrat_14, 0);
    lv_obj_center(lbl);
  }

  // Remarque : pas de bouton "Pocket PSN verifie (simule)" ici -- cet
  // indicateur ne doit jamais etre force artificiellement (voir AUDIT.md /
  // PocketPsnProvider::isVerified() : il ne devient vrai qu'apres un test
  // reseau reel reussi).

  makeSectionLabel(panel, "Affichage");
  lv_obj_t* brightRow = lv_obj_create(panel);
  lv_obj_set_size(brightRow, 320, kRowHeight);
  lv_obj_set_style_bg_opa(brightRow, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_width(brightRow, 0, 0);
  lv_obj_clear_flag(brightRow, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_align(brightRow, LV_ALIGN_TOP_LEFT, 0, nextY_);
  nextY_ += kRowHeight + 2;
  lv_obj_t* brightLbl = lv_label_create(brightRow);
  lv_label_set_text(brightLbl, "Luminosite");
  lv_obj_align(brightLbl, LV_ALIGN_LEFT_MID, 4, 0);
  lv_obj_t* brightSlider = lv_slider_create(brightRow);
  lv_obj_set_width(brightSlider, 140);
  lv_slider_set_range(brightSlider, 5, 100);
  lv_slider_set_value(brightSlider, appController->state().settings.brightnessPercent, LV_ANIM_OFF);
  lv_obj_align(brightSlider, LV_ALIGN_RIGHT_MID, 0, 0);
  lv_obj_add_event_cb(brightSlider, onBrightnessSlider, LV_EVENT_VALUE_CHANGED, appController);

  lv_obj_t* langRow = lv_obj_create(panel);
  lv_obj_set_size(langRow, 320, kRowHeight);
  lv_obj_set_style_bg_opa(langRow, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_width(langRow, 0, 0);
  lv_obj_clear_flag(langRow, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_align(langRow, LV_ALIGN_TOP_LEFT, 0, nextY_);
  nextY_ += kRowHeight + 2;
  lv_obj_t* langLbl = lv_label_create(langRow);
  lv_label_set_text(langLbl, "Langue");
  lv_obj_align(langLbl, LV_ALIGN_LEFT_MID, 4, 0);
  lv_obj_t* langDropdown = lv_dropdown_create(langRow);
  lv_dropdown_set_options(langDropdown, "Francais\nEnglish");
  lv_obj_set_width(langDropdown, 140);
  lv_obj_align(langDropdown, LV_ALIGN_RIGHT_MID, 0, 0);
  lv_obj_add_event_cb(langDropdown, onLanguageDropdown, LV_EVENT_VALUE_CHANGED, appController);

  lv_obj_t* animRow = lv_obj_create(panel);
  lv_obj_set_size(animRow, 320, kRowHeight);
  lv_obj_set_style_bg_opa(animRow, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_width(animRow, 0, 0);
  lv_obj_clear_flag(animRow, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_align(animRow, LV_ALIGN_TOP_LEFT, 0, nextY_);
  nextY_ += kRowHeight + 2;
  lv_obj_t* animLbl = lv_label_create(animRow);
  lv_label_set_text(animLbl, "Animations");
  lv_obj_align(animLbl, LV_ALIGN_LEFT_MID, 4, 0);
  lv_obj_t* animSw = lv_switch_create(animRow);
  if (appController->state().settings.animationsEnabled) lv_obj_add_state(animSw, LV_STATE_CHECKED);
  lv_obj_align(animSw, LV_ALIGN_RIGHT_MID, 0, 0);
  lv_obj_add_event_cb(animSw, onAnimationsSwitch, LV_EVENT_VALUE_CHANGED, appController);

  lv_obj_t* rotRow = lv_obj_create(panel);
  lv_obj_set_size(rotRow, 320, kRowHeight);
  lv_obj_set_style_bg_opa(rotRow, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_width(rotRow, 0, 0);
  lv_obj_clear_flag(rotRow, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_align(rotRow, LV_ALIGN_TOP_LEFT, 0, nextY_);
  nextY_ += kRowHeight + 2;
  lv_obj_t* rotLbl = lv_label_create(rotRow);
  lv_label_set_text(rotLbl, "Rotation auto");
  lv_obj_align(rotLbl, LV_ALIGN_LEFT_MID, 4, 0);
  lv_obj_t* rotSw = lv_switch_create(rotRow);
  if (appController->state().settings.autoRotateEnabled) lv_obj_add_state(rotSw, LV_STATE_CHECKED);
  lv_obj_align(rotSw, LV_ALIGN_RIGHT_MID, 0, 0);
  lv_obj_add_event_cb(rotSw, onAutoRotateSwitch, LV_EVENT_VALUE_CHANGED, appController);

  // Mode manuel du scenario de demonstration ("showroom", voir
  // ShowroomScenario.h) : chaque bouton declenche directement l'action
  // reelle de l'etape correspondante (WiFiManagerStub/PocketPsnHttpClientStub
  // reellement pilotes, pas d'etat fabrique) -- utile pour capturer chaque
  // etat individuellement, sans devoir suivre la sequence complete.
  // Memes raccourcis clavier disponibles depuis la fenetre principale (voir
  // simulator/src/main.cpp et simulator/README.md). Chaque rangee de boutons
  // est enveloppee dans son propre conteneur (meme motif que wifiRow
  // ci-dessus), necessaire pour un positionnement horizontal fiable des
  // boutons a l'interieur d'un panneau parent en flex-flow colonne.
  makeSectionLabel(panel, "Showroom (demonstration)");
  struct ShowroomButtonSpec {
    ShowroomScenario::Step step;
    const char* label;
  };
  static const ShowroomButtonSpec kShowroomButtons[] = {
      {ShowroomScenario::Step::kStartup, "1.Demarr."},      {ShowroomScenario::Step::kLoading, "2.Charg."},
      {ShowroomScenario::Step::kSyncing, "3.Sync"},         {ShowroomScenario::Step::kProfileDisplay, "4.Profil"},
      {ShowroomScenario::Step::kUsingCache, "5.Cache"},     {ShowroomScenario::Step::kNetworkLost, "6.Perte"},
      {ShowroomScenario::Step::kOffline, "7.Hors ligne"},   {ShowroomScenario::Step::kReconnecting, "8.Reco"},
      {ShowroomScenario::Step::kResyncing, "9.Resync"},     {ShowroomScenario::Step::kApiError, "10.Erreur"},
      {ShowroomScenario::Step::kBackToNormal, "11.Normal"},
  };
  constexpr int kShowroomCols = 4;
  constexpr int kShowroomBtnCount = sizeof(kShowroomButtons) / sizeof(kShowroomButtons[0]);
  for (int i = 0; i < kShowroomBtnCount; ++i) {
    if (i % kShowroomCols == 0) {
      lv_obj_t* row = lv_obj_create(panel);
      lv_obj_set_size(row, 320, 30);
      lv_obj_set_style_bg_opa(row, LV_OPA_TRANSP, 0);
      lv_obj_set_style_border_width(row, 0, 0);
      lv_obj_clear_flag(row, LV_OBJ_FLAG_SCROLLABLE);
      lv_obj_align(row, LV_ALIGN_TOP_LEFT, 0, nextY_);
      nextY_ += 32;
      for (int j = i; j < kShowroomBtnCount && j < i + kShowroomCols; ++j) {
        lv_obj_t* btn = lv_btn_create(row);
        lv_obj_set_size(btn, 76, 28);
        lv_obj_align(btn, LV_ALIGN_LEFT_MID, (j - i) * 80, 0);
        lv_obj_set_user_data(btn, reinterpret_cast<void*>(static_cast<intptr_t>(kShowroomButtons[j].step)));
        lv_obj_add_event_cb(btn, onShowroomStepButton, LV_EVENT_CLICKED, showroomScenario);
        lv_obj_t* lbl = lv_label_create(btn);
        lv_label_set_text(lbl, kShowroomButtons[j].label);
        lv_obj_set_style_text_font(lbl, &lv_font_montserrat_14, 0);
        lv_obj_center(lbl);
      }
    }
  }
  lv_obj_t* autoRow = lv_obj_create(panel);
  lv_obj_set_size(autoRow, 320, 30);
  lv_obj_set_style_bg_opa(autoRow, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_width(autoRow, 0, 0);
  lv_obj_clear_flag(autoRow, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_align(autoRow, LV_ALIGN_TOP_LEFT, 0, nextY_);
  nextY_ += 34;
  lv_obj_t* autoBtn = lv_btn_create(autoRow);
  lv_obj_set_size(autoBtn, 320, 28);
  lv_obj_align(autoBtn, LV_ALIGN_LEFT_MID, 0, 0);
  lv_obj_add_event_cb(autoBtn, onShowroomAutoButton, LV_EVENT_CLICKED, showroomScenario);
  lv_obj_t* autoLbl = lv_label_create(autoBtn);
  lv_label_set_text(autoLbl, "Lancer la sequence automatique complete");
  lv_obj_set_style_text_font(autoLbl, &lv_font_montserrat_14, 0);
  lv_obj_center(autoLbl);

  lv_obj_t* hint = lv_label_create(panel);
  lv_label_set_text(hint,
                    "Raccourcis (fenetre principale) :\n"
                    "Fleches = page  |  R = sync  |  E = erreur\n"
                    "N = nouveau trophee  |  D = mode demo  |  F = debug\n"
                    "1-9,0,- = etat showroom  |  Espace = sequence auto");
  lv_obj_set_style_text_color(hint, lv_color_hex(0x888888), 0);
  lv_obj_align(hint, LV_ALIGN_TOP_LEFT, 0, nextY_ + 6);

  return panel;
}

}  // namespace DebugPanel
