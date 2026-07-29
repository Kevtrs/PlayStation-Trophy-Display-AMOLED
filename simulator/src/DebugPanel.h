#pragma once

#include <lvgl.h>

#include "ShowroomScenario.h"
#include "WiFiManagerStub.h"
#include "app/AppController.h"
#include "data/DemoDataProvider.h"

// Panneau de debug du simulateur : fenetre separee, jamais dans le rendu
// circulaire principal. Toutes les modifications passent par des methodes
// dediees d'AppController (debugApplySettings/debugSetDisplayedData) ou du
// WiFiManagerStub (simulateConnected/simulateDisconnected/...) -- jamais
// d'ecriture directe dans UiManager::state(), qui serait ecrasee au
// prochain tick() (voir HANDOFF_PROGRESS.md).
namespace DebugPanel {
lv_obj_t* create(AppController* appController, DemoDataProvider* provider, WiFiManagerStub* wifiStub,
                 ShowroomScenario* showroomScenario);
}  // namespace DebugPanel
