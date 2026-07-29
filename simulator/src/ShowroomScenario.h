#pragma once

#include <cstdint>

#include "data/ProfileData.h"
#include "data/TrophyStats.h"

class AppController;
class WiFiManagerStub;
class PocketPsnHttpClientStub;

// Scenario de demonstration automatique et reproductible ("showroom", voir
// simulator/README.md et --showroom/-Showroom cote simulator/src/main.cpp).
// Pilote les vrais services applicatifs (AppController -> SyncService ->
// TrophyRepository -> PocketPsnProvider) au travers de transports simules
// controles (WiFiManagerStub, PocketPsnHttpClientStub) : aucune donnee ni
// aucun etat affiche n'est fabrique directement ici, uniquement des
// evenements reseau/reponses HTTP simules que les vrais services traitent
// normalement, exactement comme sur un vrai appareil connecte a la vraie
// API Pocket PSN. Le rendu visuel (LVGL, src/ui/) n'est pas concerne par
// cette classe : elle se contente d'orchestrer les vrais services dans le
// temps, l'affichage suit naturellement (gere separement, voir l'ecran rond).
//
// Deux modes d'usage :
//  - automatique (tick()) : traverse les 11 etapes dans l'ordre, avec des
//    delais de maintien penses pour rester observables a l'oeil nu ;
//  - manuel (triggerStep()) : declenche l'action reelle d'une etape donnee
//    a la demande, sans attendre les etapes precedentes -- utile pour
//    capturer chaque etat individuellement (voir simulator/DebugPanel.cpp
//    et les raccourcis clavier de simulator/src/main.cpp).
class ShowroomScenario {
 public:
  enum class Step {
    kStartup,       // demarrage
    kLoading,       // chargement (connexion Wi-Fi)
    kSyncing,       // synchronisation PocketPSN
    kProfileDisplay,// affichage du profil
    kUsingCache,    // utilisation du cache (donnees deja persistees)
    kNetworkLost,   // perte du reseau
    kOffline,       // ecran hors ligne
    kReconnecting,  // reconnexion (fenetre de stabilisation en cours)
    kResyncing,     // nouvelle synchronisation (declenchee automatiquement)
    kApiError,      // erreur API simulee, sans ecraser le cache
    kBackToNormal,  // retour a un etat normal
    kDone,          // scenario termine, reste inactif
  };

  ShowroomScenario(AppController& appController, WiFiManagerStub& wifiStub, PocketPsnHttpClientStub& httpStub);

  // Demarre (ou relance depuis le debut) la sequence automatique.
  void start(uint32_t nowMillis);

  // A appeler a chaque tick de la boucle principale (nowMillis = SDL_GetTicks()
  // ou equivalent portable). Fait avancer automatiquement la sequence quand
  // la condition de sortie de l'etape courante est remplie. Renvoie false une
  // fois kDone atteint (plus rien a faire), true tant que la sequence est en
  // cours.
  bool tick(uint32_t nowMillis);

  // Declenche directement l'action reelle d'une etape (mode manuel) : ne
  // suppose aucun ordre, utilisable a tout moment independamment de la
  // sequence automatique. N'avance pas ensuite tout seul (contrairement a
  // tick()) -- reste sur cette etape jusqu'au prochain start()/triggerStep().
  void triggerStep(Step step, uint32_t nowMillis);

  Step currentStep() const { return step_; }
  bool isRunning() const { return step_ != Step::kDone; }
  static const char* stepLabel(Step step);

  // Instantane du profil/stats pris juste avant l'envoi de la reponse
  // d'erreur simulee (kApiError) -- expose pour permettre de verifier
  // (--selftest ou observation manuelle) que le profil affiche apres
  // l'erreur reste strictement identique, preuve que le cache n'a pas ete
  // ecrase (voir TrophyRepository::validate(), deja responsable du rejet).
  const ProfileData& preErrorProfileSnapshot() const { return preErrorProfile_; }
  const TrophyStats& preErrorStatsSnapshot() const { return preErrorStats_; }

 private:
  void enterStep(Step step, uint32_t nowMillis);
  bool exitConditionMet(uint32_t nowMillis) const;

  AppController& app_;
  WiFiManagerStub& wifiStub_;
  PocketPsnHttpClientStub& httpStub_;

  Step step_ = Step::kStartup;
  uint32_t stepStartedMillis_ = 0;
  bool autoAdvance_ = false;

  // Instantane pris a l'entree de kApiError : permet de prouver que la
  // reponse d'erreur simulee n'a pas ecrase le dernier profil valide (voir
  // TrophyRepository::validate(), deja responsable du rejet cote logique --
  // ceci ne fait que verifier/journaliser le resultat pour la demonstration).
  ProfileData preErrorProfile_;
  TrophyStats preErrorStats_;
};
