#include "ui/demo_data.hpp"

namespace trophy {

ProfileData standard_profile() {
    ProfileData p;
    p.username = "Kevin_Trophies";
    p.level = 327;
    p.progress = 72;
    p.total = 4286;
    p.platinum = 58;
    p.gold = 214;
    p.silver = 876;
    p.bronze = 3138;
    p.games_completed = 142;
    p.completion = 78;
    p.world_rank = "#12 483";
    p.play_time = "3 426 h";
    p.updated = "il y a 8 min";
    p.offline_message = "Dernières données conservées. Synchronisation relancée dès que le réseau revient.";
    p.error_title = "Synchro impossible";
    p.error_message = "Profil conservé en cache. Vérifiez la connexion ou relancez plus tard.";
    return p;
}

ProfileData small_profile() {
    ProfileData p;
    p.username = "MiniHunter";
    p.level = 12;
    p.progress = 31;
    p.total = 77;
    p.platinum = 0;
    p.gold = 4;
    p.silver = 18;
    p.bronze = 55;
    p.games_completed = 3;
    p.completion = 44;
    p.world_rank = "#928 114";
    p.play_time = "46 h";
    p.updated = "hier";
    p.offline_message = "Données enregistrées. Nouvelle tentative quand le réseau revient.";
    p.error_message = "Profil introuvable ou données indisponibles. Cache conservé.";
    return p;
}

ProfileData huge_profile() {
    ProfileData p;
    p.username = "Legend_999";
    p.level = 999;
    p.progress = 98;
    p.total = 137240;
    p.platinum = 1248;
    p.gold = 8642;
    p.silver = 24510;
    p.bronze = 102840;
    p.games_completed = 2048;
    p.completion = 100;
    p.world_rank = "#42";
    p.play_time = "18 742 h";
    p.updated = "maintenant";
    p.offline_message = "Données massives conservées localement. Synchronisation différée.";
    p.error_message = "Traitement interrompu. Les dernières statistiques restent affichées.";
    return p;
}

ProfileData long_name_profile() {
    ProfileData p = standard_profile();
    p.username = "TheUltimatePlayStationCollector";
    return p;
}

} // namespace trophy
