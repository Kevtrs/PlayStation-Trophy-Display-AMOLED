# Installation web (ESP Web Tools)

Cette page permet de flasher le firmware directement depuis Chrome/Edge
(API Web Serial), sans installer PlatformIO/Python -- voir AUDIT.md pour
le contexte complet de cette decision.

## Publier sur GitHub Pages

1. Poussez ce depot sur GitHub (public, sinon Pages ne fonctionne pas sur
   un compte gratuit).
2. Dans les parametres du depot GitHub : **Settings > Pages**, source =
   branche `main` (ou celle utilisee), dossier `/webinstall`
   -- ou deplacez ce dossier a la racine si votre configuration Pages
   l'exige (certains comptes n'acceptent que `/` ou `/docs`).
3. L'URL publiee ressemblera a
   `https://<utilisateur>.github.io/<depot>/webinstall/`.
4. Partagez cette URL (ex: dans la description MakerWorld).

## Regenerer les binaires apres une modification du firmware

Les fichiers dans `firmware/` sont des **copies figees** du dernier
build -- ils ne se mettent pas a jour tout seuls. Apres toute
modification du firmware :

```powershell
# Depuis la racine du depot, apres avoir compile :
pio run -e waveshare-amoled-175
.\webinstall\update_firmware.ps1
```

Puis incrementez `"version"` dans `manifest.json` et committez.

**Important (voir AUDIT.md section 0quater)** : le firmware compile
integre la cle API Pocket PSN partagee (`include/secrets.h`, jamais
committee) -- ne regenerez et ne publiez ces binaires que depuis une
machine ou ce fichier contient la vraie cle, sinon la version publiee
retombera en mode demo pour tout le monde.

## Pourquoi Chrome/Edge uniquement

Web Serial (l'API utilisee pour parler au port USB depuis le navigateur)
n'est disponible que sur les navigateurs bases sur Chromium, sur
ordinateur. Safari, Firefox et les navigateurs mobiles ne sont pas
supportes -- la page affiche un message clair et renvoie vers
l'installation classique (`INSTALLER_ET_LANCER.bat`) dans ce cas.
