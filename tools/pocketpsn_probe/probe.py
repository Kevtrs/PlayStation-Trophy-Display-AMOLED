#!/usr/bin/env python3
"""Outil PC independant de sondage de l'API Pocket PSN.

Endpoint, encodage et parametres repris tels quels de
docs/POCKETPSN_PROTOCOL.md, identifies par inspection passive (recherche de
chaines ASCII precises, jamais un dump brut d'une zone memoire -- voir
AUDIT.md section 0ter) du firmware officiel PlaystationTrophy.bin (release
Playstation_Trophy_API_1.0). Aucun endpoint ni champ supplementaire n'est
devine ici. La cle API doit etre fournie par l'utilisateur (jamais codee en
dur dans ce fichier) -- une cle privee et legitime a ete obtenue directement
aupres du proprietaire de Pocket PSN.

Cet outil ne contourne aucune authentification : il se contente de relayer
ce que l'utilisateur lui fournit, eventuellement avec un en-tete User-Agent
different (voir --variant) -- jamais une cle differente, jamais un
endpoint different. La cle n'est jamais journalisee en clair, jamais
acceptee en argument positionnel par defaut, et n'apparait dans aucun
fichier commite (voir .gitignore : output/, fixtures/, *.key, *key*.txt).

Diagnostic HTTP 200 + corps vide (constate reellement lors des deux
premiers tests reels) : inspection ciblee du binaire (recherche des seules
chaines "Content-Type", "psn_name", "key=", l'URL, et les libelles d'en-tete
HTTP generiques -- jamais un dump de la zone contenant la cle candidate
identifiee en section 3 de AUDIT.md) a trouve une chaine "PSTrophyDisplayESP"
juste avant l'URL dans le binaire, plausible valeur d'en-tete User-Agent
attendue par le serveur. --variant permet de tester cette hypothese sans
modifier le firmware ni le parser.
"""

import argparse
import datetime
import getpass
import json
import os
import sys
import urllib.error
import urllib.parse
import urllib.request

POCKETPSN_ENDPOINT = "https://api.pocketpsn.com/PSTrophyDisplay/"
POCKETPSN_ENDPOINT_NO_TRAILING_SLASH = "https://api.pocketpsn.com/PSTrophyDisplay"
REQUEST_TIMEOUT_SECONDS = 15
MAX_RESPONSE_BYTES = 2 * 1024 * 1024  # garde-fou taille max, cf. section 15 du brief

# Champs documentes dans docs/POCKETPSN_PROTOCOL.md (jamais confirmes par
# une vraie reponse avant ce test) -- sert uniquement a la comparaison
# affichee apres la requete, ne bloque jamais rien.
DOCUMENTED_TOP_LEVEL_FIELDS = [
    "Username", "Country", "PSN Level", "PSN Level Progress", "PSN Level Remaining",
    "Trophies Plats", "Trophies Gold", "Trophies Silver", "Trophies Bronze", "Trophies Total",
    "Trophy Points", "Pocket Points", "Total Games", "World Rank", "Country Rank", "Quick Stats",
]
DOCUMENTED_QUICK_STATS_FIELDS = [
    "Games Completed", "Completion Average", "Average Rarity", "Unearned Trophies", "Hours Played",
]


def mask(value: str, keep: int = 2) -> str:
    """Masque une valeur sensible pour les logs (ne jamais logger en clair)."""
    if not value:
        return "(vide)"
    if len(value) <= keep:
        return "*" * len(value)
    return value[:keep] + "*" * (len(value) - keep)


def resolve_api_key(args) -> str:
    """Ordre de priorite, du plus sur au moins sur :
    1. --key-file (fichier local ignore par Git, jamais affiche)
    2. variable d'environnement POCKETPSN_API_KEY
    3. saisie interactive masquee (getpass -- n'apparait jamais dans
       l'historique du terminal, contrairement a un argument de commande)
    4. --key (deconseille : reste dans l'historique du shell)
    """
    if args.key_file:
        with open(args.key_file, "r", encoding="utf-8") as f:
            return f.read().strip()
    if os.environ.get("POCKETPSN_API_KEY"):
        return os.environ["POCKETPSN_API_KEY"].strip()
    if args.key:
        print(
            "Avertissement : --key expose la cle dans l'historique de votre shell. "
            "Preferez --key-file ou la saisie interactive.",
            file=sys.stderr,
        )
        return args.key.strip()
    print("Aucune cle fournie via --key-file ou POCKETPSN_API_KEY.")
    return getpass.getpass("Cle API Pocket PSN (saisie masquee, jamais affichee) : ").strip()


def build_variants(psn_name: str, api_key: str):
    """Variantes controlees de la requete, une seule variable modifiee a la
    fois par rapport a la base documentee, pour isoler ce qui declenche
    (ou non) une vraie reponse. N'invente aucun nouveau parametre/endpoint :
    seule la presence/valeur de l'en-tete User-Agent et la presence du
    slash final sont testees, les deux seuls points d'incertitude reels
    identifies par inspection du binaire."""
    body = urllib.parse.urlencode({"psn_name": psn_name, "key": api_key}).encode("ascii")
    base_headers = {"Content-Type": "application/x-www-form-urlencoded"}

    # Corps par concatenation brute, exactement comme le firmware d'origine :
    #   "psn_name=" + psnUsername_ + "&key=" + apiKey_
    # AUCUN percent-encodage, contrairement a urllib.parse.urlencode() ci-dessus.
    # C'est la difference reelle avec le firmware qui, lui, fonctionne : si la
    # cle contient des caracteres que urlencode transforme (+, /, =, espace...),
    # le serveur recoit une cle differente -> 200 + corps vide.
    raw_body = f"psn_name={psn_name}&key={api_key}".encode("utf-8")

    return [
        {
            "name": "corps_brut_comme_firmware",
            "description": "Corps concatene brut, SANS url-encodage (identique au firmware d'origine). Hypothese principale apres l'echec des variantes User-Agent.",
            "url": POCKETPSN_ENDPOINT,
            "headers": dict(base_headers),
            "body": raw_body,
        },
        {
            "name": "corps_brut_avec_user_agent",
            "description": "Corps brut + User-Agent du firmware (au cas ou les deux seraient necessaires ensemble).",
            "url": POCKETPSN_ENDPOINT,
            "headers": {**base_headers, "User-Agent": "PSTrophyDisplayESP"},
            "body": raw_body,
        },
        {
            "name": "baseline_urlencode",
            "description": "Comportement precedent de l'outil (urllib.parse.urlencode) -- pour comparaison : c'est celui qui renvoyait un corps vide.",
            "url": POCKETPSN_ENDPOINT,
            "headers": dict(base_headers),
            "body": body,
        },
    ]


def print_encoding_diagnostic(psn_name: str, api_key: str) -> None:
    """Indique, SANS jamais afficher les valeurs, si le percent-encodage
    URL modifie le pseudo ou la cle. Si la cle change une fois encodee,
    c'est tres probablement la cause du corps vide (le firmware d'origine
    envoie la cle brute, non encodee)."""
    print("=== Diagnostic d'encodage (aucune valeur affichee) ===")
    for label, value in (("psn_name", psn_name), ("cle     ", api_key)):
        encoded = urllib.parse.quote_plus(value)
        changed = encoded != value
        print(f"{label} : longueur brute={len(value):<3} longueur url-encodee={len(encoded):<3} "
              f"-> {'MODIFIEE par url-encodage (suspect)' if changed else 'inchangee'}")

    # Detecte des caracteres invisibles (BOM, espace insecable, saut de
    # ligne residuel, espace...) qui survivent souvent a un copier-coller
    # sans changer la longueur visible dans un editeur -- indique
    # uniquement la POSITION et le code du caractere suspect, jamais la
    # cle elle-meme ni le caractere en clair.
    for label, value in (("psn_name", psn_name), ("cle     ", api_key)):
        suspects = [(i, hex(ord(c))) for i, c in enumerate(value) if not c.isascii() or not c.isprintable() or c == " "]
        if suspects:
            positions = [i for i, _ in suspects]
            codes = [c for _, c in suspects]
            print(f"{label} : caractere(s) invisible(s)/inattendu(s) a la/aux position(s) {positions} "
                  f"(code(s) {codes}) -- probablement un copier-coller imparfait.")
        else:
            print(f"{label} : aucun caractere invisible detecte.")
    print()


def anonymize(raw_text: str, psn_name: str) -> str:
    """Remplace le pseudo demande partout ou il apparait tel quel dans la
    reponse (nom d'utilisateur, eventuel champ libre) -- seule donnee
    identifiante que l'on connait a coup sur avant d'avoir vu le vrai
    schema. Ne devine aucun autre champ a redacter : le schema reel sera
    inspecte manuellement avant tout usage de cette fixture."""
    if not psn_name:
        return raw_text
    return raw_text.replace(psn_name, "TEST_PSN_USERNAME")


def compare_to_documented_schema(parsed) -> None:
    """Affiche un tableau texte des ecarts entre docs/POCKETPSN_PROTOCOL.md
    et la vraie reponse -- jamais utilise pour modifier quoi que ce soit
    automatiquement, uniquement pour informer une decision humaine avant
    toute modification du parser."""
    if not isinstance(parsed, dict):
        print("La reponse n'est pas un objet JSON au premier niveau -- comparaison de schema impossible.")
        return

    real_keys = set(parsed.keys())
    documented_keys = set(DOCUMENTED_TOP_LEVEL_FIELDS)

    print("\n=== Comparaison avec docs/POCKETPSN_PROTOCOL.md (champs de premier niveau) ===")
    print(f"{'Champ':<24} {'Documente':<12} {'Present (reel)':<15}")
    for field in sorted(documented_keys | real_keys):
        print(f"{field:<24} {'oui' if field in documented_keys else 'non':<12} "
              f"{'oui' if field in real_keys else 'non':<15}")

    missing = documented_keys - real_keys
    extra = real_keys - documented_keys
    if missing:
        print(f"\nDocumentes mais ABSENTS de la vraie reponse : {sorted(missing)}")
    if extra:
        print(f"Presents dans la vraie reponse mais NON documentes : {sorted(extra)}")
    if not missing and not extra:
        print("\nAucun ecart de champs de premier niveau.")

    quick_stats = parsed.get("Quick Stats")
    if quick_stats is not None:
        print(f"\n'Quick Stats' est un(e) {type(quick_stats).__name__} dans la vraie reponse "
              f"(documente comme un objet unique -- a verifier si c'est bien le cas).")
        if isinstance(quick_stats, dict):
            real_qs_keys = set(quick_stats.keys())
            documented_qs_keys = set(DOCUMENTED_QUICK_STATS_FIELDS)
            missing_qs = documented_qs_keys - real_qs_keys
            extra_qs = real_qs_keys - documented_qs_keys
            if missing_qs:
                print(f"  Quick Stats -- documentes mais absents : {sorted(missing_qs)}")
            if extra_qs:
                print(f"  Quick Stats -- presents mais non documentes : {sorted(extra_qs)}")
            if not missing_qs and not extra_qs:
                print("  Quick Stats -- aucun ecart.")
    elif "Quick Stats" in documented_keys:
        print("\n'Quick Stats' est documente mais absent de la vraie reponse.")


def print_safe_diagnostics(status, response_headers, body: bytes, api_key: str) -> None:
    """N'affiche jamais la cle elle-meme, uniquement sa longueur -- utile
    pour ecarter une erreur de saisie (espace, retour a la ligne, troncature
    lors d'un copier-coller) sans jamais journaliser la valeur."""
    print(f"Longueur de la cle apres trim   : {len(api_key)} caractere(s)")
    print(f"Longueur du corps recu           : {len(body)} octet(s)")
    content_length_header = response_headers.get("Content-Length", "(absent)") if response_headers else "(absent)"
    print(f"En-tete Content-Length (reponse) : {content_length_header}")
    if response_headers:
        print("En-tetes de reponse complets (aucun n'est un secret, ce sont des en-tetes SERVEUR) :")
        for key, value in response_headers.items():
            print(f"  {key}: {value}")


def run_variant(variant, psn_name: str, api_key: str, output_dir: str, fixtures_dir: str, timeout: float):
    request = urllib.request.Request(
        variant["url"], data=variant["body"], method="POST", headers=variant["headers"]
    )

    print(f"\n{'=' * 70}")
    print(f"Variante : {variant['name']}")
    print(f"  {variant['description']}")
    print(f"POST {variant['url']}")
    for key, value in variant["headers"].items():
        print(f"  {key}: {value}")
    print(f"psn_name = {mask(psn_name, keep=2)}")
    print("key      = (masquee, jamais affichee)")

    try:
        with urllib.request.urlopen(request, timeout=timeout) as response:
            status = response.status
            headers = response.headers
            raw = response.read(MAX_RESPONSE_BYTES)
    except urllib.error.HTTPError as exc:
        status = exc.code
        headers = exc.headers
        raw = exc.read(MAX_RESPONSE_BYTES) if exc.fp else b""
    except urllib.error.URLError as exc:
        print(f"Echec reseau : {exc.reason}", file=sys.stderr)
        return None

    content_type = headers.get("Content-Type", "(absent)") if headers else "(absent)"
    print(f"HTTP status  : {status}")
    print(f"Content-Type : {content_type}")
    print_safe_diagnostics(status, headers, raw, api_key)

    os.makedirs(output_dir, exist_ok=True)
    os.makedirs(fixtures_dir, exist_ok=True)
    timestamp = datetime.datetime.now().strftime("%Y%m%d_%H%M%S")
    extension = "json" if "json" in content_type.lower() else "txt"

    raw_path = os.path.join(output_dir, f"pocketpsn_response_{variant['name']}_{timestamp}.{extension}")
    with open(raw_path, "wb") as f:
        f.write(raw)
    print(f"Reponse brute enregistree (donnees reelles, jamais commitee) : {raw_path}")

    if raw:
        raw_text = raw.decode("utf-8", errors="replace")
        anonymized_text = anonymize(raw_text, psn_name)
        fixture_path = os.path.join(
            fixtures_dir, f"pocketpsn_response_anonymized_{variant['name']}_{timestamp}.{extension}"
        )
        with open(fixture_path, "w", encoding="utf-8") as f:
            f.write(anonymized_text)
        print(f"Copie anonymisee ecrite pour relecture humaine : {fixture_path}")
        print("Cette copie n'est PAS committee automatiquement -- relisez-la avant de la garder "
              "(voir tools/pocketpsn_probe/README.md).")

    parsed = None
    if extension == "json" and raw:
        try:
            parsed = json.loads(raw)
            count = len(parsed) if isinstance(parsed, (dict, list)) else "?"
            print(f"JSON valide, {count} cle(s)/element(s) au premier niveau.")
        except json.JSONDecodeError as exc:
            print(f"Avertissement : Content-Type indique JSON mais le corps ne parse pas ({exc}).")

    if parsed is not None:
        compare_to_documented_schema(parsed)
    elif not raw:
        print("Corps vide.")

    return {"name": variant["name"], "status": status, "body_length": len(raw), "has_data": bool(raw)}


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Sonde l'endpoint Pocket PSN identifie par inspection passive du firmware officiel.",
    )
    parser.add_argument("psn_name", help="Pseudo PSN a interroger")
    parser.add_argument("--key", help="Cle API en argument direct (DECONSEILLE : reste dans l'historique du shell).")
    parser.add_argument("--key-file", help="Fichier local (ignore par Git) contenant uniquement la cle API.")
    parser.add_argument(
        "--variant",
        choices=["all", "corps_brut_comme_firmware", "corps_brut_avec_user_agent", "baseline_urlencode"],
        default="all",
        help="Variante de requete a tester (par defaut : toutes, l'une apres l'autre).",
    )
    parser.add_argument("--output-dir", default=os.path.join(os.path.dirname(__file__), "output"))
    parser.add_argument("--fixtures-dir", default=os.path.join(os.path.dirname(__file__), "fixtures"))
    parser.add_argument("--timeout", type=float, default=REQUEST_TIMEOUT_SECONDS)
    args = parser.parse_args()

    api_key = resolve_api_key(args)
    if not api_key:
        print("Erreur : aucune cle Pocket PSN fournie.", file=sys.stderr)
        return 1

    print_encoding_diagnostic(args.psn_name, api_key)

    variants = build_variants(args.psn_name, api_key)
    if args.variant != "all":
        variants = [v for v in variants if v["name"] == args.variant]

    results = []
    for variant in variants:
        result = run_variant(variant, args.psn_name, api_key, args.output_dir, args.fixtures_dir, args.timeout)
        if result:
            results.append(result)

    print(f"\n{'=' * 70}")
    print("=== Resume ===")
    for r in results:
        print(f"{r['name']:<32} HTTP {r['status']:<5} {r['body_length']} octet(s) "
              f"{'(donnees recues)' if r['has_data'] else '(corps vide)'}")

    any_data = any(r["has_data"] for r in results)
    return 0 if any_data else 2


if __name__ == "__main__":
    raise SystemExit(main())
