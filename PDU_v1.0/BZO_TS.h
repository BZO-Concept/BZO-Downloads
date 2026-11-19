/********************************************************************************************
 *
 *  BZO_TS.h  -  Base de communication TunerStudio pour cartes BZO Concept
 *  Marche avec tout logiciel à fichier .ini : TunerStudio, MSDroid, Shadow Dash...
 *  en USB, série ou Bluetooth HC-05 (même ligne série).
 *
 *  Processeurs : AVR (Nano, 328PB), megaTinyCore, DxCore (AVR32DD...),
 *                STM32 core officiel (F042, F103, F405...).
 *
 *  Contact : www.bzo-concept.fr
 *  v1.4 05/11/26
 *
 *******************************************************************************************
 *
 *  MODE D'EMPLOI (tout est là, le reste du fichier n'est pas à lire pour s'en servir)
 *  ----------------------------------------------------------------------------------
 *
 *  1) Copier BZO_TS.h dans le dossier du croquis.
 *
 *  2) En haut du .ino :
 *
 *       #define TS_SIGNATURE "Speeduino Ma carte v1.0"   // = "signature" du .ini (TS : commence par "Speeduino")
 *       //#define TS_PROTOCOLE_CRC             // à décommenter sur les gros MCU (F103, F405)
 *       #include "BZO_TS.h"
 *
 *  3) Dans un globals.h (inclus juste après BZO_TS.h), comme dans Speeduino :
 *     une structure "packed" par page du .ini, currentStatus libre, et la liste du log TS :
 *
 *       struct __attribute__((packed)) config1 { uint8_t mode; uint16_t seuil; };  // page 1
 *       struct statuses { uint8_t secl; uint8_t statutTS; uint8_t erreursTS; uint16_t boucles; uint16_t RPM; };
 *       struct config1  configPage1;
 *       struct statuses currentStatus;
 *
 *       uint8_t getTSLogEntry(uint16_t offset)        // ce qu'on envoie à TS, octet par octet
 *       {
 *         switch (offset)
 *         {
 *           case 0: return currentStatus.secl;
 *           case 1: return currentStatus.statutTS;
 *           case 2: return currentStatus.erreursTS;
 *           case 4: return lowByte(currentStatus.boucles);
 *           case 5: return highByte(currentStatus.boucles);
 *           case 6: return lowByte(currentStatus.RPM);
 *           case 7: return highByte(currentStatus.RPM);
 *         }
 *         return 0;
 *       }
 *
 *     Offsets 0 à 5 = "bloc standard BZO_TS" de [OutputChannels] du .ini : secondes, statut,
 *     erreurs de com, tours de loop/s. secl, statutTS, erreursTS et boucles sont tenus à
 *     jour tout seuls par la librairie (currentStatus en 3e paramètre de setOutputs).
 *
 *  4) Facultatif, les valeurs par défaut dans une fonction (sinon : base tune .msq dans TS) :
 *
 *       void ValeursParDefaut() { configPage1.mode = 1; configPage1.seuil = 3000; }
 *
 *  5) Dans setup() :
 *
 *       TS.addPage(1, configPage1);                    // page 1 du .ini
 *       TS.setOutputs(getTSLogEntry, 8, currentStatus); // 8 = ochBlockSize du .ini
 *       TS.addCommand(0x0101, MaFonction);             // bouton "E\x01\x01" -> void MaFonction()
 *       TS.begin(Serial, 115200);                      // EN DERNIER (ValeursParDefaut en 3e si besoin)
 *
 *  6) Dans loop() :
 *
 *       TS.update();                       // à chaque tour, le plus souvent possible
 *       if (configPage1.mode == 1) { ... }   // les réglages se lisent directement
 *       currentStatus.RPM = ...;             // les données temps réel s'écrivent directement
 *
 *  STATUTS DISPONIBLES (à lire quand on veut)
 *       TS.connected()        TS est connecté (commande reçue il y a moins de 2 s)
 *       TS.pageChanged(1)     TS vient de modifier la page 1 (vrai une seule fois)
 *       TS.anyPageChanged()   TS vient de modifier une page (vrai une seule fois)
 *       TS.memoryOK()         toutes les pages ont été relues correctement au démarrage
 *       TS.memoryFits()       les pages tiennent dans l'EEPROM (sinon : rien n'est sauvegardé)
 *       TS.ramOK()            toutes les pages et commandes ont trouvé leur place en RAM
 *       TS.savePending()      un burn attend d'être écrit en mémoire (0,5 s)
 *       TS.crcMode()          la dernière commande est arrivée en protocole CRC
 *
 *  OUTILS
 *       TS.curve(axeX, valeurs, x)              lit une courbe du .ini (interpolée)
 *       TS.table(axeX, axeY, table, x, y)       lit une table 3D du .ini (interpolée)
 *       TS.save(1) / TS.saveAll()               sauvegarde depuis votre code
 *       TS.restoreDefaults()                    valeurs par défaut + sauvegarde
 *
 *  BOUTONS INTÉGRÉS (rien à écrire dans le croquis, juste dans le .ini)
 *       "E\xFF\x01"  remise à zéro des compteurs d'erreurs
 *       "E\xFF\x02"  retour aux valeurs par défaut + sauvegarde
 *
 *  À RETENIR
 *    - Pas de limite au nombre de pages ni de commandes. Si l'EEPROM ou la RAM manque,
 *      le texte de version affiché par TS se termine par "ERREUR : ..." (et le statut
 *      offset 1 a son bit3 = EEPROM trop petite, bit4 = RAM insuffisante).
 *    - Les offsets et tailles du .ini doivent correspondre octet par octet aux structures.
 *    - Changement d'une structure de page -> incrémenter TS_EEPROM_VERSION (la mémoire
 *      est alors ignorée une fois et les valeurs par défaut reprises).
 *    - Protocole : "Léger" ou "CRC" dans les propriétés du projet TS, comme le croquis.
 *      MSDroid : toujours "Léger" (le croquis CRC accepte aussi le léger).
 *
 *******************************************************************************************/

#ifndef BZO_TS_H
#define BZO_TS_H

#include <Arduino.h>
#include <stdlib.h>                    // realloc (liste des commandes)

/*==========================================================================================
 *  RÉGLAGES AVANCÉS (facultatifs)
 *  Chacun peut être redéfini par un #define dans le .ino AVANT le #include "BZO_TS.h".
 *  Sinon c'est la valeur ci-dessous qui s'applique : en général on n'y touche pas.
 *==========================================================================================*/

// Signature renvoyée à la commande 'Q'. Doit être IDENTIQUE au "signature = ..." du .ini.
// La librairie accepte n'importe quel texte. MAIS TunerStudio (édition MS Lite) refuse de
// se connecter à un firmware qu'il ne connaît pas ("Unsupported Controller Firmware") :
// la signature doit commencer par "Speeduino" pour que TS accepte la connexion.
// MSDroid / Shadow Dash : pas de restriction connue.
#ifndef TS_SIGNATURE
  #define TS_SIGNATURE "Speeduino BZO Base TS v1.0"
#endif

// Texte renvoyé à la commande 'S' (affiché par TS comme nom/version du firmware)
#ifndef TS_VERSION_INFO
  #define TS_VERSION_INFO TS_SIGNATURE
#endif

// #define TS_PROTOCOLE_CRC     // à mettre dans le .ino pour activer le protocole CRC

// Taille maxi d'un bloc écrit par TS en une fois.
// DOIT être égal au "blockingFactor" du .ini. 64 passe partout (buffer série AVR = 64).
#ifndef TS_BLOCKING_FACTOR
  #define TS_BLOCKING_FACTOR 64
#endif

// Temps maxi entre deux octets d'une même commande avant abandon (ms).
// 100 ms suffit même à 9600 bauds (HC-05 d'origine).
#ifndef TS_TIMEOUT_OCTET
  #define TS_TIMEOUT_OCTET 100
#endif

// Sans commande valide pendant ce temps, on considère TS déconnecté (ms).
// TS interroge les données temps réel en continu, donc 2 s est large.
#ifndef TS_TIMEOUT_CONNEXION
  #define TS_TIMEOUT_CONNEXION 2000
#endif

// Délai entre le dernier "burn" de TS et l'écriture réelle en mémoire (ms).
// TS burn souvent plusieurs pages d'affilée : on attend qu'il ait fini pour écrire
// une seule fois (indispensable sur STM32 : un effacement de flash par sauvegarde).
#ifndef TS_DELAI_SAUVEGARDE
  #define TS_DELAI_SAUVEGARDE 500
#endif

// Adresse EEPROM où commence la zone TS (si vous gardez le début pour vous)
#ifndef TS_EEPROM_DEBUT
  #define TS_EEPROM_DEBUT 0
#endif

// Version de la disposition mémoire. À INCRÉMENTER quand vous changez une structure de
// page : la mémoire sera alors ignorée au démarrage et les valeurs par défaut reprises.
#ifndef TS_EEPROM_VERSION
  #define TS_EEPROM_VERSION 1
#endif

// 1 = si une page n'est pas valide en mémoire au démarrage (carte neuve, version changée,
//     mémoire corrompue), ses valeurs par défaut sont écrites tout de suite en mémoire.
// 0 = on laisse la mémoire telle quelle (rien n'est écrit tant que TS ne fait pas de burn
//     ou que vous n'appelez pas TS.saveAll()).
#ifndef TS_AUTO_DEFAUTS
  #define TS_AUTO_DEFAUTS 1
#endif

/*==========================================================================================
 *  MÉMOIRE (EEPROM réelle ou flash émulée)
 *  ts_memDebut()  : à appeler avant une série de lectures / écritures
 *  ts_memLire()   : lit un octet
 *  ts_memEcrire() : écrit un octet (renvoie true si l'octet a vraiment changé)
 *  ts_memFin()    : termine une série d'écritures (sur STM32 : écrit la flash)
 *
 *  Pour une autre mémoire (FRAM, flash SPI, backup SRAM...) : mettre
 *  #define TS_STOCKAGE_UTILISATEUR dans le .ino et écrire ces 4 fonctions + TS_TAILLE_MEMOIRE.
 *==========================================================================================*/

#if defined(TS_STOCKAGE_UTILISATEUR)
  // Fonctions fournies par le croquis
  void    ts_memDebut();
  uint8_t ts_memLire(uint16_t adresse);
  bool    ts_memEcrire(uint16_t adresse, uint8_t valeur);
  void    ts_memFin(bool modifie);
  #ifndef TS_TAILLE_MEMOIRE
    #error "TS_STOCKAGE_UTILISATEUR : definir aussi TS_TAILLE_MEMOIRE (taille en octets)"
  #endif

#elif defined(ARDUINO_ARCH_STM32) && !defined(DATA_EEPROM_BASE)
  // STM32 sans vraie EEPROM (F0, F1, F4...) : flash émulée par le core.
  // On passe par le buffer RAM du core : on charge la page flash une fois, on modifie
  // en RAM, puis UN SEUL effacement + écriture à la fin (EEPROM.write() efface la
  // flash à CHAQUE octet, à éviter).
  // Attention F405 : la "page" est un secteur de 128 Ko -> l'écriture bloque ~1 à 2 s.
  #include <EEPROM.h>
  #define TS_TAILLE_MEMOIRE ((uint32_t)EEPROM.length())
  static inline void    ts_memDebut()                { eeprom_buffer_fill(); }
  static inline uint8_t ts_memLire(uint16_t a)       { return eeprom_buffered_read_byte(a); }
  static inline bool    ts_memEcrire(uint16_t a, uint8_t v)
  {
    if (eeprom_buffered_read_byte(a) == v) { return false; }   // déjà bon, rien à faire
    eeprom_buffered_write_byte(a, v);
    return true;
  }
  static inline void    ts_memFin(bool modifie)      { if (modifie) { eeprom_buffer_flush(); } }

#else
  // AVR (Nano, 328PB), megaTinyCore, DxCore, et STM32 avec vraie EEPROM (L0/L1)
  #include <EEPROM.h>
  #define TS_TAILLE_MEMOIRE ((uint32_t)EEPROM.length())   // (E2END est faux sur tinyAVR/DxCore)
  static inline void    ts_memDebut()                { }
  static inline uint8_t ts_memLire(uint16_t a)       { return EEPROM.read(a); }
  static inline bool    ts_memEcrire(uint16_t a, uint8_t v)
  {
    if (EEPROM.read(a) == v) { return false; }                 // évite d'user l'EEPROM
    EEPROM.write(a, v);
    return true;
  }
  static inline void    ts_memFin(bool modifie)      { (void)modifie; }
#endif

/*==========================================================================================
 *  CONSTANTES DU PROTOCOLE (ne pas toucher)
 *==========================================================================================*/

#define TS_RC_OK         0x00   // réponse OK
#define TS_RC_BURN_OK    0x04   // sauvegarde OK
#define TS_RC_TIMEOUT    0x80   // commande incomplète
#define TS_RC_CRC_ERR    0x82   // CRC faux
#define TS_RC_INCONNU    0x83   // commande inconnue
#define TS_RC_HORS_LIMITE 0x84  // page / offset / longueur hors limites (TS ne réessaie pas)

#define TS_PAGE_SORTIES  0x30   // "page" des données temps réel dans la commande 'r'
#define TS_MAGIC         0xB2   // 1er octet de l'en-tête mémoire

// Taille du buffer de réception : 7 octets d'en-tête de commande 'M' + 1 bloc de données
#define TS_TAILLE_BUFFER (TS_BLOCKING_FACTOR + 8)

// Version de protocole renvoyée à 'F'
#ifdef TS_PROTOCOLE_CRC
  #define TS_VERSION_PROTOCOLE "002"
#else
  #define TS_VERSION_PROTOCOLE "001"
#endif

// Fonction appelée par un bouton : soit  void MaFonction()  (le plus simple),
// soit  void MaFonction(uint16_t numero)  (numero = 0x0105 pour "E\x01\x05", utile si une
// même fonction sert à plusieurs boutons)
typedef void (*TS_Fonction)();
typedef void (*TS_FonctionCommande)(uint16_t numero);

// Fonction "log TS" (comme getTSLogEntry de Speeduino) : renvoie l'octet à envoyer à TS pour
// un offset donné du bloc [OutputChannels]. Voir TS.setOutputs(getTSLogEntry, taille).
typedef uint8_t (*TS_FonctionLog)(uint16_t offset);

// Commandes directes intégrées (groupe 0xFF réservé à la librairie)
#define TS_CMD_RAZ_ERREURS   0xFF01   // "E\xFF\x01" : remet les compteurs d'erreurs à zéro
#define TS_CMD_DEFAUTS       0xFF02   // "E\xFF\x02" : valeurs par défaut + sauvegarde

/*==========================================================================================
 *  CRC32 standard (le même que TS) - calcul bit à bit, aucune table : 0 octet de RAM
 *==========================================================================================*/
static inline uint32_t ts_crc32Octet(uint32_t crc, uint8_t octet)
{
  crc ^= octet;
  for (uint8_t i = 0; i < 8; i++)
  {
    crc = (crc >> 1) ^ (0xEDB88320UL & (0UL - (crc & 1UL)));
  }
  return crc;
}

static inline uint32_t ts_crc32(const uint8_t *donnees, uint16_t taille)
{
  uint32_t crc = 0xFFFFFFFFUL;
  for (uint16_t i = 0; i < taille; i++) { crc = ts_crc32Octet(crc, donnees[i]); }
  return ~crc;
}

/*==========================================================================================
 *  LA CLASSE  (un seul objet, déjà créé en bas du fichier : TS)
 *==========================================================================================*/
class BZO_TS
{
public:

  /*========================================================================================
   *  1) MISE EN PLACE  -  dans setup(), dans cet ordre
   *========================================================================================*/

  // Déclare une page de réglages.
  //   numeroPage : le n° de page du .ini (page = 1, page = 2...)
  //   page       : votre structure (déclarée avec __attribute__((packed)))
  // Exemple : TS.addPage(1, configPage1);
  // Les pages sont rangées en mémoire dans l'ordre des addPage() : ne pas changer cet
  // ordre sans incrémenter TS_EEPROM_VERSION. Pas de limite de nombre (false si plus de RAM).
  template <typename T> bool addPage(uint8_t numeroPage, T &page)
  {
    return addPage(numeroPage, (void *)&page, (uint16_t)sizeof(T));
  }
  // Interdit : TS.addPage(1, &configPage1) (sans taille) donnerait la taille d'un pointeur.
  // Écrire TS.addPage(1, configPage1) sans le &.
  template <typename T> bool addPage(uint8_t numeroPage, T *page) = delete;

  // Même chose avec adresse + taille (pour un tableau d'octets par exemple)
  bool addPage(uint8_t numeroPage, void *donnees, uint16_t taille)
  {
    Page *nouvelle = (Page *)realloc(_pages, (_nbPages + 1) * sizeof(Page));
    if (nouvelle == 0) { _erreurRAM = true; return false; }   // plus de RAM
    _pages = nouvelle;
    _pages[_nbPages].numero = numeroPage;
    _pages[_nbPages].donnees = (uint8_t *)donnees;
    _pages[_nbPages].taille = taille;
    _pages[_nbPages].modifiee = false;
    _pages[_nbPages].valide = false;
    _pages[_nbPages].relue = false;
    _pages[_nbPages].aSauver = false;
    _nbPages++;
    return true;
  }

  // Déclare le temps réel, façon Speeduino : c'est VOTRE fonction qui dit quel octet
  // envoyer pour chaque offset du bloc [OutputChannels] du .ini.
  //   TS.setOutputs(getTSLogEntry, 16);   // 16 = ochBlockSize du .ini
  //   uint8_t getTSLogEntry(uint16_t offset) { switch (offset) { case 6: return ...; } }
  // Tous les octets, du 0 au dernier, viennent de votre fonction.
  // currentStatus en 3e : la librairie tient à jour, en fond (à chaque TS.update()),
  // ses champs secl, statutTS, erreursTS et boucles (à déclarer dans struct statuses).
  //   TS.setOutputs(getTSLogEntry, TS_LOG_TAILLE, currentStatus);
  void setOutputs(TS_FonctionLog fonction, uint16_t taille)
  {
    _fonctionLog = fonction;
    _tailleSorties = taille;
    _majStatus = 0;
  }

  template <typename T> void setOutputs(TS_FonctionLog fonction, uint16_t taille, T &status)
  {
    setOutputs(fonction, taille);
    _status = &status;
    _majStatus = [](void *p, BZO_TS *ts)
    {
      T *s = (T *)p;
      s->secl      = ts->secl();
      s->statutTS  = ts->statusByte();
      s->erreursTS = ts->errorCount();
      s->boucles   = ts->loopsPerSecond();
    };
  }

  // Associe une fonction à un bouton de commande directe du .ini.
  // Bouton "E\x01\x05"  ->  TS.addCommand(1, 5, MaFonction);
  // La fonction s'écrit :  void MaFonction(uint16_t numero) { ... }
  // (numero vaut 0x0105 ici, utile si une même fonction sert à plusieurs boutons)
  // Pas de limite de nombre : la liste s'agrandit toute seule (renvoie false si plus de RAM).
  bool addCommand(uint8_t groupe, uint8_t numero, TS_Fonction fonction)
  {
    return _ajouterCommande((uint16_t)(((uint16_t)groupe << 8) | numero), fonction, 0);
  }
  bool addCommand(uint8_t groupe, uint8_t numero, TS_FonctionCommande fonction)
  {
    return _ajouterCommande((uint16_t)(((uint16_t)groupe << 8) | numero), 0, fonction);
  }

  // Même chose avec le numéro complet : "E\x01\x05" -> TS.addCommand(0x0105, MaFonction);
  bool addCommand(uint16_t numero, TS_Fonction fonction)         { return _ajouterCommande(numero, fonction, 0); }
  bool addCommand(uint16_t numero, TS_FonctionCommande fonction) { return _ajouterCommande(numero, 0, fonction); }

  // Facultatif : fonction qui reçoit tous les boutons NON déclarés avec addCommand()
  // (si vous préférez tout traiter dans un seul switch(numero))
  void onCommand(TS_FonctionCommande fonction) { _commandeParDefaut = fonction; }

  // Démarre la com. À appeler EN DERNIER (après les addPage) : relit la mémoire et
  // remplace les valeurs par défaut des pages par celles sauvegardées.
  // Exemple : TS.begin(Serial, 115200);   ouvre aussi le port à la vitesse donnée
  //
  // Avec la fonction des valeurs par défaut (conseillé) :
  //   TS.begin(Serial, 115200, ValeursParDefaut);
  // La librairie l'appelle elle-même AVANT de relire la mémoire (donc les valeurs
  // sauvegardées gagnent), et sur le bouton "E\xFF\x02" (retour aux valeurs par défaut).
  template <typename T> void begin(T &port, unsigned long vitesse, TS_Fonction valeursParDefaut = 0)
  {
    port.begin(vitesse);
    begin(port, valeursParDefaut);
  }

  // Même chose si le port est déjà ouvert : Serial.begin(115200); TS.begin(Serial);
  void begin(Stream &port, TS_Fonction valeursParDefaut = 0)
  {
    _port = &port;
    _defauts = valeursParDefaut;
    if (_defauts != 0) { _defauts(); }
    _chargerMemoire();
    _debutSeconde = millis();
  }

  // Remet les valeurs par défaut et les sauvegarde (même chose que le bouton "E\xFF\x02")
  void restoreDefaults()
  {
    if (_defauts == 0) { return; }
    _defauts();
    for (uint16_t i = 0; i < _nbPages; i++) { _pages[i].modifiee = true; }
    saveAll();
  }

  /*========================================================================================
   *  2) FONCTIONNEMENT  -  dans loop()
   *========================================================================================*/

  // À appeler à CHAQUE tour de loop. N'attend jamais des octets : il traite ce qui est
  // arrivé et rend la main. Seules les réponses à TS sont envoyées d'un bloc.
  // Votre loop doit tourner assez vite pour que le buffer série ne déborde pas :
  // sur AVR, 64 octets à 115200 bauds arrivent en 5,5 ms.
  void update()
  {
    if (_port == 0) { return; }

    // Compteurs de l'en-tête standard : tours de loop par seconde et secondes
    _compteurBoucles++;
    if ((millis() - _debutSeconde) >= 1000)
    {
      _debutSeconde += 1000;
      _bouclesParSeconde = _compteurBoucles;
      _compteurBoucles = 0;
      _secondes++;
    }
    if (_majStatus != 0) { _majStatus(_status, this); }   // secl, statutTS, erreursTS, boucles

    // Commande commencée mais plus rien depuis TS_TIMEOUT_OCTET : on abandonne
    if ((_etat != ETAT_ATTENTE) && ((millis() - _dernierOctet) > TS_TIMEOUT_OCTET))
    {
      timeoutErrors++;
      if (_etat >= ETAT_CRC_LONGUEUR) { _envoyerCode(TS_RC_TIMEOUT); }
      _etat = ETAT_ATTENTE;
    }

    // Lecture de tout ce qui est arrivé
    while (_port->available() > 0)
    {
      uint8_t c = (uint8_t)_port->read();
      _dernierOctet = millis();
      _recevoir(c);
    }

    // Sauvegarde différée après un burn de TS (rien en cours de réception)
    if (_sauvegardeEnAttente && (_etat == ETAT_ATTENTE) &&
        ((millis() - _dernierBurn) > TS_DELAI_SAUVEGARDE))
    {
      saveNow();
    }

#ifdef TS_PROTOCOLE_CRC
    // Après un silence, on ré-autorise le legacy (ex. on passe du PC à MSDroid)
    if (_legacyBloque && ((millis() - _dernierValide) > TS_TIMEOUT_CONNEXION))
    {
      _legacyBloque = false;
    }
#endif
  }

  /*========================================================================================
   *  3) STATUTS  -  à lire quand on veut dans son code
   *========================================================================================*/

  // TS (ou MSDroid...) a envoyé une commande valide il y a moins de TS_TIMEOUT_CONNEXION ms
  bool connected()
  {
    return _dejaConnecte && ((millis() - _dernierValide) < TS_TIMEOUT_CONNEXION);
  }

  // Vrai UNE fois après que TS a modifié la page (le drapeau retombe à la lecture).
  // Pratique pour recalculer quelque chose quand un réglage change.
  bool pageChanged(uint8_t numeroPage)
  {
    int16_t i = _indexPage(numeroPage);
    if (i < 0) { return false; }
    bool r = _pages[i].modifiee;
    _pages[i].modifiee = false;
    return r;
  }

  // Vrai UNE fois si n'importe quelle page a été modifiée par TS
  bool anyPageChanged()
  {
    bool r = false;
    for (uint16_t i = 0; i < _nbPages; i++) { r |= _pages[i].modifiee; _pages[i].modifiee = false; }
    return r;
  }

  // La page a été relue correctement en mémoire au démarrage (sinon : valeurs par défaut)
  bool memoryOK(uint8_t numeroPage)
  {
    int16_t i = _indexPage(numeroPage);
    if (i < 0) { return false; }
    return _pages[i].valide;
  }

  // Toutes les pages ont été relues correctement au démarrage
  bool memoryOK()
  {
    if (!_memoireTient) { return false; }
    for (uint16_t i = 0; i < _nbPages; i++) { if (!_pages[i].valide) { return false; } }
    return true;
  }

  // Les pages tiennent dans la mémoire (sinon rien n'est jamais sauvegardé)
  bool memoryFits() { return _memoireTient; }

  // Toutes les pages et commandes ont trouvé leur place en RAM (sinon addPage/addCommand
  // ont refusé : voir aussi le texte de version dans TS)
  bool ramOK() { return !_erreurRAM; }

  // Valeurs de com à mettre dans currentStatus (offsets 0 à 5 du .ini, "bloc standard BZO_TS")
  uint8_t  secl() { return _secondes; }                    // secondes depuis le démarrage (0-255)
  uint8_t  statusByte()                                    // bit0 mémoire OK, bit1 sauvegarde en attente,
  {                                                        // bit2 protocole CRC, bit3 mémoire trop petite,
                                                           // bit4 RAM insuffisante
    return (memoryOK() ? 0x01 : 0) | (_sauvegardeEnAttente ? 0x02 : 0) |
           (_modeCRC ? 0x04 : 0)   | (_memoireTient ? 0 : 0x08) |
           (_erreurRAM ? 0x10 : 0);
  }
  uint8_t  errorCount()                                    // erreurs de com (bloqué à 255)
  {
    uint32_t err = (uint32_t)crcErrors + timeoutErrors + rangeErrors + unknownCommands;
    return (err > 255) ? 255 : (uint8_t)err;
  }
  uint16_t loopsPerSecond()                                // tours de loop par seconde
  {
    return (_bouclesParSeconde > 65535UL) ? 65535 : (uint16_t)_bouclesParSeconde;
  }

  // Un burn attend d'être écrit en mémoire (délai TS_DELAI_SAUVEGARDE)
  bool savePending() { return _sauvegardeEnAttente; }

  // La dernière commande valide est arrivée en protocole CRC (sinon : léger / legacy)
  bool crcMode() { return _modeCRC; }

  // Une commande est en cours de réception (des octets sont arrivés, pas encore complète)
  bool receiving() { return _etat != ETAT_ATTENTE; }

  // Dernière commande reçue ('Q', 'r', 'M', 'b'...)
  char lastCommand() { return _derniereCommande; }

  // Compteurs d'erreurs (remis à zéro par votre code si besoin : TS.crcErrors = 0;)
  uint16_t crcErrors = 0;       // trames CRC fausses
  uint16_t timeoutErrors = 0;   // commandes incomplètes abandonnées
  uint16_t unknownCommands = 0; // commandes inconnues
  uint16_t rangeErrors = 0;     // page / offset / longueur hors limites

  /*========================================================================================
   *  4) SAUVEGARDE  -  si votre code modifie lui-même un réglage
   *========================================================================================*/

  // Demande la sauvegarde d'une page (écrite après TS_DELAI_SAUVEGARDE, comme un burn TS)
  void save(uint8_t numeroPage)
  {
    int16_t i = _indexPage(numeroPage);
    if (i < 0) { return; }
    _pages[i].aSauver = true;
    _sauvegardeEnAttente = true;
    _dernierBurn = millis();
  }

  // Sauvegarde toutes les pages tout de suite (ex. après remise aux valeurs par défaut)
  void saveAll()
  {
    for (uint16_t i = 0; i < _nbPages; i++) { _pages[i].aSauver = true; }
    saveNow();
  }

  // Première adresse mémoire libre après la zone TS (pour vos propres sauvegardes)
  uint16_t memoryEnd() { return _finMemoire; }

  // Écrit tout de suite en mémoire les pages en attente (appelé tout seul par update)
  void saveNow()
  {
    _sauvegardeEnAttente = false;
    bool aFaire = false;
    for (uint16_t i = 0; i < _nbPages; i++) { aFaire |= _pages[i].aSauver; }
    if (!_memoireTient || !aFaire) { for (uint16_t i = 0; i < _nbPages; i++) { _pages[i].aSauver = false; } return; }

    ts_memDebut();
    bool modifie = false;
    uint16_t adresse = TS_EEPROM_DEBUT;

    // En-tête : octet magique + version de disposition
    modifie |= ts_memEcrire(adresse++, TS_MAGIC);
    modifie |= ts_memEcrire(adresse++, TS_EEPROM_VERSION);

    for (uint16_t i = 0; i < _nbPages; i++)
    {
      if (_pages[i].aSauver)
      {
        // Données de la page
        for (uint16_t j = 0; j < _pages[i].taille; j++)
        {
          modifie |= ts_memEcrire(adresse + j, _pages[i].donnees[j]);
        }
        // CRC32 de la page juste derrière (contrôlé au démarrage)
        uint32_t crc = ts_crc32(_pages[i].donnees, _pages[i].taille);
        for (uint8_t k = 0; k < 4; k++)
        {
          modifie |= ts_memEcrire(adresse + _pages[i].taille + k, (uint8_t)(crc >> (8 * k)));
        }
        _pages[i].valide = true;
        _pages[i].aSauver = false;
      }
      adresse += _pages[i].taille + 4;
    }

    ts_memFin(modifie);
  }

  /*========================================================================================
   *  5) OUTILS  -  lecture des courbes et tables du .ini, avec interpolation
   *  Les tableaux se passent tels quels (la librairie connaît leur taille).
   *  Les axes doivent être croissants. En dehors des axes : valeur du bord.
   *  Les valeurs sont brutes (celles stockées dans la page, sans le scale du .ini).
   *========================================================================================*/

  // Courbe 2D : TS.curve(configPage2.courbeX, configPage2.courbeY, x)
  template <typename TX, typename TY, size_t N>
  static int32_t curve(const TX (&axeX)[N], const TY (&valeurs)[N], int32_t x)
  {
    uint8_t i; int32_t f;
    _position(axeX, N, x, i, f);
    int32_t y0 = valeurs[i];
    int32_t y1 = valeurs[((size_t)i + 1 < N) ? i + 1 : i];
    return y0 + (((y1 - y0) * f) / 256);
  }

  // Table 3D : TS.table(configPage2.axeX, configPage2.axeY, configPage2.table, x, y)
  // la table est déclarée table[taille de l'axe Y][taille de l'axe X] (comme TS)
  template <typename TX, typename TY, typename TZ, size_t NX, size_t NY>
  static int32_t table(const TX (&axeX)[NX], const TY (&axeY)[NY], const TZ (&valeurs)[NY][NX],
                       int32_t x, int32_t y)
  {
    uint8_t ix, iy; int32_t fx, fy;
    _position(axeX, NX, x, ix, fx);
    _position(axeY, NY, y, iy, fy);
    uint8_t ix1 = ((size_t)ix + 1 < NX) ? ix + 1 : ix;
    uint8_t iy1 = ((size_t)iy + 1 < NY) ? iy + 1 : iy;
    int32_t bas  = (int32_t)valeurs[iy][ix]  + ((((int32_t)valeurs[iy][ix1]  - valeurs[iy][ix])  * fx) / 256);
    int32_t haut = (int32_t)valeurs[iy1][ix] + ((((int32_t)valeurs[iy1][ix1] - valeurs[iy1][ix]) * fx) / 256);
    return bas + (((haut - bas) * fy) / 256);
  }

  /*========================================================================================
   *  PARTIE INTERNE  -  rien à modifier ni à appeler en dessous
   *========================================================================================*/
private:

  /*----------------------------------------------------------------------------------------
   *  Données internes
   *----------------------------------------------------------------------------------------*/
  struct Page { uint8_t numero; uint8_t *donnees; uint16_t taille;
                bool modifiee; bool valide; bool relue; bool aSauver; };   // drapeaux par page
  struct Commande { uint16_t numero; TS_Fonction simple; TS_FonctionCommande fonction; };

  enum
  {
    ETAT_ATTENTE = 0,       // rien en cours
    ETAT_LEGACY,            // commande legacy en cours de réception
    ETAT_LEGACY_IGNORE,     // écriture legacy trop grosse : on laisse passer les octets
    ETAT_CRC_LONGUEUR,      // trame CRC : 2e octet de longueur attendu
    ETAT_CRC_DONNEES,       // trame CRC : réception des données
    ETAT_CRC_CRC,           // trame CRC : réception des 4 octets de CRC
    ETAT_CRC_IGNORE         // trame CRC trop grosse : on laisse passer, puis erreur
  };

  Stream  *_port = 0;
  Page    *_pages = 0;                  // liste agrandie à chaque addPage()
  uint16_t _nbPages = 0;
  Commande *_commandes = 0;             // liste agrandie au fur et à mesure (par blocs de 8)
  uint16_t _nbCommandes = 0;
  uint16_t _placeCommandes = 0;
  TS_FonctionCommande _commandeParDefaut = 0;

  uint16_t _tailleSorties = 0;
  void    *_status = 0;                                  // currentStatus (ou 0)
  void   (*_majStatus)(void *, BZO_TS *) = 0;            // mise à jour secl, statutTS...
  TS_FonctionLog _fonctionLog = 0;      // fonction "log TS" (getTSLogEntry)
  TS_Fonction _defauts = 0;             // fonction des valeurs par défaut (ou 0)
  uint32_t _compteurBoucles = 0;
  uint32_t _bouclesParSeconde = 0;
  uint32_t _debutSeconde = 0;
  uint8_t  _secondes = 0;

  uint8_t  _buffer[TS_TAILLE_BUFFER];   // commande en cours de réception
  uint16_t _nbRecus = 0;                // octets déjà dans le buffer
  uint16_t _longueur = 0;               // longueur attendue (trame CRC ou données 'M')
  uint32_t _aIgnorer = 0;               // octets à laisser passer
  uint32_t _crcRecu = 0;
  uint8_t  _nbOctetsCRC = 0;
  uint8_t  _etat = ETAT_ATTENTE;

  bool     _enveloppe = false;          // réponse en cours au format CRC
  uint32_t _crcEnvoi = 0;

  uint32_t _dernierOctet = 0;
  uint32_t _dernierValide = 0;
  uint32_t _dernierBurn = 0;
  bool     _dejaConnecte = false;
  bool     _modeCRC = false;
  bool     _legacyBloque = false;
  char     _derniereCommande = 0;

  bool     _erreurRAM = false;          // une page ou une commande n'a pas trouvé de place en RAM
  bool     _sauvegardeEnAttente = false;
  bool     _memoireTient = false;
  uint16_t _finMemoire = TS_EEPROM_DEBUT;

  /*----------------------------------------------------------------------------------------
   *  Commandes et en-tête
   *----------------------------------------------------------------------------------------*/
  // Position de x sur un axe : case i et fraction f (0 à 256) vers la case suivante
  template <typename TA>
  static void _position(const TA *axe, size_t n, int32_t x, uint8_t &i, int32_t &f)
  {
    i = 0; f = 0;
    if ((n < 2) || (x <= (int32_t)axe[0])) { return; }
    if (x >= (int32_t)axe[n - 1]) { i = (uint8_t)(n - 1); return; }
    while ((i < n - 2) && (x >= (int32_t)axe[i + 1])) { i++; }
    int32_t ecart = (int32_t)axe[i + 1] - (int32_t)axe[i];
    if (ecart > 0) { f = ((x - (int32_t)axe[i]) * 256) / ecart; }
  }


  bool _ajouterCommande(uint16_t numero, TS_Fonction simple, TS_FonctionCommande fonction)
  {
    if (_nbCommandes >= _placeCommandes)
    {
      Commande *nouvelle = (Commande *)realloc(_commandes, (_placeCommandes + 8) * sizeof(Commande));
      if (nouvelle == 0) { _erreurRAM = true; return false; }  // plus de RAM
      _commandes = nouvelle;
      _placeCommandes += 8;
    }
    _commandes[_nbCommandes].numero = numero;
    _commandes[_nbCommandes].simple = simple;
    _commandes[_nbCommandes].fonction = fonction;
    _nbCommandes++;
    return true;
  }


  /*----------------------------------------------------------------------------------------
   *  Pages
   *----------------------------------------------------------------------------------------*/
  int16_t _indexPage(uint8_t numeroPage)
  {
    for (uint16_t i = 0; i < _nbPages; i++)
    {
      if (_pages[i].numero == numeroPage) { return (int16_t)i; }
    }
    return -1;
  }

  // Relit toutes les pages en mémoire au démarrage. Une page n'est recopiée dans votre
  // structure que si son CRC est bon : sinon la structure garde ses valeurs par défaut.
  void _chargerMemoire()
  {
    uint32_t taille = 2;   // en-tête
    for (uint16_t i = 0; i < _nbPages; i++) { taille += (uint32_t)_pages[i].taille + 4; }
    _memoireTient = ((uint32_t)TS_EEPROM_DEBUT + taille) <= (uint32_t)TS_TAILLE_MEMOIRE;
    _finMemoire = (uint16_t)(TS_EEPROM_DEBUT + taille);
    for (uint16_t i = 0; i < _nbPages; i++) { _pages[i].valide = false; _pages[i].relue = false; }
    if (!_memoireTient) { return; }

    ts_memDebut();
    uint16_t adresse = TS_EEPROM_DEBUT;
    bool enteteOK = (ts_memLire(adresse) == TS_MAGIC) &&
                    (ts_memLire(adresse + 1) == TS_EEPROM_VERSION);
    adresse += 2;

    for (uint16_t i = 0; i < _nbPages; i++)
    {
      uint16_t t = _pages[i].taille;
      if (enteteOK)
      {
        // 1er passage : CRC de ce qui est stocké, sans toucher à la structure
        uint32_t crc = 0xFFFFFFFFUL;
        for (uint16_t j = 0; j < t; j++) { crc = ts_crc32Octet(crc, ts_memLire(adresse + j)); }
        crc = ~crc;
        uint32_t crcStocke = 0;
        for (uint8_t k = 0; k < 4; k++)
        {
          crcStocke |= (uint32_t)ts_memLire(adresse + t + k) << (8 * k);
        }
        // 2e passage : recopie seulement si le CRC est bon
        if (crc == crcStocke)
        {
          for (uint16_t j = 0; j < t; j++) { _pages[i].donnees[j] = ts_memLire(adresse + j); }
          _pages[i].valide = true;
          _pages[i].relue = true;
        }
      }
      adresse += t + 4;
    }

#if TS_AUTO_DEFAUTS
    // Pages invalides : on écrit leurs valeurs par défaut pour la prochaine fois
    for (uint16_t i = 0; i < _nbPages; i++) { _pages[i].aSauver = !_pages[i].relue; }
    saveNow();
    // saveNow() marque les pages écrites comme valides : memoryOK() doit pourtant dire
    // ce qui a été RELU au démarrage, on remet donc le statut d'origine
    for (uint16_t i = 0; i < _nbPages; i++) { _pages[i].valide = _pages[i].relue; }
#endif
  }

  /*----------------------------------------------------------------------------------------
   *  Réception, octet par octet (jamais bloquant)
   *----------------------------------------------------------------------------------------*/
  void _recevoir(uint8_t c)
  {
    switch (_etat)
    {
      case ETAT_ATTENTE:
      {
        if (c == 0xF0) { return; }   // octet envoyé par Windows à l'ouverture du port (DTR)

        bool lettre = ((c >= 'A') && (c <= 'z')) || (c == '?');
#ifdef TS_PROTOCOLE_CRC
        // En mode CRC, le 1er octet d'une trame est le poids fort de la longueur (0x00
        // ou 0x01 en pratique). Une lettre est donc une commande legacy, sauf si une trame
        // CRC vient de passer (alors on reste en CRC, sauf 'F' toujours accepté).
        if (lettre && (!_legacyBloque || (c == 'F')))
#else
        if (lettre)
#endif
        {
          _buffer[0] = c;
          _nbRecus = 1;
          _etat = ETAT_LEGACY;
          _legacyVerifier();
        }
#ifdef TS_PROTOCOLE_CRC
        else
        {
          _longueur = (uint16_t)c << 8;   // poids fort de la longueur
          _etat = ETAT_CRC_LONGUEUR;
        }
#endif
        // Protocole léger : tout octet qui n'est pas une lettre est ignoré
        break;
      }

      case ETAT_LEGACY:
        _buffer[_nbRecus++] = c;
        _legacyVerifier();
        break;

      case ETAT_LEGACY_IGNORE:
        if (--_aIgnorer == 0) { _etat = ETAT_ATTENTE; }
        break;

      case ETAT_CRC_LONGUEUR:
        _longueur |= c;
        _nbRecus = 0;
        if (_longueur == 0) { _etat = ETAT_ATTENTE; }                 // trame vide : ignorée
        else if (_longueur > TS_TAILLE_BUFFER)
        {
          _aIgnorer = (uint32_t)_longueur + 4;                       // données + CRC
          _etat = ETAT_CRC_IGNORE;
        }
        else { _etat = ETAT_CRC_DONNEES; }
        break;

      case ETAT_CRC_DONNEES:
        _buffer[_nbRecus++] = c;
        if (_nbRecus >= _longueur) { _crcRecu = 0; _nbOctetsCRC = 0; _etat = ETAT_CRC_CRC; }
        break;

      case ETAT_CRC_CRC:
        _crcRecu = (_crcRecu << 8) | c;   // CRC envoyé poids fort en premier
        if (++_nbOctetsCRC >= 4)
        {
          _etat = ETAT_ATTENTE;
          if (_crcRecu == ts_crc32(_buffer, _longueur))
          {
            _legacyBloque = true;          // TS parle en CRC : on ne lit plus de legacy
            _traiter(_buffer, _longueur, true);
          }
          else
          {
            crcErrors++;
            _envoyerCode(TS_RC_CRC_ERR);
          }
        }
        break;

      case ETAT_CRC_IGNORE:
        if (--_aIgnorer == 0)
        {
          _etat = ETAT_ATTENTE;
          rangeErrors++;
          _envoyerCode(TS_RC_HORS_LIMITE);
        }
        break;

      default:
        _etat = ETAT_ATTENTE;
        break;
    }
  }

  // Commande legacy : regarde si elle est complète (la longueur dépend de la lettre)
  void _legacyVerifier()
  {
    uint16_t attendu;
    switch (_buffer[0])
    {
      case 'p':                       // p + id page (2) + offset (2) + longueur (2)
      case 'r':                       // r + canId + 0x30 + offset (2) + longueur (2)
        attendu = 7;
        break;

      case 'M':                       // M + id page (2) + offset (2) + longueur (2) + données
        attendu = 7;
        if (_nbRecus >= 7)
        {
          uint16_t n = (uint16_t)_buffer[5] | ((uint16_t)_buffer[6] << 8);
          if ((7U + n) > TS_TAILLE_BUFFER)
          {
            // Bloc plus gros que le buffer (blockingFactor du .ini trop grand) : on laisse
            // passer les données pour rester synchronisé
            rangeErrors++;
            if (n == 0) { _etat = ETAT_ATTENTE; }
            else { _aIgnorer = n; _etat = ETAT_LEGACY_IGNORE; }
            return;
          }
          attendu = 7 + n;
        }
        break;

      case 'd':                       // d + id page (2)
      case 'b':                       // b + id page (2)
      case 'B':
      case 'E':                       // E + n° commande (2)
        attendu = 3;
        break;

      default:                        // Q, S, F, C, I, A... : une seule lettre
        attendu = 1;
        break;
    }

    if (_nbRecus >= attendu)
    {
      _etat = ETAT_ATTENTE;
      _traiter(_buffer, _nbRecus, false);
    }
  }

  /*----------------------------------------------------------------------------------------
   *  Envoi des réponses
   *  Légère : les données brutes.
   *  CRC    : longueur (2, poids fort d'abord) + code retour + données + CRC32 (4)
   *----------------------------------------------------------------------------------------*/
  void _debutReponse(uint16_t tailleDonnees)
  {
    if (_enveloppe)
    {
      uint16_t n = tailleDonnees + 1;        // +1 pour le code retour
      _port->write((uint8_t)(n >> 8));
      _port->write((uint8_t)(n & 0xFF));
      _crcEnvoi = 0xFFFFFFFFUL;
      _octet(TS_RC_OK);
    }
  }

  void _octet(uint8_t b)
  {
    _port->write(b);
    if (_enveloppe) { _crcEnvoi = ts_crc32Octet(_crcEnvoi, b); }
  }

  void _finReponse()
  {
    if (_enveloppe)
    {
      uint32_t crc = ~_crcEnvoi;
      _port->write((uint8_t)(crc >> 24));
      _port->write((uint8_t)(crc >> 16));
      _port->write((uint8_t)(crc >> 8));
      _port->write((uint8_t)crc);
    }
  }

  // Réponse "code seul" (n'existe qu'en protocole CRC ; en léger, rien n'est envoyé)
  void _envoyerCode(uint8_t code)
  {
    if (_port == 0) { return; }
#ifdef TS_PROTOCOLE_CRC
    _port->write((uint8_t)0x00);
    _port->write((uint8_t)0x01);
    uint32_t crc = ~ts_crc32Octet(0xFFFFFFFFUL, code);
    _port->write(code);
    _port->write((uint8_t)(crc >> 24));
    _port->write((uint8_t)(crc >> 16));
    _port->write((uint8_t)(crc >> 8));
    _port->write((uint8_t)crc);
#else
    (void)code;
#endif
  }

  void _envoyerCodeSiCRC(uint8_t code) { if (_enveloppe) { _envoyerCode(code); } }

  void _envoyerTexte(const char *texte)
  {
    uint16_t n = (uint16_t)strlen(texte);
    _debutReponse(n);
    for (uint16_t i = 0; i < n; i++) { _octet((uint8_t)texte[i]); }
    _finReponse();
  }

  // Erreur de limites : en CRC on le dit à TS ; en léger on renvoie quand même le bon
  // nombre d'octets (des zéros) pour que TS reste synchronisé
  void _horsLimite(uint16_t octetsAttendus)
  {
    rangeErrors++;
    if (_enveloppe) { _envoyerCode(TS_RC_HORS_LIMITE); return; }
    for (uint16_t i = 0; i < octetsAttendus; i++) { _port->write((uint8_t)0); }
  }

  /*----------------------------------------------------------------------------------------
   *  Traitement d'une commande complète
   *  p[0] = lettre ; pour p/M/d/b/r : p[1] = canId (ignoré), p[2] = n° de page
   *  offset et longueur en petit-boutiste (poids faible d'abord), comme le .ini
   *----------------------------------------------------------------------------------------*/
  void _traiter(const uint8_t *p, uint16_t n, bool enveloppe)
  {
    _enveloppe = enveloppe;
    _modeCRC = enveloppe;
    _derniereCommande = (char)p[0];
    _dernierValide = millis();
    _dejaConnecte = true;

    switch (p[0])
    {
      case 'Q':                                     // signature
        _envoyerTexte(TS_SIGNATURE);
        break;

      case 'S':                                     // texte de version (+ erreur éventuelle)
        if      (!_memoireTient) { _envoyerTexte(TS_VERSION_INFO " - ERREUR : EEPROM trop petite pour les pages"); }
        else if (_erreurRAM)     { _envoyerTexte(TS_VERSION_INFO " - ERREUR : RAM insuffisante (page ou commande refusee)"); }
        else                     { _envoyerTexte(TS_VERSION_INFO); }
        break;

      case 'F':                                     // version du protocole série
        _envoyerTexte(TS_VERSION_PROTOCOLE);
        break;

      case 'C':                                     // test de communication
        if (_enveloppe) { _debutReponse(1); _octet(255); _finReponse(); }
        else { _port->write((uint8_t)1); }
        break;

      case 'I':                                     // CAN ID (toujours 0)
        _debutReponse(1); _octet(0); _finReponse();
        break;

      case 'f':                                     // capacités série (protocole CRC)
        if (!_enveloppe) { break; }
        _debutReponse(5);
        _octet(2);                                  // version de protocole
        _octet((uint8_t)(TS_BLOCKING_FACTOR >> 8)); _octet((uint8_t)(TS_BLOCKING_FACTOR & 0xFF));
        _octet((uint8_t)(TS_BLOCKING_FACTOR >> 8)); _octet((uint8_t)(TS_BLOCKING_FACTOR & 0xFF));
        _finReponse();
        break;

      case 'A':                                     // toutes les données temps réel
        _envoyerSorties(0, _tailleSorties);
        break;

      case 'r':                                     // données temps réel (offset, longueur)
      {
        if (n < 7) { _horsLimite(0); break; }
        uint16_t offset = _mot(p[3], p[4]);
        uint16_t longueur = _mot(p[5], p[6]);
        if (p[2] == TS_PAGE_SORTIES)
        {
          if ((_fonctionLog == 0) || ((uint32_t)offset + longueur > _tailleSorties))
          {
            _horsLimite(longueur);
          }
          else { _envoyerSorties(offset, longueur); }
        }
        else if ((p[2] == 0x0F) && _enveloppe) { _envoyerTexte(TS_SIGNATURE); }
        else { _horsLimite(longueur); }
        break;
      }

      case 'p':                                     // lecture d'une page de réglages
      {
        if (n < 7) { _horsLimite(0); break; }
        int16_t i = _indexPage(p[2]);
        uint16_t offset = _mot(p[3], p[4]);
        uint16_t longueur = _mot(p[5], p[6]);
        if ((i < 0) || ((uint32_t)offset + longueur > _pages[i].taille))
        {
          _horsLimite(longueur);
          break;
        }
        _debutReponse(longueur);
        for (uint16_t j = 0; j < longueur; j++) { _octet(_pages[i].donnees[offset + j]); }
        _finReponse();
        break;
      }

      case 'M':                                     // écriture dans une page de réglages
      {
        if (n < 7) { _envoyerCodeSiCRC(TS_RC_HORS_LIMITE); rangeErrors++; break; }
        int16_t i = _indexPage(p[2]);
        uint16_t offset = _mot(p[3], p[4]);
        uint16_t longueur = _mot(p[5], p[6]);
        if ((i < 0) || ((uint32_t)offset + longueur > _pages[i].taille) ||
            ((uint32_t)7 + longueur > n))
        {
          _envoyerCodeSiCRC(TS_RC_HORS_LIMITE);
          rangeErrors++;
          break;
        }
        for (uint16_t j = 0; j < longueur; j++) { _pages[i].donnees[offset + j] = p[7 + j]; }
        _pages[i].modifiee = true;
        _envoyerCodeSiCRC(TS_RC_OK);
        break;
      }

      case 'd':                                     // CRC32 d'une page (comparé par TS)
      {
        if (n < 3) { _horsLimite(4); break; }
        int16_t i = _indexPage(p[2]);
        if (i < 0) { _horsLimite(4); break; }
        uint32_t crc = ts_crc32(_pages[i].donnees, _pages[i].taille);
        _debutReponse(4);
        _octet((uint8_t)(crc >> 24));               // poids fort d'abord
        _octet((uint8_t)(crc >> 16));
        _octet((uint8_t)(crc >> 8));
        _octet((uint8_t)crc);
        _finReponse();
        break;
      }

      case 'b':                                     // burn : sauvegarde de la page
      case 'B':
      {
        if (n < 3) { _envoyerCodeSiCRC(TS_RC_HORS_LIMITE); rangeErrors++; break; }
        int16_t i = _indexPage(p[2]);
        if (i < 0) { _envoyerCodeSiCRC(TS_RC_HORS_LIMITE); rangeErrors++; break; }
        save(p[2]);                                 // écrite après TS_DELAI_SAUVEGARDE
        _envoyerCodeSiCRC(TS_RC_BURN_OK);
        break;
      }

      case 'E':                                     // commande directe (bouton TS)
      {
        if (n < 3) { _envoyerCodeSiCRC(TS_RC_HORS_LIMITE); rangeErrors++; break; }
        uint16_t numero = ((uint16_t)p[1] << 8) | p[2];
        bool trouve = false;
        for (uint16_t i = 0; i < _nbCommandes; i++)
        {
          if (_commandes[i].numero == numero)
          {
            if (_commandes[i].simple != 0)   { _commandes[i].simple(); }
            if (_commandes[i].fonction != 0) { _commandes[i].fonction(numero); }
            trouve = true;
            break;
          }
        }
        if (!trouve)
        {
          if (numero == TS_CMD_RAZ_ERREURS)
          {
            crcErrors = 0; timeoutErrors = 0; rangeErrors = 0; unknownCommands = 0;
          }
          else if (numero == TS_CMD_DEFAUTS) { restoreDefaults(); }
          else if (_commandeParDefaut != 0)  { _commandeParDefaut(numero); }
        }
        _envoyerCodeSiCRC(TS_RC_OK);
        break;
      }

      default:                                      // commande inconnue
        unknownCommands++;
        _envoyerCodeSiCRC(TS_RC_INCONNU);
        break;
    }
  }

  // Remplit un en-tête standard (secondes, statut, erreurs, boucles/s)
  void _envoyerSorties(uint16_t offset, uint16_t longueur)
  {
    // Chaque octet est demandé à la fonction de l'utilisateur (getTSLogEntry)
    if (_majStatus != 0) { _majStatus(_status, this); }   // valeurs de com à jour
    _debutReponse(longueur);
    for (uint16_t j = 0; j < longueur; j++)
    {
      uint16_t o = offset + j;
      if (_fonctionLog != 0) { _octet(_fonctionLog(o)); }
      else                   { _octet(0); }
    }
    _finReponse();
  }

  static uint16_t _mot(uint8_t faible, uint8_t fort) { return (uint16_t)faible | ((uint16_t)fort << 8); }
};

// L'objet unique, utilisable partout dans le croquis : TS.update(), TS.connected()...
BZO_TS TS;

#endif // BZO_TS_H
