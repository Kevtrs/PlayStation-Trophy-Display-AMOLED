"use strict";

const $ = (selector) => document.querySelector(selector);

// Traductions FR/EN (voir AUDIT.md -- demande utilisateur du 2026-07-28 :
// version anglaise du portail captif). currentLang suit config.language,
// deja persiste cote firmware (voir ConfigManager/AppSettings) -- reutilise
// le meme reglage plutot que d'en ajouter un nouveau. Repli sur fr si une
// cle manque dans la langue courante.
const translations = {
  fr: {
    pageTitle: "Trophy Display - Configuration",
    heading: "Configuration locale",
    connLoading: "Chargement",
    connOk: "Connecte",
    connCheck: "A verifier",
    connSetup: "A configurer",
    connApiError: "Erreur API",
    deviceStateKicker: "Etat appareil",
    setupTitleLoading: "Lecture de l'etat",
    setupSubtitleLoading: "Connexion a l'interface locale...",
    setupTitleRequired: "Configuration requise",
    setupSubtitleRequired: "Renseignez le Wi-Fi et le pseudo PSN pour activer la synchronisation.",
    setupTitleSyncing: "Synchronisation en cours",
    setupSubtitleSyncing: "L'appareil recupere les donnees et mettra l'affichage a jour automatiquement.",
    setupTitleCached: "Donnees en cache",
    setupSubtitleCached: "Les dernieres donnees valides restent visibles jusqu'au prochain acces Pocket PSN.",
    setupTitleActionNeeded: "Action requise",
    setupTitleReady: "Pret",
    setupSubtitleReady: "Le Trophy Display est configure et pret a synchroniser.",
    setupTitleUnavailable: "Interface indisponible",
    setupSubtitleUnavailable: "Impossible de lire l'API locale du Trophy Display.",
    networkLabel: "Reseau",
    lastSyncLabel: "Derniere sync",
    lastSyncNever: "Jamais",
    lastSyncInProgress: "En cours",
    networkNotConnected: "Non connecte",
    unknownIp: "IP inconnue",
    mockKicker: "Mock local",
    mockHeading: "Scenarios de test",
    wifiKicker: "Wi-Fi",
    wifiHeading: "Connexion reseau",
    scanButton: "Scanner",
    ssidLabel: "Reseau detecte",
    ssidNone: "Aucun reseau detecte",
    ssidSecure: "securise",
    ssidOpen: "ouvert",
    ssidConfigured: "configure",
    ssidManualOption: "Saisir le nom manuellement...",
    ssidManualLabel: "Nom du reseau (SSID)",
    ssidManualPlaceholder: "Nom exact de votre reseau Wi-Fi",
    passwordLabel: "Mot de passe",
    passwordPlaceholder: "Mot de passe Wi-Fi",
    wifiStatusIdle: "Aucun scan lance.",
    wifiScanning: "Scan en cours...",
    wifiScanResult: (n) => `${n} reseaux detectes.`,
    profileKicker: "Profil",
    profileHeading: "Compte PSN",
    psnUsernameLabel: "Pseudo PSN",
    keyStatusUnknown: "Statut de la cle inconnu.",
    keyStatusReady: "Cle API Pocket PSN prete.",
    keyStatusMissing: "Aucune cle API disponible -- mode demo actif.",
    pocketPsnCredit: "Donnees PlayStation fournies par",
    saveButton: "Enregistrer",
    saveStatusReady: "Pret.",
    saveStatusSaving: "Enregistrement...",
    saveStatusSaved: "Configuration enregistree.",
    advancedSettings: "Reglages avances",
    displayKicker: "Affichage",
    displayHeading: "Reglages ecran",
    brightnessLabel: "Luminosite",
    sleepTitle: "Veille",
    sleepDesc: "Reduit la luminosite apres inactivite.",
    rotationTitle: "Rotation automatique",
    rotationDesc: "Alterner les ecrans principaux.",
    animationsTitle: "Animations",
    animationsDesc: "Transitions et micro-interactions LVGL.",
    rotationDelayLabel: "Delai rotation",
    sleepDelayLabel: "Delai veille",
    syncKicker: "Synchronisation",
    syncHeading: "Donnees trophées",
    syncNowButton: "Synchroniser",
    syncLaunched: "Synchronisation lancee.",
    languageLabel: "Langue",
    syncIntervalLabel: "Intervalle de synchro",
    diagMaintenance: "Diagnostics et maintenance",
    diagKicker: "Diagnostics",
    diagHeading: "Etat systeme",
    diagRefresh: "Actualiser",
    maintenanceKicker: "Maintenance",
    maintenanceHeading: "Actions appareil",
    rebootButton: "Redemarrer",
    rebootRequested: "Redemarrage demande.",
    resetButton: "Reinitialiser",
    resetModalTitle: "Confirmer la reinitialisation",
    resetModalDesc: "Cette action efface la configuration locale. Tapez RESET pour confirmer.",
    resetCancel: "Annuler",
    resetConfirm: "Confirmer",
    resetTypeError: "Tapez RESET pour confirmer.",
    resetDone: "Configuration effacee.",
    httpError: (status) => `Erreur HTTP ${status}`,
    diagFirmwareVersion: "Version firmware",
    diagUptime: "Disponibilite",
    diagFreeHeap: "Memoire libre (heap)",
    diagMinFreeHeap: "Memoire libre minimale",
    diagPsramTotal: "PSRAM totale",
    diagPsramFree: "PSRAM libre",
    diagFlashSize: "Taille Flash",
    diagAppUsed: "Flash utilisee (appli)",
    diagFsTotal: "LittleFS totale",
    diagFsUsed: "LittleFS utilisee",
    diagWifiState: "Etat Wi-Fi",
    diagSsid: "SSID",
    diagIp: "Adresse IP",
    diagRssi: "Wi-Fi RSSI",
    diagCacheAvailable: "Cache disponible",
    diagCacheAge: "Age du cache",
    diagLastSyncState: "Etat derniere synchro",
    diagLastSync: "Derniere synchro",
    diagLastHttpStatus: "Dernier code HTTP",
    diagLastErrorCode: "Dernier code d'erreur",
    diagLastErrorNone: "Aucune",
    diagSyncSuccess: "Synchronisations reussies",
    diagSyncFailure: "Synchronisations en echec",
    boolYes: "Oui",
    boolNo: "Non",
    scenarioUnconfigured: "Non configure",
    scenarioConnected: "Wi-Fi connecte",
    scenarioBadPassword: "Mauvais mot de passe",
    scenarioProfileNotFound: "Profil introuvable",
    scenarioSyncProgress: "Sync en cours",
    scenarioPocketDown: "Pocket PSN indisponible",
    scenarioCached: "Donnees en cache",
    scenarioInternal: "Erreur interne"
  },
  en: {
    pageTitle: "Trophy Display - Setup",
    heading: "Local setup",
    connLoading: "Loading",
    connOk: "Connected",
    connCheck: "Check needed",
    connSetup: "Needs setup",
    connApiError: "API error",
    deviceStateKicker: "Device status",
    setupTitleLoading: "Reading status",
    setupSubtitleLoading: "Connecting to the local interface...",
    setupTitleRequired: "Setup required",
    setupSubtitleRequired: "Fill in your Wi-Fi and PSN username to enable syncing.",
    setupTitleSyncing: "Syncing",
    setupSubtitleSyncing: "The device is fetching data and will update the display automatically.",
    setupTitleCached: "Cached data",
    setupSubtitleCached: "The last valid data stays on screen until the next successful Pocket PSN sync.",
    setupTitleActionNeeded: "Action needed",
    setupTitleReady: "Ready",
    setupSubtitleReady: "Trophy Display is set up and ready to sync.",
    setupTitleUnavailable: "Interface unavailable",
    setupSubtitleUnavailable: "Could not reach the Trophy Display's local API.",
    networkLabel: "Network",
    lastSyncLabel: "Last sync",
    lastSyncNever: "Never",
    lastSyncInProgress: "In progress",
    networkNotConnected: "Not connected",
    unknownIp: "unknown IP",
    mockKicker: "Local mock",
    mockHeading: "Test scenarios",
    wifiKicker: "Wi-Fi",
    wifiHeading: "Network connection",
    scanButton: "Scan",
    ssidLabel: "Detected network",
    ssidNone: "No network detected",
    ssidSecure: "secured",
    ssidOpen: "open",
    ssidConfigured: "configured",
    ssidManualOption: "Type network name manually...",
    ssidManualLabel: "Network name (SSID)",
    ssidManualPlaceholder: "Exact name of your Wi-Fi network",
    passwordLabel: "Password",
    passwordPlaceholder: "Wi-Fi password",
    wifiStatusIdle: "No scan yet.",
    wifiScanning: "Scanning...",
    wifiScanResult: (n) => `${n} networks found.`,
    profileKicker: "Profile",
    profileHeading: "PSN account",
    psnUsernameLabel: "PSN username",
    keyStatusUnknown: "Key status unknown.",
    keyStatusReady: "Pocket PSN API key ready.",
    keyStatusMissing: "No API key available -- demo mode active.",
    pocketPsnCredit: "PlayStation data provided by",
    saveButton: "Save",
    saveStatusReady: "Ready.",
    saveStatusSaving: "Saving...",
    saveStatusSaved: "Configuration saved.",
    advancedSettings: "Advanced settings",
    displayKicker: "Display",
    displayHeading: "Screen settings",
    brightnessLabel: "Brightness",
    sleepTitle: "Sleep",
    sleepDesc: "Dims the screen after inactivity.",
    rotationTitle: "Auto-rotation",
    rotationDesc: "Cycle through the main screens.",
    animationsTitle: "Animations",
    animationsDesc: "LVGL transitions and micro-interactions.",
    rotationDelayLabel: "Rotation delay",
    sleepDelayLabel: "Sleep delay",
    syncKicker: "Sync",
    syncHeading: "Trophy data",
    syncNowButton: "Sync now",
    syncLaunched: "Sync started.",
    languageLabel: "Language",
    syncIntervalLabel: "Sync interval",
    diagMaintenance: "Diagnostics & maintenance",
    diagKicker: "Diagnostics",
    diagHeading: "System status",
    diagRefresh: "Refresh",
    maintenanceKicker: "Maintenance",
    maintenanceHeading: "Device actions",
    rebootButton: "Restart",
    rebootRequested: "Restart requested.",
    resetButton: "Factory reset",
    resetModalTitle: "Confirm factory reset",
    resetModalDesc: "This clears the local configuration. Type RESET to confirm.",
    resetCancel: "Cancel",
    resetConfirm: "Confirm",
    resetTypeError: "Type RESET to confirm.",
    resetDone: "Configuration cleared.",
    httpError: (status) => `HTTP error ${status}`,
    diagFirmwareVersion: "Firmware version",
    diagUptime: "Uptime",
    diagFreeHeap: "Free heap memory",
    diagMinFreeHeap: "Minimum free heap",
    diagPsramTotal: "Total PSRAM",
    diagPsramFree: "Free PSRAM",
    diagFlashSize: "Flash size",
    diagAppUsed: "Flash used (app)",
    diagFsTotal: "LittleFS total",
    diagFsUsed: "LittleFS used",
    diagWifiState: "Wi-Fi state",
    diagSsid: "SSID",
    diagIp: "IP address",
    diagRssi: "Wi-Fi RSSI",
    diagCacheAvailable: "Cache available",
    diagCacheAge: "Cache age",
    diagLastSyncState: "Last sync state",
    diagLastSync: "Last sync",
    diagLastHttpStatus: "Last HTTP status",
    diagLastErrorCode: "Last error code",
    diagLastErrorNone: "None",
    diagSyncSuccess: "Successful syncs",
    diagSyncFailure: "Failed syncs",
    boolYes: "Yes",
    boolNo: "No",
    scenarioUnconfigured: "Not configured",
    scenarioConnected: "Wi-Fi connected",
    scenarioBadPassword: "Wrong password",
    scenarioProfileNotFound: "Profile not found",
    scenarioSyncProgress: "Syncing",
    scenarioPocketDown: "Pocket PSN down",
    scenarioCached: "Cached data",
    scenarioInternal: "Internal error"
  }
};

let currentLang = "fr";

function t(key, ...args) {
  const dict = translations[currentLang] || translations.fr;
  const entry = dict[key] ?? translations.fr[key] ?? key;
  return typeof entry === "function" ? entry(...args) : entry;
}

function applyStaticTranslations() {
  document.documentElement.lang = currentLang;
  document.title = t("pageTitle");
  document.querySelectorAll("[data-i18n]").forEach((el) => {
    el.textContent = t(el.getAttribute("data-i18n"));
  });
  document.querySelectorAll("[data-i18n-placeholder]").forEach((el) => {
    el.placeholder = t(el.getAttribute("data-i18n-placeholder"));
  });
}

function setLanguage(lang, persistLocally = true) {
  currentLang = translations[lang] ? lang : "fr";
  applyStaticTranslations();
  document.querySelectorAll(".lang-btn").forEach((btn) => {
    btn.classList.toggle("is-active", btn.dataset.lang === currentLang);
  });
  renderMockScenarios();
  // Memorise le choix cote navigateur (localStorage) : visible/utilisable
  // immediatement au prochain chargement de la page, sans attendre que la
  // configuration de l'appareil (qui peut echouer/prendre du temps) soit
  // chargee -- essentiel pour quelqu'un qui ne lit pas le francais et doit
  // pouvoir changer de langue des l'arrivee sur la page.
  if (persistLocally) {
    try {
      window.localStorage.setItem("trophyDisplayLang", currentLang);
    } catch {
      // Stockage indisponible (navigation privee, etc.) -- sans consequence,
      // simplement pas de memorisation entre deux visites.
    }
  }
}

const els = {
  connectionPill: $("#connectionPill"),
  connectionText: $("#connectionText"),
  setupTitle: $("#setupTitle"),
  setupSubtitle: $("#setupSubtitle"),
  networkState: $("#networkState"),
  lastSync: $("#lastSync"),
  mockPanel: $("#mockPanel"),
  scenarioList: $("#scenarioList"),
  form: $("#configForm"),
  scanButton: $("#scanButton"),
  ssidSelect: $("#ssidSelect"),
  ssidManualField: $("#ssidManualField"),
  ssidManual: $("#ssidManual"),
  wifiPassword: $("#wifiPassword"),
  wifiStatus: $("#wifiStatus"),
  psnUsername: $("#psnUsername"),
  pocketPsnKeyStatus: $("#pocketPsnKeyStatus"),
  brightness: $("#brightness"),
  brightnessValue: $("#brightnessValue"),
  sleepEnabled: $("#sleepEnabled"),
  autoRotation: $("#autoRotation"),
  animations: $("#animations"),
  rotationDelay: $("#rotationDelay"),
  sleepDelay: $("#sleepDelay"),
  language: $("#language"),
  syncInterval: $("#syncInterval"),
  syncNowButton: $("#syncNowButton"),
  saveButton: $("#saveButton"),
  saveStatus: $("#saveStatus"),
  diagnosticsGrid: $("#diagnosticsGrid"),
  diagnosticsRaw: $("#diagnosticsRaw"),
  refreshDiagnosticsButton: $("#refreshDiagnosticsButton"),
  rebootButton: $("#rebootButton"),
  resetButton: $("#resetButton"),
  resetModal: $("#resetModal"),
  resetConfirmInput: $("#resetConfirmInput"),
  cancelResetButton: $("#cancelResetButton"),
  confirmResetButton: $("#confirmResetButton"),
  toastStack: $("#toastStack")
};

const state = {
  status: null,
  config: null,
  networks: [],
  loading: new Set()
};

const scenarios = [
  ["unconfigured", "scenarioUnconfigured"],
  ["connected", "scenarioConnected"],
  ["bad_password", "scenarioBadPassword"],
  ["profile_not_found", "scenarioProfileNotFound"],
  ["sync_progress", "scenarioSyncProgress"],
  ["pocket_down", "scenarioPocketDown"],
  ["cached", "scenarioCached"],
  ["internal", "scenarioInternal"]
];

function setBusy(key, busy) {
  if (busy) state.loading.add(key);
  else state.loading.delete(key);

  const any = state.loading.size > 0;
  els.saveButton.disabled = any;
  els.scanButton.disabled = state.loading.has("scan");
  els.syncNowButton.disabled = state.loading.has("sync");
  els.refreshDiagnosticsButton.disabled = state.loading.has("diagnostics");
  els.rebootButton.disabled = state.loading.has("reboot");
  els.confirmResetButton.disabled = state.loading.has("reset");
}

async function api(path, options = {}) {
  const response = await fetch(path, {
    ...options,
    headers: {
      "Accept": "application/json",
      ...(options.body ? { "Content-Type": "application/json" } : {}),
      ...(options.headers || {})
    }
  });

  let payload = null;
  const text = await response.text();
  if (text) {
    try {
      payload = JSON.parse(text);
    } catch {
      payload = { message: text };
    }
  }

  if (!response.ok) {
    const message = payload?.message || payload?.error || t("httpError", response.status);
    const error = new Error(message);
    error.status = response.status;
    error.payload = payload;
    throw error;
  }

  return payload || {};
}

function showToast(message, type = "info", durationMs = 4200) {
  const toast = document.createElement("div");
  toast.className = `toast${type === "error" ? " is-error" : ""}`;
  toast.textContent = message;
  els.toastStack.appendChild(toast);
  window.setTimeout(() => {
    toast.remove();
  }, durationMs);
}

function formatLastSync(value) {
  if (!value) return t("lastSyncNever");
  if (value === "syncing") return t("lastSyncInProgress");
  return value;
}

function renderStatus(status) {
  state.status = status;

  const online = status.network?.connected && !status.offline;
  els.connectionPill.classList.toggle("is-ok", Boolean(online));
  els.connectionPill.classList.toggle("is-bad", Boolean(status.error));
  els.connectionText.textContent = online ? t("connOk") : status.configured ? t("connCheck") : t("connSetup");

  els.networkState.textContent = online
    ? `${status.network.ssid || "Wi-Fi"} - ${status.network.ip || t("unknownIp")}`
    : status.network?.message || t("networkNotConnected");
  els.lastSync.textContent = formatLastSync(status.sync?.lastSync);

  if (!status.configured) {
    els.setupTitle.textContent = t("setupTitleRequired");
    els.setupSubtitle.textContent = t("setupSubtitleRequired");
    return;
  }

  if (status.sync?.state === "syncing") {
    els.setupTitle.textContent = t("setupTitleSyncing");
    els.setupSubtitle.textContent = t("setupSubtitleSyncing");
    return;
  }

  if (status.sync?.source === "cache") {
    els.setupTitle.textContent = t("setupTitleCached");
    els.setupSubtitle.textContent = t("setupSubtitleCached");
    return;
  }

  if (status.error) {
    els.setupTitle.textContent = t("setupTitleActionNeeded");
    els.setupSubtitle.textContent = status.error;
    return;
  }

  els.setupTitle.textContent = t("setupTitleReady");
  els.setupSubtitle.textContent = t("setupSubtitleReady");
}

function renderConfig(config) {
  state.config = config;
  // N'impose la langue enregistree sur l'appareil que si l'utilisateur n'a
  // pas deja choisi explicitement une langue d'affichage locale (bouton
  // FR/EN, voir setLanguage()) -- sinon un simple rechargement de la page
  // ou un rafraichissement du statut annulerait silencieusement son choix.
  let storedLang = null;
  try {
    storedLang = window.localStorage.getItem("trophyDisplayLang");
  } catch {
    // Stockage indisponible -- pas de preference locale a respecter.
  }
  if (!storedLang) {
    setLanguage(config.language || "fr", false);
  }
  els.psnUsername.value = config.psnUsername || "";
  // La cle API Pocket PSN n'est plus saisie par l'utilisateur final : elle
  // est compilee dans le firmware (voir ProviderFactory::effectiveApiKey()
  // et AUDIT.md section 0quater) -- seul un statut informatif est affiche.
  els.pocketPsnKeyStatus.textContent = config.pocketPsnKeyConfigured
    ? t("keyStatusReady")
    : t("keyStatusMissing");
  els.brightness.value = config.brightness ?? 82;
  els.brightnessValue.textContent = `${els.brightness.value}%`;
  els.sleepEnabled.checked = Boolean(config.sleepEnabled);
  els.autoRotation.checked = Boolean(config.autoRotation);
  els.animations.checked = config.animations !== false;
  els.rotationDelay.value = String(config.rotationDelay ?? 20);
  els.sleepDelay.value = String(config.sleepDelay ?? 5);
  els.language.value = config.language || "fr";
  els.syncInterval.value = String(config.syncInterval ?? 60);

  if (config.ssid) {
    ensureNetworkOption(config.ssid, t("ssidConfigured"));
    els.ssidSelect.value = config.ssid;
  }
}

function ensureNetworkOption(ssid, suffix = "") {
  if (!ssid) return;
  const existing = Array.from(els.ssidSelect.options).find((option) => option.value === ssid);
  if (existing) return;
  const option = document.createElement("option");
  option.value = ssid;
  option.textContent = suffix ? `${ssid} (${suffix})` : ssid;
  els.ssidSelect.appendChild(option);
}

const MANUAL_SSID_VALUE = "__manual__";

function appendManualSsidOption() {
  const option = document.createElement("option");
  option.value = MANUAL_SSID_VALUE;
  option.textContent = t("ssidManualOption");
  els.ssidSelect.appendChild(option);
}

function updateManualSsidVisibility() {
  const manual = els.ssidSelect.value === MANUAL_SSID_VALUE;
  els.ssidManualField.hidden = !manual;
  if (manual) els.ssidManual.focus();
}

// Le scan Wi-Fi peut echouer ou ne rien trouver (reseau non diffuse, scan
// perturbe -- voir DEPANNAGE_MATERIEL.md) : sans option de saisie manuelle,
// l'utilisateur restait totalement bloque, aucun champ texte n'existait
// nulle part ailleurs sur cette page pour taper un SSID a la main. Retour
// utilisateur reel du 2026-07-29 ("quand je scan... 0 reseaux"). L'option
// manuelle reste toujours proposee, meme quand le scan reussit (reseau
// masque/SSID non diffuse -- meme raisonnement).
function renderNetworks(networks) {
  state.networks = networks;
  els.ssidSelect.innerHTML = "";
  if (!networks.length) {
    const option = document.createElement("option");
    option.value = "";
    option.textContent = t("ssidNone");
    els.ssidSelect.appendChild(option);
  } else {
    for (const network of networks) {
      const option = document.createElement("option");
      option.value = network.ssid;
      const security = network.secure ? t("ssidSecure") : t("ssidOpen");
      option.textContent = `${network.ssid} - ${network.rssi} dBm - ${security}`;
      els.ssidSelect.appendChild(option);
    }
  }

  appendManualSsidOption();

  if (state.config?.ssid) {
    ensureNetworkOption(state.config.ssid, t("ssidConfigured"));
    els.ssidSelect.value = state.config.ssid;
  }
  updateManualSsidVisibility();
}

function readConfigForm() {
  const config = {
    ssid: els.ssidSelect.value === MANUAL_SSID_VALUE ? els.ssidManual.value.trim() : els.ssidSelect.value,
    psnUsername: els.psnUsername.value.trim(),
    brightness: Number(els.brightness.value),
    sleepEnabled: els.sleepEnabled.checked,
    sleepDelay: Number(els.sleepDelay.value),
    autoRotation: els.autoRotation.checked,
    rotationDelay: Number(els.rotationDelay.value),
    animations: els.animations.checked,
    language: els.language.value,
    syncInterval: Number(els.syncInterval.value)
  };
  // Le mot de passe Wi-Fi vit dans le meme formulaire que le reste (voir
  // index.html) : l'inclure ici permet d'enregistrer reseau + mot de passe
  // + pseudo PSN en un seul "Enregistrer", sans passer par le bouton
  // "Connecter" separe (qui coupe la page en cours de saisie -- voir
  // WiFiManager.cpp). Champ ecriture seule : n'inclure que si l'utilisateur
  // a saisi une valeur, sinon le mot de passe deja enregistre est conserve.
  const wifiPassword = els.wifiPassword.value;
  if (wifiPassword) {
    config.password = wifiPassword;
  }
  return config;
}

function formatBytes(value) {
  if (value === null || value === undefined) return "-";
  if (value < 1024) return `${value} o`;
  if (value < 1024 * 1024) return `${(value / 1024).toFixed(1)} Ko`;
  return `${(value / (1024 * 1024)).toFixed(1)} Mo`;
}

function formatDuration(seconds) {
  if (seconds === null || seconds === undefined) return "-";
  const s = Math.max(0, Math.floor(seconds));
  const h = Math.floor(s / 3600);
  const m = Math.floor((s % 3600) / 60);
  const rem = s % 60;
  if (h > 0) return `${h} h ${m} min`;
  if (m > 0) return `${m} min ${rem} s`;
  return `${rem} s`;
}

function formatTimestamp(value) {
  if (!value) return t("lastSyncNever");
  return new Date(value * 1000).toLocaleTimeString();
}

function formatBool(value) {
  return value ? t("boolYes") : t("boolNo");
}

// Champs exacts renvoyes par GET /api/diagnostics (voir
// src/web/DiagnosticsSnapshot.h) : ne pas renommer sans mettre a jour ce
// fichier ET le firmware en meme temps (voir docs/WEB_UI_GAP_ANALYSIS.md).
function renderDiagnostics(diagnostics) {
  const rows = [
    [t("diagFirmwareVersion"), diagnostics.firmwareVersion],
    [t("diagUptime"), formatDuration(diagnostics.uptimeSeconds)],
    [t("diagFreeHeap"), formatBytes(diagnostics.freeHeapBytes)],
    [t("diagMinFreeHeap"), formatBytes(diagnostics.minFreeHeapBytes)],
    [t("diagPsramTotal"), formatBytes(diagnostics.psramTotalBytes)],
    [t("diagPsramFree"), formatBytes(diagnostics.psramFreeBytes)],
    [t("diagFlashSize"), formatBytes(diagnostics.flashSizeBytes)],
    [t("diagAppUsed"), formatBytes(diagnostics.appUsedBytes)],
    [t("diagFsTotal"), formatBytes(diagnostics.littleFsTotalBytes)],
    [t("diagFsUsed"), formatBytes(diagnostics.littleFsUsedBytes)],
    [t("diagWifiState"), diagnostics.wifiState ?? "-"],
    [t("diagSsid"), diagnostics.ssid || "-"],
    [t("diagIp"), diagnostics.ipAddress || "-"],
    [t("diagRssi"), diagnostics.rssi || diagnostics.rssi === 0 ? `${diagnostics.rssi} dBm` : "-"],
    [t("diagCacheAvailable"), formatBool(diagnostics.cacheAvailable)],
    [t("diagCacheAge"), formatDuration(diagnostics.cacheAgeSeconds)],
    [t("diagLastSyncState"), diagnostics.lastSyncState ?? "-"],
    [t("diagLastSync"), formatTimestamp(diagnostics.lastSyncTimestamp)],
    [t("diagLastHttpStatus"), diagnostics.lastHttpStatus ?? "-"],
    [t("diagLastErrorCode"), diagnostics.lastErrorCode || t("diagLastErrorNone")],
    [t("diagSyncSuccess"), diagnostics.syncSuccessCount ?? "-"],
    [t("diagSyncFailure"), diagnostics.syncFailureCount ?? "-"]
  ];

  els.diagnosticsGrid.innerHTML = rows.map(([key, value]) => `
    <div>
      <dt>${escapeHtml(key)}</dt>
      <dd>${escapeHtml(String(value ?? "-"))}</dd>
    </div>
  `).join("");
  els.diagnosticsRaw.textContent = JSON.stringify(diagnostics, null, 2);
}

function escapeHtml(value) {
  return value.replace(/[&<>"']/g, (char) => ({
    "&": "&amp;",
    "<": "&lt;",
    ">": "&gt;",
    "\"": "&quot;",
    "'": "&#039;"
  }[char]));
}

async function loadStatus() {
  const status = await api("/api/status");
  renderStatus(status);
}

async function loadConfig() {
  const config = await api("/api/config");
  renderConfig(config);
}

async function scanWifi() {
  setBusy("scan", true);
  els.wifiStatus.textContent = t("wifiScanning");
  try {
    const result = await api("/api/wifi/scan");
    renderNetworks(result.networks || []);
    els.wifiStatus.textContent = t("wifiScanResult", (result.networks || []).length);
  } catch (error) {
    els.wifiStatus.textContent = error.message;
    showToast(error.message, "error");
  } finally {
    setBusy("scan", false);
  }
}

async function saveConfig(event) {
  event.preventDefault();
  setBusy("save", true);
  els.saveStatus.textContent = t("saveStatusSaving");
  try {
    const config = readConfigForm();
    const result = await api("/api/config", {
      method: "POST",
      body: JSON.stringify(config)
    });
    renderConfig(result.config || config);
    const restarting = Boolean(result.message && result.message.includes("redemarrer"));
    els.saveStatus.textContent = result.message || t("saveStatusSaved");
    if (restarting) {
      // Redemarrage programme cote firmware (voir
      // CaptivePortalServer::handleConfigPost()) : message garde visible
      // plus longtemps qu'un toast normal, l'appareil devient injoignable
      // quelques instants. Pas de loadStatus() ici : l'appareil redemarre.
      showToast(result.message, "info", 10000);
    } else {
      showToast(t("saveStatusSaved"));
      await loadStatus();
    }
  } catch (error) {
    els.saveStatus.textContent = error.message;
    showToast(error.message, "error");
  } finally {
    setBusy("save", false);
  }
}

async function syncNow() {
  setBusy("sync", true);
  try {
    const result = await api("/api/sync", { method: "POST" });
    showToast(result.message || t("syncLaunched"));
    await loadStatus();
  } catch (error) {
    showToast(error.message, "error");
    await loadStatus().catch(() => {});
  } finally {
    setBusy("sync", false);
  }
}

async function refreshDiagnostics() {
  setBusy("diagnostics", true);
  try {
    const diagnostics = await api("/api/diagnostics");
    renderDiagnostics(diagnostics);
  } catch (error) {
    els.diagnosticsRaw.textContent = error.message;
    showToast(error.message, "error");
  } finally {
    setBusy("diagnostics", false);
  }
}

async function reboot() {
  setBusy("reboot", true);
  try {
    const result = await api("/api/reboot", { method: "POST" });
    showToast(result.message || t("rebootRequested"));
  } catch (error) {
    showToast(error.message, "error");
  } finally {
    setBusy("reboot", false);
  }
}

function openResetModal() {
  els.resetConfirmInput.value = "";
  els.resetModal.classList.remove("hidden");
  window.setTimeout(() => els.resetConfirmInput.focus(), 30);
}

function closeResetModal() {
  els.resetModal.classList.add("hidden");
}

async function resetDevice() {
  // "RESET" reste le mot-cle de confirmation dans les deux langues
  // (deja un mot anglais, universellement reconnaissable) -- seul le
  // texte d'instruction autour est traduit.
  if (els.resetConfirmInput.value.trim().toUpperCase() !== "RESET") {
    showToast(t("resetTypeError"), "error");
    return;
  }

  setBusy("reset", true);
  try {
    const result = await api("/api/reset", {
      method: "POST",
      body: JSON.stringify({ confirm: true })
    });
    closeResetModal();
    showToast(result.message || t("resetDone"));
    await Promise.all([loadStatus(), loadConfig(), refreshDiagnostics()]);
  } catch (error) {
    showToast(error.message, "error");
  } finally {
    setBusy("reset", false);
  }
}

function renderMockScenarios() {
  const local = ["localhost", "127.0.0.1", "::1"].includes(location.hostname);
  if (!local) return;

  els.mockPanel.classList.remove("hidden");
  const current = new URLSearchParams(location.search).get("scenario") || "connected";
  els.scenarioList.innerHTML = scenarios.map(([id, labelKey]) => {
    const selected = id === current ? " aria-current=\"true\"" : "";
    return `<a href="/?scenario=${encodeURIComponent(id)}"${selected}>${escapeHtml(t(labelKey))}</a>`;
  }).join("");
}

function wireEvents() {
  els.scanButton.addEventListener("click", scanWifi);
  els.ssidSelect.addEventListener("change", updateManualSsidVisibility);
  els.form.addEventListener("submit", saveConfig);
  els.syncNowButton.addEventListener("click", syncNow);
  els.refreshDiagnosticsButton.addEventListener("click", refreshDiagnostics);
  els.rebootButton.addEventListener("click", reboot);
  els.resetButton.addEventListener("click", openResetModal);
  els.cancelResetButton.addEventListener("click", closeResetModal);
  els.confirmResetButton.addEventListener("click", resetDevice);
  els.resetModal.addEventListener("click", (event) => {
    if (event.target === els.resetModal) closeResetModal();
  });
  els.brightness.addEventListener("input", () => {
    els.brightnessValue.textContent = `${els.brightness.value}%`;
  });
  els.language.addEventListener("change", () => setLanguage(els.language.value));
  document.querySelectorAll(".lang-btn").forEach((btn) => {
    btn.addEventListener("click", () => {
      setLanguage(btn.dataset.lang);
      // Garde le menu deroulant "Langue" (Reglages avances) synchronise :
      // si l'utilisateur clique ensuite sur "Enregistrer", le meme choix
      // est persiste cote appareil, pas seulement affiche localement.
      els.language.value = btn.dataset.lang;
    });
  });
}

async function boot() {
  // Applique une preference de langue locale (bouton FR/EN memorise dans
  // ce navigateur) avant meme de contacter l'appareil : essentiel pour
  // quelqu'un qui ne lit pas le francais et doit pouvoir lire la page des
  // le premier chargement, meme si l'appareil met du temps a repondre ou
  // ne repond pas encore (voir aussi renderConfig()).
  let storedLang = null;
  try {
    storedLang = window.localStorage.getItem("trophyDisplayLang");
  } catch {
    // Stockage indisponible -- reste sur le francais par defaut.
  }
  setLanguage(storedLang || "fr", false);
  wireEvents();
  renderMockScenarios();
  try {
    await Promise.all([loadStatus(), loadConfig(), refreshDiagnostics()]);
    await scanWifi();
  } catch (error) {
    els.setupTitle.textContent = t("setupTitleUnavailable");
    els.setupSubtitle.textContent = t("setupSubtitleUnavailable");
    els.connectionPill.classList.add("is-bad");
    els.connectionText.textContent = t("connApiError");
    showToast(error.message, "error");
  }
}

document.addEventListener("DOMContentLoaded", boot);
