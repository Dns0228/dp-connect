const encoder = new TextEncoder();
const decoder = new TextDecoder();
const ITERATIONS = 600_000;

function bytesToBase64(bytes) {
  let binary = "";
  for (const byte of bytes) binary += String.fromCharCode(byte);
  return btoa(binary);
}

function base64ToBytes(value) {
  const binary = atob(value);
  return Uint8Array.from(binary, (character) => character.charCodeAt(0));
}

async function deriveKey(password, salt, usage) {
  const material = await crypto.subtle.importKey(
    "raw",
    encoder.encode(password),
    "PBKDF2",
    false,
    ["deriveKey"],
  );

  return crypto.subtle.deriveKey(
    { name: "PBKDF2", hash: "SHA-256", salt, iterations: ITERATIONS },
    material,
    { name: "AES-GCM", length: 256 },
    false,
    usage,
  );
}

export async function encryptProfile(config, password) {
  const salt = crypto.getRandomValues(new Uint8Array(16));
  const iv = crypto.getRandomValues(new Uint8Array(12));
  const key = await deriveKey(password, salt, ["encrypt"]);
  const ciphertext = await crypto.subtle.encrypt({ name: "AES-GCM", iv }, key, encoder.encode(config));

  return {
    version: 1,
    cipher: "AES-256-GCM",
    kdf: "PBKDF2-HMAC-SHA256",
    iterations: ITERATIONS,
    salt: bytesToBase64(salt),
    iv: bytesToBase64(iv),
    data: bytesToBase64(new Uint8Array(ciphertext)),
  };
}

export async function decryptProfile(envelope, password) {
  if (
    envelope?.version !== 1 ||
    envelope?.cipher !== "AES-256-GCM" ||
    envelope?.kdf !== "PBKDF2-HMAC-SHA256" ||
    envelope?.iterations !== ITERATIONS
  ) {
    throw new Error("Неподдерживаемый формат защищённого профиля");
  }

  const iv = base64ToBytes(envelope.iv);
  const key = await deriveKey(password, base64ToBytes(envelope.salt), ["decrypt"]);
  const plaintext = await crypto.subtle.decrypt(
    { name: "AES-GCM", iv },
    key,
    base64ToBytes(envelope.data),
  );
  return decoder.decode(plaintext);
}

export function parseWgConfig(config) {
  const sections = {};
  let current = "";

  for (const rawLine of config.split(/\r?\n/)) {
    const line = rawLine.trim();
    if (!line || line.startsWith("#") || line.startsWith(";")) continue;
    const section = line.match(/^\[([^\]]+)]$/);
    if (section) {
      current = section[1].toLowerCase();
      sections[current] ||= {};
      continue;
    }
    const separator = line.indexOf("=");
    if (separator > 0 && current) {
      sections[current][line.slice(0, separator).trim().toLowerCase()] = line.slice(separator + 1).trim();
    }
  }

  if (!sections.interface?.privatekey || !sections.peer?.publickey || !sections.peer?.endpoint) {
    throw new Error("Нужен полный профиль DP WG/WireGuard с Interface, PrivateKey, Peer и Endpoint");
  }

  return {
    address: sections.interface.address || "—",
    dns: sections.interface.dns || "—",
    endpoint: sections.peer.endpoint,
    allowedIps: sections.peer.allowedips || "—",
    persistentKeepalive: sections.peer.persistentkeepalive || "—",
  };
}
