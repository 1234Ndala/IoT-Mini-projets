

// --- Pins LEDs voiture ---
#define PIN_V_VERT   2
#define PIN_V_JAUNE  3
#define PIN_V_ROUGE  4

// --- Pins LEDs piéton ---
#define PIN_P_VERT   5
#define PIN_P_ROUGE  6

// --- Bouton piéton ---
#define PIN_BTN      7

// --- Potentiomètre (optionnel) ---
#define PIN_POT      A0

// --- Durées fixes (ms) ---
#define T_ORANGE       2000UL
#define T_TOUT_ROUGE   1000UL
#define DEBOUNCE_MS      50UL

// --- Durée piéton : lue sur le potentiomètre (4 à 10 s) ---
unsigned long T_PIETON = 6000UL;

// --- Machine à états ---
enum Etat {
  VOITURE_VERT,
  VOITURE_ORANGE,
  PIETON_VERT,
  RETOUR
};

Etat etatCourant      = VOITURE_VERT;
unsigned long tEntree = 0;       // horodatage entrée état courant

// --- Bouton ---
bool demandeActive  = false;
bool btnPrecedent   = HIGH;      // INPUT_PULLUP → repos = HIGH
unsigned long tDebounce = 0;

// --- Compte à rebours ---
int dernierAffiche  = -1;

// ============================================================
void setup() {
  Serial.begin(9600);

  pinMode(PIN_V_VERT,  OUTPUT);
  pinMode(PIN_V_JAUNE, OUTPUT);
  pinMode(PIN_V_ROUGE, OUTPUT);
  pinMode(PIN_P_VERT,  OUTPUT);
  pinMode(PIN_P_ROUGE, OUTPUT);
  pinMode(PIN_BTN,     INPUT_PULLUP);
  // PIN_POT : analogique, pas besoin de pinMode

  appliquerEtat(VOITURE_VERT);

  Serial.println("=== FEU PIETON INTELLIGENT – DEMARRAGE ===");
  Serial.println("Etat initial : VOITURE_VERT");
  Serial.println("Appuyez sur le bouton bleu pour demander le passage.");
}

// ============================================================
void loop() {
  lirePotentiometre();   // mise à jour T_PIETON
  lireBouton();          // anti-rebond + enregistrement demande
  gererEtat();           // machine à états non bloquante
}

// ============================================================
// Lecture potentiomètre → T_PIETON entre 4 s et 10 s
// ============================================================
void lirePotentiometre() {
  int val = analogRead(PIN_POT);               // 0–1023
  T_PIETON = map(val, 0, 1023, 4000, 10000);  // 4 000–10 000 ms
}

// ============================================================
// Anti-rebond non bloquant sur le bouton piéton
// ============================================================
void lireBouton() {
  bool btnActuel = digitalRead(PIN_BTN);

  // Détection front descendant (appui)
  if (btnPrecedent == HIGH && btnActuel == LOW) {
    tDebounce = millis();
  }

  // Validation après délai anti-rebond
  if (btnActuel == LOW && (millis() - tDebounce) >= DEBOUNCE_MS) {
    if (etatCourant == VOITURE_VERT && !demandeActive) {
      // Seul cas où on enregistre la demande
      demandeActive = true;
      Serial.println("[BTN] >> Demande pieton enregistree (demande=true)");
    } else if (etatCourant != VOITURE_VERT) {
      // Appui ignoré pendant la séquence — obligatoire
      Serial.println("[BTN] >> Appui ignore (sequence en cours)");
    }
  }

  btnPrecedent = btnActuel;
}

// ============================================================
// Machine à états – temporisations toutes non bloquantes
// ============================================================
void gererEtat() {
  unsigned long maintenant = millis();
  unsigned long duree      = maintenant - tEntree;

  switch (etatCourant) {

    // ----------------------------------------------------------
    case VOITURE_VERT:
      // On attend la demande ; Serial : état en continu (1/s suffit)
      if (demandeActive) {
        demandeActive = false;
        changerEtat(VOITURE_ORANGE);
      }
      break;

    // ----------------------------------------------------------
    case VOITURE_ORANGE:
      afficherSerial("VOITURE_ORANGE", (long)(T_ORANGE - duree));
      if (duree >= T_ORANGE) {
        changerEtat(PIETON_VERT);
      }
      break;

    // ----------------------------------------------------------
    case PIETON_VERT: {
      // Compte à rebours non bloquant
      long msRestants = (long)(T_PIETON - duree);
      if (msRestants < 0) msRestants = 0;
      int secRestantes = (int)(msRestants / 1000);

      // N'affiche que quand la valeur change
      if (secRestantes != dernierAffiche) {
        dernierAffiche = secRestantes;
        Serial.print("[ETAT] PIETON_VERT | Compte rebours : ");
        Serial.print(secRestantes);
        Serial.println(" s");
      }

      if (duree >= T_PIETON) {
        changerEtat(RETOUR);
      }
      break;
    }

    // ----------------------------------------------------------
    case RETOUR:
      afficherSerial("RETOUR (tout rouge)", (long)(T_TOUT_ROUGE - duree));
      if (duree >= T_TOUT_ROUGE) {
        changerEtat(VOITURE_VERT);
      }
      break;
  }
}

// ============================================================
// Changement d'état : mise à jour + LEDs + Serial
// ============================================================
void changerEtat(Etat nouvel) {
  etatCourant   = nouvel;
  tEntree       = millis();
  dernierAffiche = -1;
  appliquerEtat(nouvel);

  Serial.print("\n[ETAT] ——> ");
  Serial.print(nomEtat(nouvel));
  Serial.print(" | demande=");
  Serial.println(demandeActive ? "true" : "false");
}

// ============================================================
// Application des LEDs selon l'état courant
// ============================================================
void appliquerEtat(Etat e) {
  // Tout éteindre d'abord
  digitalWrite(PIN_V_VERT,  LOW);
  digitalWrite(PIN_V_JAUNE, LOW);
  digitalWrite(PIN_V_ROUGE, LOW);
  digitalWrite(PIN_P_VERT,  LOW);
  digitalWrite(PIN_P_ROUGE, LOW);

  switch (e) {
    case VOITURE_VERT:
      // Voiture vert ON ; piéton rouge ON
      digitalWrite(PIN_V_VERT,  HIGH);
      digitalWrite(PIN_P_ROUGE, HIGH);
      break;

    case VOITURE_ORANGE:
      // Voiture jaune ON ; piéton rouge ON
      digitalWrite(PIN_V_JAUNE, HIGH);
      digitalWrite(PIN_P_ROUGE, HIGH);
      break;

    case PIETON_VERT:
      // Voiture rouge ON ; piéton vert ON
      digitalWrite(PIN_V_ROUGE, HIGH);
      digitalWrite(PIN_P_VERT,  HIGH);
      break;

    case RETOUR:
      // Voiture rouge ON pendant 1 s ; piéton rouge ON
      digitalWrite(PIN_V_ROUGE, HIGH);
      digitalWrite(PIN_P_ROUGE, HIGH);
      break;
  }
}

// ============================================================
// Affichage Serial : état courant + temps restant + événements
// ============================================================
void afficherSerial(const char* etat, long msRestants) {
  // N'affiche que toutes les secondes pour ne pas saturer
  static unsigned long dernierLog = 0;
  if (millis() - dernierLog >= 1000) {
    dernierLog = millis();
    Serial.print("[ETAT] ");
    Serial.print(etat);
    Serial.print(" | Temps restant : ");
    Serial.print(msRestants / 1000);
    Serial.println(" s");
  }
}

// ============================================================
// Nom lisible de l'état
// ============================================================
const char* nomEtat(Etat e) {
  switch (e) {
    case VOITURE_VERT:   return "VOITURE_VERT";
    case VOITURE_ORANGE: return "VOITURE_ORANGE";
    case PIETON_VERT:    return "PIETON_VERT";
    case RETOUR:         return "RETOUR";
    default:             return "INCONNU";
  }
}
