#include "data/ProviderFactory.h"

#include "secrets.h"

namespace ProviderFactory {

std::string effectiveApiKey(const AppSettings& settings) {
  // La cle saisie manuellement (portail captif) reste prioritaire si
  // presente -- permet a un utilisateur avance de remplacer la cle
  // partagee compilee dans le firmware par la sienne. Sinon, repli sur
  // POCKETPSN_SHARED_API_KEY (voir include/secrets.example.h) : une seule
  // cle, obtenue une fois aupres du createur de Pocket PSN, compilee dans
  // le firmware distribue (ex: MakerWorld) pour que chaque utilisateur
  // final n'ait plus qu'a renseigner son propre pseudo PSN -- voir
  // AUDIT.md section 0quater.
  if (!settings.pocketPsnApiKey.empty()) return settings.pocketPsnApiKey;
  return POCKETPSN_SHARED_API_KEY;
}

bool shouldUsePocketPsn(const AppSettings& settings) {
  return !settings.psnUsername.empty() && !effectiveApiKey(settings).empty();
}

}  // namespace ProviderFactory
