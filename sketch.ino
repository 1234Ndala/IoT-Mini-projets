/* ============================================================
   FEU PIETON INTELLIGENT + COMPTE A REBOURS
   Groupe 1 - Theme 4 - Master 1 IA / IoT - DIT
   Arduino Uno - simulation Wokwi
   ============================================================ */

// --- Pins LEDs voiture ---
#define PIN_V_VERT   2
#define PIN_V_JAUNE  3
#define PIN_V_ROUGE  4

// --- Pins LEDs pieton ---
#define PIN_P_VERT   5
#define PIN_P_ROUGE  6

// --- Bouton pieton ---
#define PIN_BTN      7

// --- Potentiometre ---
#define PIN_POT      A0

// --- Durees fixes (ms) ---
#define T_VERT_MIN     5000UL    // (2) duree minimale du vert voiture
#define T_ORANGE       2000UL
#define T_TOUT_ROUGE   1000UL
#define DEBOUNCE_MS      50UL

// --- Duree pieton : figee a l'entree de PIETON_VERT ---
unsigned long T_PIETON = 6000UL;

// --- Machine a etats ---
enum Etat {
  VOITURE_VERT,
  VOITURE_ORANGE,
  TOUT_ROUGE_AV,        // (4) securite : le carrefour se vide
  PIETON_VERT,
  RETOUR
};

Etat etatCourant      = VOITURE_VERT;
unsigned long tEntree = 0;       // horodatage entree etat courant

// --- Bouton ---
bool demandeActive  = false;
bool btnPrecedent   = HIGH;      // INPUT_PULLUP -> repos = HIGH
unsigned long tDebounce = 0;

// --- Compte a rebours ---
int dernierAffiche  = -1;

// --- (6) Filtrage du potentiometre : moyenne des 5 dernieres lectures ---
#define N_FILTRE 5
int histoPot[N_FILTRE];
int idxPot = 0;
bool potRempli = false;

// ============================================================
void setup() {
  Serial.begin(9600);

  pinMode(PIN_V_VERT,  OUTPUT);
  pinMode(PIN_V_JAUNE, OUTPUT);
  pinMode(PIN_V_ROUGE, OUTPUT);
  pinMode(PIN_P_VERT,  OUTPUT);
  pinMode(PIN_P_ROUGE, OUTPUT);
  pinMode(PIN_BTN,     INPUT_PULLUP);

  appliquerEtat(VOITURE_VERT);
  tEntree = millis();

  Serial.println("=== FEU PIETON INTELLIGENT - DEMARRAGE ===");
  Serial.println("Etat initial : VOITURE_VERT");
  Serial.println("Appuyez sur le bouton bleu pour demander le passage.");
}

// ============================================================
void loop() {
  lireBouton();          // anti-rebond + enregistrement demande
  gererEtat();           // machine a etats non bloquante
}

// ============================================================
// (6) Moyenne glissante sur les 5 dernieres lectures du potentiometre
// ============================================================
int filtrerPot(int valeur) {
  histoPot[idxPot] = valeur;
  idxPot = (idxPot + 1) % N_FILTRE;
  if (idxPot == 0) potRempli = true;

  int nb = potRempli ? N_FILTRE : idxPot;
  long somme = 0;
  for (int i = 0; i < nb; i++) somme += histoPot[i];
  return somme / nb;
}

// ============================================================
// (1) Anti-rebond NON BLOQUANT, sur FRONT DESCENDANT uniquement
//     -> un appui maintenu ne genere qu'un seul evenement
// ============================================================
void lireBouton() {
  bool btnActuel = digitalRead(PIN_BTN);

  // On ne reagit qu'a un CHANGEMENT d'etat, espace d'au moins 50 ms
  if (btnActuel != btnPrecedent && (millis() - tDebounce) >= DEBOUNCE_MS) {
    tDebounce    = millis();
    btnPrecedent = btnActuel;

    // Front descendant = appui reel (le relachement ne fait rien)
    if (btnActuel == LOW) {
      if (etatCourant == VOITURE_VERT && !demandeActive) {
        demandeActive = true;
        Serial.println("[BTN] >> Demande pieton enregistree (demande=true)");
      } else {
        Serial.println("[BTN] >> Appui ignore (sequence en cours)");
      }
    }
  }
}

// ============================================================
// Machine a etats - temporisations toutes non bloquantes
// ============================================================
void gererEtat() {
  unsigned long duree = millis() - tEntree;

  switch (etatCourant) {

    // ----------------------------------------------------------
    case VOITURE_VERT:
      afficherSerial("VOITURE_VERT", (long)(T_VERT_MIN - duree));
      // (2) le vert termine sa duree minimale avant de ceder la place
      if (demandeActive && duree >= T_VERT_MIN) {
        changerEtat(VOITURE_ORANGE);
        // (3) demandeActive N'EST PAS remis a false ici :
        //     c'est ce qui empeche une deuxieme demande d'etre empilee.
      }
      break;

    // ----------------------------------------------------------
    case VOITURE_ORANGE:
      afficherSerial("VOITURE_ORANGE", (long)(T_ORANGE - duree));
      if (duree >= T_ORANGE) {
        changerEtat(TOUT_ROUGE_AV);
      }
      break;

    // ----------------------------------------------------------
    // (4) Tout rouge AVANT le pieton : le carrefour se vide
    case TOUT_ROUGE_AV:
      afficherSerial("TOUT_ROUGE_AVANT", (long)(T_TOUT_ROUGE - duree));
      if (duree >= T_TOUT_ROUGE) {
        // (5)(6) duree de traversee lue UNE SEULE FOIS et figee ici
        int brut   = analogRead(PIN_POT);
        int filtre = filtrerPot(brut);
        T_PIETON   = map(filtre, 0, 1023, 4000, 15000);

        Serial.print("[INFO] Duree traversee fixee : ");
        Serial.print(T_PIETON / 1000);
        Serial.print(" s (pot brut=");
        Serial.print(brut);
        Serial.print(", filtre=");
        Serial.print(filtre);
        Serial.println(")");

        changerEtat(PIETON_VERT);
      }
      break;

    // ----------------------------------------------------------
    case PIETON_VERT: {
      long msRestants = (long)T_PIETON - (long)duree;
      if (msRestants < 0) msRestants = 0;
      int secRestantes = (int)(msRestants / 1000);

      // N'affiche que quand la valeur change -> 1 ligne par seconde
      if (secRestantes != dernierAffiche) {
        dernierAffiche = secRestantes;
        Serial.print("[ETAT] PIETON_VERT | Compte a rebours : ");
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
        demandeActive = false;   // (3) remise a zero AU BON ENDROIT
        changerEtat(VOITURE_VERT);
      }
      break;
  }
}

// ============================================================
// Changement d'etat : mise a jour + LEDs + Serial
// ============================================================
void changerEtat(Etat nouvel) {
  etatCourant    = nouvel;
  tEntree        = millis();
  dernierAffiche = -1;
  appliquerEtat(nouvel);

  Serial.print("\n[ETAT] ==> ");
  Serial.print(nomEtat(nouvel));
  Serial.print(" | demande=");
  Serial.println(demandeActive ? "true" : "false");
}

// ============================================================
// Application des LEDs selon l'etat courant
// ============================================================
void appliquerEtat(Etat e) {
  // Tout eteindre d'abord : garantit qu'on ne peut JAMAIS avoir
  // voiture verte et pieton vert en meme temps.
  digitalWrite(PIN_V_VERT,  LOW);
  digitalWrite(PIN_V_JAUNE, LOW);
  digitalWrite(PIN_V_ROUGE, LOW);
  digitalWrite(PIN_P_VERT,  LOW);
  digitalWrite(PIN_P_ROUGE, LOW);

  switch (e) {
    case VOITURE_VERT:                       // voiture vert ; pieton rouge
      digitalWrite(PIN_V_VERT,  HIGH);
      digitalWrite(PIN_P_ROUGE, HIGH);
      break;

    case VOITURE_ORANGE:                     // voiture jaune ; pieton rouge
      digitalWrite(PIN_V_JAUNE, HIGH);
      digitalWrite(PIN_P_ROUGE, HIGH);
      break;

    case TOUT_ROUGE_AV:                      // tout rouge (securite)
      digitalWrite(PIN_V_ROUGE, HIGH);
      digitalWrite(PIN_P_ROUGE, HIGH);
      break;

    case PIETON_VERT:                        // voiture rouge ; pieton vert
      digitalWrite(PIN_V_ROUGE, HIGH);
      digitalWrite(PIN_P_VERT,  HIGH);
      break;

    case RETOUR:                             // tout rouge (securite)
      digitalWrite(PIN_V_ROUGE, HIGH);
      digitalWrite(PIN_P_ROUGE, HIGH);
      break;
  }
}

// ============================================================
// Affichage Serial : etat courant + temps restant
// ============================================================
void afficherSerial(const char* etat, long msRestants) {
  static unsigned long dernierLog = 0;
  if (millis() - dernierLog >= 1000) {
    dernierLog = millis();
    if (msRestants < 0) msRestants = 0;
    Serial.print("[ETAT] ");
    Serial.print(etat);
    Serial.print(" | Temps restant : ");
    Serial.print(msRestants / 1000);
    Serial.print(" s | demande=");
    Serial.println(demandeActive ? "true" : "false");
  }
}

// ============================================================
// Nom lisible de l'etat
// ============================================================
const char* nomEtat(Etat e) {
  switch (e) {
    case VOITURE_VERT:   return "VOITURE_VERT";
    case VOITURE_ORANGE: return "VOITURE_ORANGE";
    case TOUT_ROUGE_AV:  return "TOUT_ROUGE_AVANT";
    case PIETON_VERT:    return "PIETON_VERT";
    case RETOUR:         return "RETOUR (tout rouge)";
    default:             return "INCONNU";
  }
}
