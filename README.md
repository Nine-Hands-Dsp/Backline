<p align="center">
  <picture>
    <source media="(prefers-color-scheme: dark)" srcset="assets/logo/NineHands_LogoPP_v003_FullWhite.svg">
    <img src="assets/logo/NineHands_LogoPP_v003_FullBlack.svg" alt="NINE HANDS | DSP" width="520">
  </picture>
</p>

<h1 align="center">• Backline •</h1>

<p align="center">
  Application Windows et macOS qui scanne le dossier de vos VST3, compare chaque plugin NINE HANDS | DSP<br>
  à sa dernière version publiée sur GitHub, et l'installe ou le met à jour, un par un ou tous d'un coup.
</p>

<p align="center">
  <a href="https://github.com/Nine-Hands-Dsp/Backline/releases/latest/download/Backline-Windows.zip"><img alt="Télécharger pour Windows" src="https://img.shields.io/badge/T%C3%A9l%C3%A9charger-Windows-0078D4?style=for-the-badge&logo=windows&logoColor=white"></a>
  <a href="https://github.com/Nine-Hands-Dsp/Backline/releases/latest/download/Backline-macOS.zip"><img alt="Télécharger pour macOS" src="https://img.shields.io/badge/T%C3%A9l%C3%A9charger-macOS-000000?style=for-the-badge&logo=apple&logoColor=white"></a>
  <br>
  <a href="https://github.com/Nine-Hands-Dsp/Backline/releases/latest"><img alt="Dernière version" src="https://img.shields.io/github/v/release/Nine-Hands-Dsp/Backline?style=flat-square&label=version&color=919496"></a>
</p>

---

## Utilisation

1. Au premier lancement, choisir le dossier des VST3 (proposé par défaut : `C:\Program Files\Common Files\VST3` sur Windows, `~/Library/Audio/Plug-Ins/VST3` sur macOS). Le bouton **Change folder…** permet d'en changer.
2. Chaque fiche indique la version installée et la dernière version publiée :
   - **up to date** (vert) : à jour ;
   - **v4.2.1 → v4.3.0** (orange, bandeau **UPDATE**) : mise à jour disponible ;
   - **Not installed** (vignette grisée) : pas encore installé.
3. Boutons de chaque fiche : flèche (installer en bleu, mettre à jour en orange), coche (installé), corbeille (désinstaller, vers la corbeille du système), **Notice** (PDF). Clic droit : réinstaller, afficher dans le dossier, page GitHub, favoris.
4. **Install all** installe tous les plugins manquants, **Update all** met à jour tous ceux qui ont une nouvelle version, d'après les filtres affichés (état : All / Installed / Updates / Not installed / Favorites / New ; type : Guitar / Studio / Instrument).

- **Windows** : `C:\Program Files\Common Files\VST3` demande les droits administrateur ; si le dossier choisi n'est pas accessible en écriture, l'application propose de se relancer en administrateur.
- **macOS** : application signée localement ; la ranger dans le dossier **Applications**, puis au premier lancement clic droit sur *Backline.app* > **Ouvrir**.
- **Mise à jour automatique** : à chaque démarrage, l'application consulte sa propre dernière version sur GitHub. S'il y en a une plus récente, elle la télécharge (après les installations en cours), se ferme et se rouvre toute seule sur la nouvelle version.
- Fermer le DAW avant de mettre à jour un plugin qu'il utilise.

## Onglet Session : partager un projet REAPER

L'onglet **Session** (à droite de **Plugins**, en haut) reprend l'application [Session](https://github.com/Nine-Hands-Dsp/Session) :

- **Export a project** : choisir un projet REAPER enregistré (`.rpp`, ou un projet récent) ; Session le range dans un fichier `.nhsession` avec ses fichiers audio, les captures NAM d'Ampire, les IR d'Impulse et les presets des plugins NINE HANDS | DSP utilisés.
- **Import a session** : ouvrir le `.nhsession` reçu, vérifier les plugins nécessaires (installés en vert, à mettre à jour en orange, absents en rouge : onglet Plugins pour les installer), choisir les dossiers du projet, des captures NAM et des IR, puis **Import session**. Le projet s'ouvre dans REAPER et Ampire / Impulse retrouvent les captures et IR installées.

## Fonctionnement

- **Plugins installés** : `Contents/Resources/moduleinfo.json` de chaque `.vst3` (nom, version, fabricant), sinon `Info.plist` (macOS) ou la version du fichier (Windows). Les plugins d'autres fabricants sont ignorés.
- **Dernières versions** : la Release `latest` de chaque dépôt `Nine-Hands-Dsp/<dépôt>`.
- **Téléchargements** : `releases/latest/download/<Plugin>-VST3-Windows.zip` / `-macOS.zip` de chaque dépôt.
- **Liste des plugins** : `catalog/plugins.json` et `catalog/img/<dépôt>.jpg`, copiés du dépôt Download. L'application lit la liste en ligne à chaque démarrage (nouveaux plugins visibles sans nouvelle version de l'application) et garde une copie intégrée.

## Mise à jour de la liste (à chaque nouvelle version d'un plugin ou nouveau plugin)

Après la mise à jour du dépôt Download :

```
python3 tools/sync_catalog.py <chemin du dépôt Download>
git add catalog && git commit -m "Liste des plugins" && git push
```

## Build

CMake + JUCE 8 (téléchargé automatiquement). Les builds Windows et macOS (universel) tournent sur GitHub Actions et publient la Release `v<version>` (version dans `CMakeLists.txt`).
