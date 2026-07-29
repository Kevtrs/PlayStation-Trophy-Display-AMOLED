#!/usr/bin/env python3
"""Outil PC independant de sondage PUBLIC de pocketpsn.com (sans cle, sans
authentification, sans contournement d'aucune protection).

Teste uniquement des ressources qui seraient accessibles a n'importe quel
visiteur anonyme : la page de profil publique https://pocketpsn.com/<pseudo>,
la racine du domaine, et deux fichiers habituellement publics
(robots.txt/sitemap.xml) utilises ici uniquement pour verifier l'etendue
d'une eventuelle protection anti-robot rencontree -- pas pour deviner une
route d'API.

Cet outil ne tente JAMAIS de resoudre un defi anti-robot (Cloudflare ou
autre) : si la reponse est detectee comme un defi/blocage, il le signale
explicitement dans le rapport au lieu de tenter de le contourner (pas de
navigateur headless, pas de resolution de CAPTCHA, pas d'usurpation
d'empreinte navigateur).
"""

import argparse
import datetime
import json
import os
import re
import sys
import urllib.error
import urllib.request

USER_AGENT = (
    "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 "
    "(KHTML, like Gecko) Chrome/124.0 Safari/537.36"
)
REQUEST_TIMEOUT_SECONDS = 15
MAX_RESPONSE_BYTES = 4 * 1024 * 1024
ANONYMIZED_PLACEHOLDER = "TEST_PSN_USERNAME"

# Noms de champs attendus par src/data/PocketPsnParser.cpp (voir
# docs/POCKETPSN_PROTOCOL.md) -- reference pour la comparaison, jamais
# modifies ici.
EXPECTED_PARSER_FIELDS = [
    "Status", "Username", "Country", "PSN Level", "PSN Level Progress",
    "PSN Level Remaining", "Trophies Bronze", "Trophies Silver", "Trophies Gold",
    "Trophies Plats", "Trophies Total", "Trophy Points", "Pocket Points",
    "Total Games", "World Rank", "Country Rank", "Quick Stats",
    "Games Completed", "Completion Average", "Average Rarity",
    "Unearned Trophies", "Hours Played",
]

CLOUDFLARE_CHALLENGE_MARKERS = (
    "just a moment",
    "enable javascript and cookies to continue",
    "challenges.cloudflare.com",
    "cf-mitigated",
)


def fetch(url, timeout):
    """Requete GET simple, sans cookies/session persistants, sans aucune
    tentative de resolution de defi. Renvoie (status, headers_dict, body_bytes)."""
    request = urllib.request.Request(
        url, headers={"User-Agent": USER_AGENT, "Accept": "text/html,application/json,*/*"}
    )
    try:
        with urllib.request.urlopen(request, timeout=timeout) as response:
            return response.status, dict(response.headers.items()), response.read(MAX_RESPONSE_BYTES)
    except urllib.error.HTTPError as exc:
        headers = dict(exc.headers.items()) if exc.headers else {}
        body = exc.read(MAX_RESPONSE_BYTES) if exc.fp else b""
        return exc.code, headers, body
    except urllib.error.URLError as exc:
        return None, {"error": str(exc.reason)}, b""


def is_bot_challenge(status, headers, body_text):
    lowered_headers = {k.lower(): v for k, v in headers.items()}
    if "cf-mitigated" in lowered_headers:
        return True
    lowered_body = body_text.lower()
    return status == 403 and any(marker in lowered_body for marker in CLOUDFLARE_CHALLENGE_MARKERS)


def extract_embedded_json_scripts(html_text):
    """Cherche les <script type="application/json">, <script id="...">
    (ex: hydration Next.js/Nuxt) contenant du JSON. Ne devine rien : ne
    retient que ce qui est reellement present et parse comme JSON valide."""
    found = []
    for match in re.finditer(
        r'<script\b([^>]*)>(.*?)</script>', html_text, re.DOTALL | re.IGNORECASE
    ):
        attrs, content = match.group(1), match.group(2).strip()
        is_json_type = 'type="application/json"' in attrs or "type='application/json'" in attrs
        id_match = re.search(r'id=["\']([^"\']+)["\']', attrs)
        looks_like_data_id = bool(id_match) and (
            "data" in id_match.group(1).lower() or "state" in id_match.group(1).lower()
        )
        if not content or not (is_json_type or looks_like_data_id):
            continue
        try:
            json.loads(content)
            valid = True
        except json.JSONDecodeError:
            valid = False
        if is_json_type or valid:
            found.append({
                "id": id_match.group(1) if id_match else "(sans id)",
                "length": len(content),
                "valid_json": valid,
            })
    return found


def extract_api_like_urls(html_text):
    """Cherche des URLs referencees dans des appels fetch()/XHR ou des
    chemins /api/ presents tels quels dans la page. Ne devine ni n'invente
    aucune route non observee."""
    patterns = [
        r'fetch\(\s*["\']([^"\']+)["\']',
        r'\.open\(\s*["\'][A-Za-z]+["\']\s*,\s*["\']([^"\']+)["\']',
        r'["\'](https?://[a-zA-Z0-9.\-]*pocketpsn[a-zA-Z0-9.\-]*/[^"\'<>\s]*)["\']',
        r'["\'](/api/[^"\'<>\s]*)["\']',
    ]
    found = set()
    for pattern in patterns:
        for match in re.finditer(pattern, html_text):
            found.add(match.group(1))
    return sorted(found)


def find_recoverable_fields(html_text):
    """Compare le texte brut recupere aux noms de champs attendus par
    PocketPsnParser -- ne renvoie que ceux dont le libelle apparait
    explicitement dans la reponse (aucune deduction)."""
    return [field for field in EXPECTED_PARSER_FIELDS if field.lower() in html_text.lower()]


def anonymize(text, psn_name):
    if not psn_name:
        return text
    return text.replace(psn_name, ANONYMIZED_PLACEHOLDER)


def probe(psn_name, output_dir, fixtures_dir, timeout):
    targets = [
        ("profile_page", f"https://pocketpsn.com/{psn_name}"),
        ("domain_root", "https://pocketpsn.com/"),
        ("robots_txt", "https://pocketpsn.com/robots.txt"),
        ("sitemap_xml", "https://pocketpsn.com/sitemap.xml"),
    ]

    os.makedirs(output_dir, exist_ok=True)
    os.makedirs(fixtures_dir, exist_ok=True)
    timestamp = datetime.datetime.now().strftime("%Y%m%d_%H%M%S")

    results = []
    for name, url in targets:
        print(f"GET {url}")
        status, headers, body = fetch(url, timeout)
        body_text = body.decode("utf-8", errors="replace")
        blocked = status is not None and is_bot_challenge(status, headers, body_text)
        content_type = headers.get("Content-Type", headers.get("content-type", "(absent)"))

        print(f"  HTTP status  : {status}")
        print(f"  Content-Type : {content_type}")
        print(f"  Defi anti-robot detecte : {'oui (Cloudflare)' if blocked else 'non'}")

        raw_path = os.path.join(output_dir, f"{name}_{timestamp}.html")
        with open(raw_path, "wb") as f:
            f.write(body)

        anonymized_text = anonymize(body_text, psn_name)
        fixture_path = os.path.join(fixtures_dir, f"{name}_anonymized.html")
        with open(fixture_path, "w", encoding="utf-8") as f:
            f.write(anonymized_text)

        embedded_scripts = [] if blocked else extract_embedded_json_scripts(body_text)
        api_urls = [] if blocked else extract_api_like_urls(body_text)
        recoverable_fields = [] if blocked else find_recoverable_fields(body_text)

        results.append({
            "target": name,
            "url": url,
            "http_status": status,
            "content_type": content_type,
            "bot_challenge_detected": blocked,
            "raw_html_saved": raw_path,
            "anonymized_fixture": fixture_path,
            "embedded_json_scripts": embedded_scripts,
            "public_api_urls_found": api_urls,
            "recoverable_profile_fields": recoverable_fields,
        })

    all_recoverable_fields = sorted({f for r in results for f in r["recoverable_profile_fields"]})
    all_api_urls = sorted({u for r in results for u in r["public_api_urls_found"]})
    all_blocked = all(r["bot_challenge_detected"] for r in results if r["http_status"] is not None)

    missing_report = []
    if all_blocked:
        missing_report.append(
            "Toutes les ressources testees (page de profil, racine du domaine, "
            "robots.txt, sitemap.xml) renvoient un defi anti-robot Cloudflare "
            "(HTTP 403, en-tete cf-mitigated=challenge, page 'Just a moment...'). "
            "Aucune donnee de profil, aucun script JSON embarque, aucune URL "
            "d'API publique n'a pu etre observee sans executer le defi "
            "JavaScript -- ce que cet outil ne fait jamais (voir consigne : ne "
            "contourner aucune protection)."
        )
    else:
        if not all_recoverable_fields:
            missing_report.append(
                "Aucun champ attendu par PocketPsnParser n'a ete retrouve tel "
                "quel dans le texte recupere."
            )
        if not all_api_urls:
            missing_report.append("Aucune URL d'API publique n'a ete identifiee dans les pages recuperees.")

    report = {
        "psn_name_tested": psn_name,
        "timestamp": timestamp,
        "results": results,
        "summary": {
            "all_targets_blocked_by_bot_challenge": all_blocked,
            "recoverable_profile_fields": all_recoverable_fields,
            "public_api_urls_found": all_api_urls,
            "expected_parser_fields_reference": EXPECTED_PARSER_FIELDS,
            "missing": missing_report,
        },
    }

    report_path = os.path.join(output_dir, f"report_{timestamp}.json")
    with open(report_path, "w", encoding="utf-8") as f:
        json.dump(report, f, indent=2, ensure_ascii=False)

    anonymized_report_path = os.path.join(fixtures_dir, "report_anonymized.json")
    with open(anonymized_report_path, "w", encoding="utf-8") as f:
        json.dump(json.loads(anonymize(json.dumps(report), psn_name)), f, indent=2, ensure_ascii=False)

    print()
    print(f"Rapport complet : {report_path}")
    print(f"Rapport anonymise (committable) : {anonymized_report_path}")
    print()
    print("--- Resume ---")
    if all_blocked:
        print("BLOQUE : defi anti-robot Cloudflare sur toutes les ressources testees.")
    print(f"Champs PocketPsnParser recuperables : {all_recoverable_fields or '(aucun)'}")
    print(f"URLs d'API publiques trouvees : {all_api_urls or '(aucune)'}")
    for line in missing_report:
        print(f"Manquant : {line}")

    return 0


def main():
    parser = argparse.ArgumentParser(
        description="Sonde uniquement les ressources publiques (sans cle/authentification) de pocketpsn.com.",
    )
    parser.add_argument("psn_name", help="Pseudo PSN public a interroger (le votre, jamais celui d'un tiers sans accord)")
    parser.add_argument("--output-dir", default=os.path.join(os.path.dirname(__file__), "output"))
    parser.add_argument("--fixtures-dir", default=os.path.join(os.path.dirname(__file__), "fixtures"))
    parser.add_argument("--timeout", type=float, default=REQUEST_TIMEOUT_SECONDS)
    args = parser.parse_args()

    return probe(args.psn_name, args.output_dir, args.fixtures_dir, args.timeout)


if __name__ == "__main__":
    raise SystemExit(main())
