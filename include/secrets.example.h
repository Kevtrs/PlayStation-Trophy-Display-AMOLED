#pragma once

// Modele committe dans Git (voir .gitignore : "include/secrets.h" est
// ignore). Copiez ce fichier en "include/secrets.h" et renseignez-y votre
// propre cle avant de compiler -- ne renseignez jamais la vraie cle ici.
//
// Cette cle authentifie l'APPLICATION aupres de l'API Pocket PSN, pas un
// utilisateur individuel (voir docs/POCKETPSN_PROTOCOL.md, section sur le
// modele du firmware d'origine) : une seule cle, obtenue une fois aupres du
// createur de Pocket PSN, peut donc etre compilee dans le firmware et
// partagee par tous les exemplaires distribues (ex: MakerWorld). Chaque
// utilisateur final n'a alors plus qu'a renseigner son propre pseudo PSN
// via le portail captif -- pas de cle a demander individuellement.
//
// Laisser vide (chaine vide) desactive ce mecanisme : il faudra alors que
// chaque utilisateur renseigne sa propre cle via le champ web (ancien
// comportement, toujours supporte en repli -- voir
// ProviderFactory::effectiveApiKey()).
#define POCKETPSN_SHARED_API_KEY ""
