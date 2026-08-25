# Feu piéton intelligent + compte à rebours

Thème 4 — Mini-projet Arduino / IoT  
Master 1 Intelligence Artificielle — Dakar Institute of Technology, 2025/2026  
| Groupe | Thème | Formation | Établissement | Année |
|--------|-------|-----------|---------------|-------|
| 1 | Feu piéton intelligent | Master 1 Intelligence Artificielle | Dakar Institute of Technology | 2025/2026 |

| # | Nom | Prénom |
|---|-----|--------|
| 1 | Dehou Modeste | Kassem |
| 2 | Ndala | William Marrion Branham |
| 3 | Junior | Kra |
| 4 | Sow | Mohamed |
| 5 | Diouf | François Pape |
| 6 | Kourouma | Saibou |

Simulation : https://wokwi.com/projects/473342435632946177

---

## Contexte

Un feu de passage piéton à la demande. Les voitures circulent normalement. Quand un piéton appuie sur le bouton, le système enregistre la demande et déclenche une séquence sécurisée — orange, tout-rouge, traversée avec compte à rebours — avant de revenir au cycle normal. La durée de traversée est réglable via potentiomètre.

---

## Matériel

| Composant        | Qté | Pin  |
|------------------|-----|------|
| Arduino Uno      | 1   | —    |
| LED verte        | 2   | D2, D5 |
| LED jaune        | 1   | D3   |
| LED rouge        | 2   | D4, D6 |
| Résistance 220 Ω | 5   | en série sur chaque LED |
| Bouton poussoir  | 1   | D7   |
| Potentiomètre    | 1   | A0   |

Câblage : chaque LED est pilotée séparément avec sa résistance en série. Le bouton est câblé en INPUT_PULLUP (une borne sur D7, l'autre sur GND).

---

## Lancer le projet

**Wokwi**

1. Ouvrir https://wokwi.com/projects/473342435632946177
2. Cliquer sur ▶ Start Simulation
3. Ouvrir le Serial Monitor (9600 baud)
4. Appuyer sur le bouton bleu pour déclencher la séquence
5. Tourner le potentiomètre pour ajuster la durée de traversée (4 à 15 s)

**Arduino physique**

1. Ouvrir `feu_pieton.ino` dans l'IDE Arduino
2. Board → Arduino Uno, téléverser
3. Câbler selon le tableau ci-dessus
4. Serial Monitor à 9600 baud

---

## Logique

Cinq états. Le sujet en exige quatre ; on a ajouté un tout-rouge de sécurité avant la traversée.

```
VOITURE_VERT    voiture verte, piéton rouge   durée min. 5 s
      |
      | bouton appuyé
      v
VOITURE_ORANGE  voiture jaune, piéton rouge   2 s
      v
TOUT_ROUGE_AV   tout rouge (sécurité)         1 s
      v
PIETON_VERT     voiture rouge, piéton vert    4 – 15 s  (compte à rebours)
      v
RETOUR          tout rouge (sécurité)         1 s
      v
VOITURE_VERT    ...
```

Le bouton ne change pas les feux directement : il lève un drapeau `demande = true`. La machine à états consomme ce drapeau à la fin du vert voiture. Un appui pendant la séquence est ignoré.

---

## Robustesse

Le sujet impose deux mécanismes parmi la liste. Quatre ont été implémentés.

- **Anti-rebond** — front descendant uniquement, validé après 50 ms
- **Temporisations non bloquantes** — `millis()` partout, aucun `delay()`
- **Filtrage potentiomètre** — moyenne glissante sur 5 lectures
- **Verrouillage de la durée** — figée à l'entrée de TOUT_ROUGE_AV, non modifiable pendant la traversée

---

## Serial Monitor

Exemple de sortie lors d'un cycle complet :

```
=== FEU PIETON INTELLIGENT - DEMARRAGE ===
Etat initial : VOITURE_VERT
[ETAT] VOITURE_VERT | Temps restant : 3 s | demande=false
[BTN] >> Demande pieton enregistree (demande=true)
[ETAT] ==> VOITURE_ORANGE | demande=true
[BTN] >> Appui ignore (sequence en cours)
[ETAT] ==> TOUT_ROUGE_AVANT | demande=true
[INFO] Duree traversee fixee : 6 s (pot brut=512, filtre=510)
[ETAT] ==> PIETON_VERT | demande=true
[ETAT] PIETON_VERT | Compte a rebours : 5 s
[ETAT] PIETON_VERT | Compte a rebours : 0 s
[ETAT] ==> RETOUR | demande=true
[ETAT] ==> VOITURE_VERT | demande=false
```

---

## Tests

1. Appuyer sur le bouton → vérifier la séquence complète et le compte à rebours
2. Appuyer pendant la séquence → vérifier que le Serial affiche "Appui ignoré" et que la séquence continue normalement

---

## Fichiers

```
feu_pieton.ino    code Arduino
diagram.json      schéma Wokwi
slides.pdf        présentation
README.md         ce fichier
```
