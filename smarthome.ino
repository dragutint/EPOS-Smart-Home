#include <MFRC522.h>            // RFID biblioteka
#include <Servo.h>              // Servo biblioteka
#include <BlynkSimpleEsp8266.h> // Blynk biblioteka

/*
 * Pinovi koje koristi RFID citac
 */
#define SS_PIN D8  
#define RST_PIN D3
#define BLYNK_PRINT Serial

/*
 * Kreiranje instance RFID citaca i Servo motora
 */
MFRC522 mfrc522(SS_PIN, RST_PIN); 
Servo myservo;                    

/*
 * WiFi podaci za povezivanje na internet
 */
const char* ssid = "WiFON";    
const char* pass = "Mu4Rovaca7";  

/*
 * Kod za povezivanje Blynk aplikacije sa razvojnom plocom
 */
const char* auth = "0f27b1b0e5174bcc906175043c670527";   

/*
 * UID tagovi koji imaju pristup objektu
 */
int UIDbrojac = 1;
String uids[10] = {"A9 B5 63 48"}; 

/* 
 * Metoda koja dodaje nov UID tag u bazu
 * @param uid UID tag koji je RFID citac procitao
 */
void addUID(String uid){
  uids[UIDbrojac++] = uid;
}

/*
 * Metoda koja prihvata zahtev iz aplikacije. Kada korisnik u aplikaciji 
 * pritisne dugme na virtual pin-u 0, ploca prihvata parametar koji je 
 * poslat od strane aplikacije i kontrolise svetlo ovom metodom
 */
BLYNK_WRITE(V0){
  int pinValue = param.asInt();   // prihvatanje parametra koji je poslat 

  if(pinValue){                   // 1 - upali svetlo
    digitalWrite(D2, HIGH);
    Serial.println("Korisnik je upalio svetlo preko aplikacije");
    Blynk.virtualWrite(V4, "Upalili ste svetlo\n");
  } else {                        // 0 - ugasi svetlo
    digitalWrite(D2, LOW);
    Serial.println("Korisnik je ugasio svetlo preko aplikacije");
    Blynk.virtualWrite(V4, "Ugasili ste svetlo\n");
  }
}

/*
 * Metoda koja prihvata zahtev iz aplikacije. Kada korisnik u aplikaciji 
 * pritisne dugme na virtual pin-u 1,ploca prihvata parametar koji je 
 * poslat od strane aplikacije i kontrolise bravu ovom metodom
 */
BLYNK_WRITE(V1){
  int pinValue = param.asInt();   // prihvatanje parametra koji je poslat

  if(pinValue){                   // 1 - otkljucaj bravu
    myservo.write(45);   
    Serial.println("Korisnik je otkljucao vrata preko aplikacije");
    Blynk.virtualWrite(V4, "Otkljucali ste vrata\n");
  } else {                        // 0 - zakljucaj bravu
    myservo.write(135);  
    Serial.println("Korisnik je zakljucao bravu");
    Blynk.virtualWrite(V4, "Zakljucali ste vrata\n");
  }
}

/*
 * Metoda koja prihvata zahtev iz aplikacije za unos nove kartice. 
 * Kada korisnik u aplikaciji pritisne dugme na virtual pin-u 3,
 * ploca prihvata parametar koji je poslat od strane aplikacije i 
 * ocekuje novu karticu da ocita citac i unese u bazu
 */
BLYNK_WRITE(V3){
  int pinValue = param.asInt();
  int counter = 0;
  
  if(pinValue){
    while(counter < 10){
      if ( mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial() ) {
        String UID = getUID();
        addUID(UID);
        Serial.println("Nov UID dodat: " + UID);
        Blynk.virtualWrite(V4, "Nov UID dodat: " + UID + "\n");
        delay(1000);
        break;
      }
      delay(1000);
      counter++;
    }
  }
}


/* 
 * Metoda koja proverava validnost UID taga
 * @param uid UID tag koji je RFID citac procitao
 * @return true ako je UID validan, false ako nije UID validan
 */
boolean validUID(String uid){
  for(int i = 0; i < UIDbrojac; i++){
    if(String(uids[i]) == uid){
      return true;  
    }
  }  
  return false;
}

/*
 * Metoda koja se pokrece samo pri startovanju razvojne ploce
 */
void setup() {
  // pokretanje ploce
  Serial.begin(9600); 

  // pokretanje RFID citaca
  SPI.begin();
  mfrc522.PCD_Init();   
  Serial.println("RFID citac je pokrenut");
  Blynk.virtualWrite(V4, "RFID citac je pokrenut\n");

  myservo.attach(D1);     // pokretanje servo motora na D1 izlazu
  myservo.write(135);     // inicijalna vrednost servo motora
                          // (ugao od 135 stepeni - zakljucana brava)
  
  pinMode(D2, OUTPUT);    // pokretanje uticnice na D2 izlazu
  digitalWrite(D2, LOW);  // inicijalna vrednost uticnice 
                          // (LOW odnosno 0, sto znaci da je strujno kolo 
                          //  prekinuto odnosno uticnica je ugasena)
  
  // Konektovanje razvojne ploce na internet i povezivanje sa Blynkom
  Blynk.begin(auth, ssid, pass);

  // Upisivanje pocetnog teksta na label V2
  Blynk.virtualWrite(V2, "Duh!");

  // Brisanje starog loga iz terminala
  Blynk.virtualWrite(V4, "clr");
}

/*
 * Vraca UID tag kartice koju je RFID procitao
 * @return UID tag kartice
 */
String getUID(){
  String content = "";
  
  for (byte i = 0; i < mfrc522.uid.size; i++){
     content.concat(String(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " "));
     content.concat(String(mfrc522.uid.uidByte[i], HEX));
  }

  content.toUpperCase();
  return content.substring(1);
}

/*
 * Logika aplikacije, kada korisnik prinese karticu RFID citacu, 
 * ova metoda se pokrece
 */
void logic(){
  // izvlacenje UID taga iz kartice i skladistenje
  String UID = getUID();
  
  /*
   * promenljiva koja oznacava da li je svetlo bilo 
   * upaljeno pre prinosenja kartice RFID citacu
   */
  bool isTurnedOn = false;

  // VALIDNA KARTICA // 
  if( validUID(UID) ){      
    myservo.write(45);      // otkljucavanje brave ukoliko je validna kartica

    // provera da li je svetlo bilo upaljeno pre prinosenja kartice RFID citacu
    if(digitalRead(D2) == HIGH){
      isTurnedOn = true;
    } else {
      digitalWrite(D2, HIGH); // paljenje svetla
    }
    

    Serial.println("Korisnik (UID: " + UID + ") je usao u objekat. Vrata su se otkljucala, svetlo se upalilo");
    Blynk.virtualWrite(V4, "Korisnik (UID: " + UID + ") je usao u objekat. Vrata su se otkljucala, svetlo se upalilo\n");
    Serial.println();

    Blynk.virtualWrite(V2, UID);   // Slanje UID taga Blynk aplikaciji i prikazivanje ko je usao
    
    delay(5000);          // cekanje pet sekundi da korisnik udje u zgradu i zatvori vrata

    // ukoliko svetlo nije bilo upaljeno ranije, pre dolaska korisnika do RFID citaca i prinosenja kartice,
    // svetlo ce biti ugaseno, ako jeste, svetlo se nece ugasiti
    if(!isTurnedOn){
      digitalWrite(D2, LOW);// gasenje svetla
    }
    
    myservo.write(135);   // zakljucavanje brave 
    delay(500);           // pauza od pola sekunde, ceka se servo motor da se vrati u pocetni polozaj

  // NIJE VALIDNA KARTICA // 
  } else {
    Serial.println("Korisnik nema pravo da udje u zgradu: UID: " + UID);
    Blynk.virtualWrite(V4, "Korisnik nema pravo da udje u zgradu: UID: " + UID + "\n");
    delay(1000);
  }
}

/*
 * Metoda koja se pokrece nakon metode setup() i pokrece 
 * se iterativno dok je pokrenuta razvojna ploca
 */
void loop() {
  Blynk.run(); 

  /*
   * Kada korisnik prinese karticu RFID citacu, uslov u IF-u 
   * je zadovoljen i pokrece se metoda logic()
   */
  if ( mfrc522.PICC_IsNewCardPresent() && mfrc522.PICC_ReadCardSerial()) {
    logic();
  }  
} 