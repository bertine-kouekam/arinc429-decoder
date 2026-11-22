# Décodeur ARINC 429 (format BNR) en langage C

[![CI](https://github.com/bertine-kouekam/arinc429-decoder/actions/workflows/ci.yml/badge.svg)](https://github.com/bertine-kouekam/arinc429-decoder/actions/workflows/ci.yml)

Bibliothèque en langage C qui décode et encode des mots ARINC 429 au format
binaire (BNR) : contrôle de la parité, lecture du label, du SDI et de la SSM,
conversion entre le champ de données et la valeur physique.

## Format du mot

Un mot fait 32 bits. Le bit 1 est le premier transmis.

| Bits | Champ | Rôle |
|---|---|---|
| 32 | Parité | Parité impaire sur les 32 bits |
| 31-30 | SSM | État de la donnée : 00 panne, 01 donnée non calculée, 10 test, 11 normal |
| 29-11 | Données | Bit 29 = signe, valeur en complément à deux cadrée à gauche |
| 10-9 | SDI | Identifiant de source ou de destination |
| 8-1 | Label | Type de la donnée, noté en octal |

Le label est transmis dans l'ordre inverse du reste du mot : ses huit bits
sont inversés à la lecture.

Valeur physique = entier signé × plage / 2^(bits significatifs).

## Exemple de calcul

Altitude de 35 000 ft sur le label 203 (17 bits significatifs, plage
131 072 ft, soit 1 ft par pas) :

- données : 35 000 pas, cadrés sur 18 bits, soit 70 000 = `0x11170`, placés à
  partir du bit 11 ;
- label 203 en octal = `1000 0011`, inversé = `1100 0001` = `0xC1` ;
- SSM = 11 (fonctionnement normal), SDI = 0 ;
- le mot contient alors 11 bits à 1 : la parité est déjà impaire, le bit 32
  reste à 0.

Mot obtenu : `0x6445C0C1`.

## Utilisation

```c
#include "arinc429.h"

static const a429_bnr_param_t ALTITUDE = { 0203, 17u, 131072.0f };

uint32_t mot = 0x6445C0C1u;
float altitude_ft;

if (a429_bnr_decode(mot, &ALTITUDE, &altitude_ft) == A429_OK
    && a429_ssm(mot) == A429_SSM_NORMAL_OPERATION) {
    printf("%.0f ft\n", altitude_ft);
}
```

Le décodage contrôle la parité et le label. La SSM est lue à part avec
`a429_ssm()` : c'est à l'appelant de décider s'il utilise la valeur.

## Exigences

| ID | Exigence |
|---|---|
| REQ-A429-001 | Un mot est valide si son nombre de bits à 1 est impair. |
| REQ-A429-002 | Le label est restitué en inversant l'ordre des bits 1 à 8. |
| REQ-A429-003 | Le SDI est lu sur les bits 9 et 10. |
| REQ-A429-004 | La SSM est lue sur les bits 30 et 31. |
| REQ-A429-005 | Le décodage BNR lit les bits 11 à 29 en complément à deux et ignore les bits situés sous les bits significatifs. |
| REQ-A429-006 | L'encodage BNR arrondit la valeur au pas le plus proche et positionne le bit 32 pour obtenir une parité impaire. |
| REQ-A429-007 | Un mot de parité incorrecte, un label inattendu, une valeur hors plage ou un argument invalide sont refusés sans modifier la sortie. |
| REQ-A429-008 | Ni allocation dynamique ni état global. |

## Tests

| Cas | Fonction | Exigences |
|---|---|---|
| TC-A429-001 | `test_parity` | REQ-A429-001 |
| TC-A429-002 | `test_label` | REQ-A429-002 |
| TC-A429-003 | `test_sdi_ssm` | REQ-A429-003, REQ-A429-004 |
| TC-A429-004 | `test_encode_altitude` | REQ-A429-006 |
| TC-A429-005 | `test_encode_negative` | REQ-A429-006 |
| TC-A429-006 | `test_encode_airspeed` | REQ-A429-006 |
| TC-A429-007 | `test_decode` | REQ-A429-005 |
| TC-A429-008 | `test_decode_ignores_padding` | REQ-A429-005 |
| TC-A429-009 | `test_decode_errors` | REQ-A429-007 |
| TC-A429-010 | `test_encode_range` | REQ-A429-007 |
| TC-A429-011 | `test_invalid_arguments` | REQ-A429-007 |
| TC-A429-012 | `test_round_trip` | REQ-A429-005, REQ-A429-006 |

REQ-A429-008 est vérifiée par relecture du code.

Mots de référence, calculés à part :

| Paramètre | Valeur | SDI | Mot |
|---|---|---|---|
| Altitude, label 203 | 35 000 ft | 0 | `0x6445C0C1` |
| Altitude, label 203 | 1 ft | 0 | `0xE00008C1` |
| Altitude, label 203 | -1 000 ft | 0 | `0x7FE0C0C1` |
| Vitesse, label 206 | 250,5 kt | 1 | `0x63EA0161` |

## Compilation et tests

```bash
make test
```

Il faut `gcc` (ou `clang`) et `make`. Le code est en C standard (norme C99) et
compile sans avertissement avec `-Wall -Wextra -Werror -pedantic -Wconversion
-Wshadow`.

## Limites

- Seul le format BNR est traité (pas le BCD ni les mots discrets).
- Les paramètres des labels 203 et 206 pris en exemple viennent de guides
  d'installation d'équipements consultables en ligne. Ils peuvent varier selon
  l'équipement émetteur et sont à vérifier dans la norme ARINC 429.
- Seul le contenu du mot est traité : ni la couche électrique, ni les débits.
- Ce n'est pas un logiciel certifié.

## Licence

MIT, voir [LICENSE](LICENSE).
