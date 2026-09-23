import QRCode from "qrcode";
import "./styles.css";
import { decryptProfile, encryptProfile, parseWgConfig } from "./vault.js";

const STORAGE_KEY = "dp-connect.profiles.v1";
const AUTO_LOCK_MS = 5 * 60 * 1000;

let profiles = loadProfiles();
let selectedId = profiles[0]?.id || null;
let unlockedConfig = "";
let lockTimer = 0;
let deferredInstallPrompt = null;

const app = document.querySelector("#app");
app.innerHTML = `
  <header class="topbar">
    <a class="brand" href="./" aria-label="DP Connect">
      <img src="./icons/icon.svg" alt="" width="42" height="42" />
      <span><strong>DP Connect</strong><small>PWA client</small></span>
    </a>
    <div class="topbar-actions">
      <span id="networkBadge" class="badge"></span>
      <button id="installButton" class="button secondary hidden" type="button">Установить</button>
    </div>
  </header>

  <main>
    <section class="hero">
      <div>
        <span class="eyebrow">DP WG · зашифровано на устройстве</span>
        <h1>Профили всегда под рукой</h1>
        <p>Импортируйте DP WG конфигурацию, откройте QR-код или передайте файл в системный VPN-клиент.</p>
      </div>
      <div class="hero-shield" aria-hidden="true">DP</div>
    </section>

    <section class="workspace">
      <aside class="panel sidebar">
        <div class="section-heading">
          <div><span class="label">Хранилище</span><h2>Профили</h2></div>
          <button id="addButton" class="icon-button" type="button" aria-label="Добавить профиль">+</button>
        </div>
        <div id="profileList" class="profile-list"></div>
        <button id="addFirstButton" class="button primary full" type="button">Добавить профиль</button>
      </aside>

      <section id="profilePanel" class="panel profile-panel"></section>
    </section>

    <section class="notice">
      <strong>Как работает PWA</strong>
      <p>Браузер не может создавать системный VPN-туннель. DP Connect PWA хранит профиль в зашифрованном виде и безопасно передаёт его в DP Connect или совместимый WireGuard-клиент.</p>
    </section>
  </main>

  <dialog id="importDialog">
    <form id="importForm" method="dialog">
      <div class="dialog-heading"><div><span class="label">Новый профиль</span><h2>Импорт DP WG</h2></div><button class="dialog-close" value="cancel" aria-label="Закрыть">×</button></div>
      <label>Название<input id="profileName" name="name" maxlength="50" placeholder="Например, VPS Нидерланды" required /></label>
      <label class="file-field">Файл .conf<input id="configFile" type="file" accept=".conf,text/plain" /></label>
      <label>Конфигурация<textarea id="configText" name="config" rows="8" spellcheck="false" placeholder="[Interface]&#10;PrivateKey = …&#10;&#10;[Peer]&#10;PublicKey = …&#10;Endpoint = …" required></textarea></label>
      <div class="two-columns">
        <label>Пароль<input id="profilePassword" name="password" type="password" minlength="10" autocomplete="new-password" required /></label>
        <label>Повторите пароль<input id="profilePasswordConfirm" name="passwordConfirm" type="password" minlength="10" autocomplete="new-password" required /></label>
      </div>
      <p class="form-hint">Минимум 10 символов. Пароль не сохраняется и не восстанавливается.</p>
      <p id="importError" class="error-text" role="alert"></p>
      <button class="button primary full" type="submit">Зашифровать и сохранить</button>
    </form>
  </dialog>

  <dialog id="unlockDialog">
    <form id="unlockForm" method="dialog">
      <div class="dialog-heading"><div><span class="label">Защищённый профиль</span><h2>Разблокировать</h2></div><button class="dialog-close" value="cancel" aria-label="Закрыть">×</button></div>
      <label>Пароль<input id="unlockPassword" type="password" minlength="10" autocomplete="current-password" required /></label>
      <p id="unlockError" class="error-text" role="alert"></p>
      <button class="button primary full" type="submit">Открыть профиль</button>
    </form>
  </dialog>

  <div id="toast" class="toast" role="status" aria-live="polite"></div>
`;

const elements = {
  networkBadge: document.querySelector("#networkBadge"),
  installButton: document.querySelector("#installButton"),
  addButton: document.querySelector("#addButton"),
  addFirstButton: document.querySelector("#addFirstButton"),
  profileList: document.querySelector("#profileList"),
  profilePanel: document.querySelector("#profilePanel"),
  importDialog: document.querySelector("#importDialog"),
  importForm: document.querySelector("#importForm"),
  importError: document.querySelector("#importError"),
  configFile: document.querySelector("#configFile"),
  configText: document.querySelector("#configText"),
  unlockDialog: document.querySelector("#unlockDialog"),
  unlockForm: document.querySelector("#unlockForm"),
  unlockError: document.querySelector("#unlockError"),
  toast: document.querySelector("#toast"),
};

function loadProfiles() {
  try {
    const parsed = JSON.parse(localStorage.getItem(STORAGE_KEY) || "[]");
    return Array.isArray(parsed) ? parsed : [];
  } catch {
    return [];
  }
}

function persistProfiles() {
  localStorage.setItem(STORAGE_KEY, JSON.stringify(profiles));
}

function escapeHtml(value) {
  return String(value).replace(/[&<>'"]/g, (character) => ({
    "&": "&amp;", "<": "&lt;", ">": "&gt;", "'": "&#39;", '"': "&quot;",
  })[character]);
}

function selectedProfile() {
  return profiles.find((profile) => profile.id === selectedId) || null;
}

function render() {
  elements.profileList.innerHTML = profiles.map((profile) => `
    <button class="profile-item ${profile.id === selectedId ? "selected" : ""}" data-profile-id="${profile.id}" type="button">
      <span class="profile-avatar">DP</span>
      <span><strong>${escapeHtml(profile.name)}</strong><small>${new Date(profile.createdAt).toLocaleDateString("ru-RU")}</small></span>
      <span class="lock-dot" title="Зашифровано">●</span>
    </button>
  `).join("");
  elements.addFirstButton.classList.toggle("hidden", profiles.length > 0);

  const profile = selectedProfile();
  if (!profile) {
    elements.profilePanel.innerHTML = `
      <div class="empty-state">
        <div class="empty-icon">DP</div>
        <h2>Добавьте первый профиль</h2>
        <p>Конфигурация шифруется AES-256-GCM перед сохранением в браузере.</p>
        <button class="button primary" data-action="add" type="button">Импортировать .conf</button>
      </div>`;
    return;
  }

  if (!unlockedConfig) {
    elements.profilePanel.innerHTML = `
      <div class="locked-state">
        <span class="status-pill">AES-256-GCM</span>
        <div class="lock-visual">⌁</div>
        <h2>${escapeHtml(profile.name)}</h2>
        <p>Профиль заблокирован. Введите пароль, чтобы открыть QR-код или экспортировать конфигурацию.</p>
        <button class="button primary" data-action="unlock" type="button">Разблокировать</button>
        <button class="button danger-text" data-action="delete" type="button">Удалить профиль</button>
      </div>`;
    return;
  }

  const summary = parseWgConfig(unlockedConfig);
  elements.profilePanel.innerHTML = `
    <div class="profile-heading">
      <div><span class="status-pill online">Разблокирован</span><h2>${escapeHtml(profile.name)}</h2></div>
      <button class="button secondary" data-action="lock" type="button">Заблокировать</button>
    </div>
    <div class="profile-grid">
      <dl class="details">
        <div><dt>Endpoint</dt><dd>${escapeHtml(summary.endpoint)}</dd></div>
        <div><dt>IP-адрес</dt><dd>${escapeHtml(summary.address)}</dd></div>
        <div><dt>DNS</dt><dd>${escapeHtml(summary.dns)}</dd></div>
        <div><dt>Маршруты</dt><dd>${escapeHtml(summary.allowedIps)}</dd></div>
      </dl>
      <div class="qr-card"><canvas id="qrCanvas" aria-label="QR-код конфигурации"></canvas><small>QR содержит приватный ключ. Не показывайте его посторонним.</small></div>
    </div>
    <div class="profile-actions">
      <button class="button primary" data-action="share" type="button">Передать профиль</button>
      <button class="button secondary" data-action="download" type="button">Скачать .conf</button>
      <button class="button secondary" data-action="copy" type="button">Копировать</button>
    </div>`;

  QRCode.toCanvas(document.querySelector("#qrCanvas"), unlockedConfig, {
    width: 220,
    margin: 1,
    errorCorrectionLevel: "L",
    color: { dark: "#10171C", light: "#FFFFFF" },
  }).catch(() => showToast("Конфигурация слишком велика для QR-кода"));
}

function openImport() {
  elements.importForm.reset();
  elements.importError.textContent = "";
  elements.importDialog.showModal();
}

function lockProfile() {
  unlockedConfig = "";
  clearTimeout(lockTimer);
  render();
}

function armAutoLock() {
  clearTimeout(lockTimer);
  lockTimer = window.setTimeout(() => {
    lockProfile();
    showToast("Профиль автоматически заблокирован");
  }, AUTO_LOCK_MS);
}

function safeFileName(name) {
  return `${name.toLowerCase().replace(/[^a-zа-я0-9]+/gi, "-").replace(/^-|-$/g, "") || "dp-connect"}.conf`;
}

function profileFile() {
  return new File([unlockedConfig], safeFileName(selectedProfile().name), { type: "text/plain" });
}

function downloadProfile() {
  const file = profileFile();
  const link = document.createElement("a");
  link.href = URL.createObjectURL(file);
  link.download = file.name;
  link.click();
  URL.revokeObjectURL(link.href);
}

async function shareProfile() {
  const file = profileFile();
  if (navigator.share && navigator.canShare?.({ files: [file] })) {
    await navigator.share({ title: selectedProfile().name, files: [file] });
  } else {
    downloadProfile();
    showToast("Файл скачан — откройте его в VPN-клиенте");
  }
}

function showToast(message) {
  elements.toast.textContent = message;
  elements.toast.classList.add("visible");
  window.setTimeout(() => elements.toast.classList.remove("visible"), 2600);
}

function updateNetworkBadge() {
  elements.networkBadge.textContent = navigator.onLine ? "Онлайн" : "Офлайн";
  elements.networkBadge.className = `badge ${navigator.onLine ? "online" : "offline"}`;
}

elements.profileList.addEventListener("click", (event) => {
  const button = event.target.closest("[data-profile-id]");
  if (!button || button.dataset.profileId === selectedId) return;
  selectedId = button.dataset.profileId;
  unlockedConfig = "";
  render();
});

elements.profilePanel.addEventListener("click", async (event) => {
  const action = event.target.closest("[data-action]")?.dataset.action;
  if (!action) return;
  try {
    if (action === "add") openImport();
    if (action === "unlock") {
      elements.unlockForm.reset();
      elements.unlockError.textContent = "";
      elements.unlockDialog.showModal();
      document.querySelector("#unlockPassword").focus();
    }
    if (action === "lock") lockProfile();
    if (action === "download") downloadProfile();
    if (action === "share") await shareProfile();
    if (action === "copy") {
      await navigator.clipboard.writeText(unlockedConfig);
      showToast("Конфигурация скопирована");
    }
    if (action === "delete" && confirm(`Удалить профиль «${selectedProfile().name}»?`)) {
      profiles = profiles.filter((profile) => profile.id !== selectedId);
      selectedId = profiles[0]?.id || null;
      persistProfiles();
      lockProfile();
    }
  } catch (error) {
    showToast(error.message || "Операция не выполнена");
  }
});

elements.addButton.addEventListener("click", openImport);
elements.addFirstButton.addEventListener("click", openImport);
elements.configFile.addEventListener("change", async () => {
  const file = elements.configFile.files?.[0];
  if (file) elements.configText.value = await file.text();
});

elements.importForm.addEventListener("submit", async (event) => {
  event.preventDefault();
  elements.importError.textContent = "";
  const name = document.querySelector("#profileName").value.trim();
  const config = elements.configText.value.trim();
  const password = document.querySelector("#profilePassword").value;
  const confirmation = document.querySelector("#profilePasswordConfirm").value;
  try {
    if (password.length < 10) throw new Error("Пароль должен содержать минимум 10 символов");
    if (password !== confirmation) throw new Error("Пароли не совпадают");
    parseWgConfig(config);
    const envelope = await encryptProfile(`${config}\n`, password);
    const profile = { id: crypto.randomUUID(), name, createdAt: new Date().toISOString(), envelope };
    profiles.unshift(profile);
    selectedId = profile.id;
    persistProfiles();
    elements.importDialog.close();
    render();
    showToast("Профиль сохранён и зашифрован");
  } catch (error) {
    elements.importError.textContent = error.message || "Не удалось сохранить профиль";
  }
});

elements.unlockForm.addEventListener("submit", async (event) => {
  event.preventDefault();
  elements.unlockError.textContent = "";
  try {
    unlockedConfig = await decryptProfile(selectedProfile().envelope, document.querySelector("#unlockPassword").value);
    parseWgConfig(unlockedConfig);
    elements.unlockDialog.close();
    armAutoLock();
    render();
  } catch {
    unlockedConfig = "";
    elements.unlockError.textContent = "Неверный пароль или повреждённый профиль";
  }
});

window.addEventListener("online", updateNetworkBadge);
window.addEventListener("offline", updateNetworkBadge);
window.addEventListener("beforeinstallprompt", (event) => {
  event.preventDefault();
  deferredInstallPrompt = event;
  elements.installButton.classList.remove("hidden");
});
elements.installButton.addEventListener("click", async () => {
  if (!deferredInstallPrompt) return;
  deferredInstallPrompt.prompt();
  await deferredInstallPrompt.userChoice;
  deferredInstallPrompt = null;
  elements.installButton.classList.add("hidden");
});

if ("serviceWorker" in navigator) {
  window.addEventListener("load", () => navigator.serviceWorker.register("./sw.js"));
}

updateNetworkBadge();
render();
