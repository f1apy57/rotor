#include <EasyNextionLibrary.h>
#include <trigger.h>

/* Ce programme a été écrit 04/02/2022 par J. SCHERER - F1APY dit 'le pirate' :-), il est open source et donc peut 
 * être utilisé/modifié sous réserve d'indiquer l'origine du source.
 * Programme de commande de rotor YEASU GS1000, associé à un écran Nextion pour l'affichage azimut
 * On réutilise que la partie puissance (transfo + redressement et commande moteur rotor (interrupteurs 
 * poussoirs gauche droite) tout le reste de l'électronique d'origine est remplacé par la carte mega pro, l'ecran
 * nextion, une platine filtre en entrée de l'ADC avec protection en surtension et une platine relais pour la commande
 * moteur.
 * Les tesions fournies sont 22v pour la commande moteur et 12V pour le reste, un converter CC fournit du 8V pour alimenter 
 * la carte mega et le potar distant
 * Les cartes relais(une pour l'alim moteur et une pour l'orientation)sont au repos lorsque les pins de commande sont HIGH.
 * Le fonctionnement est assez simple, on lit la position grace à un potar couplé au rotor, 
 * la tension du potar est proportinelle à l'angle fourni par le rotor ATTENTION le rotor fait 1 tour et demi, donc 
 * 360° + 180° , les 180° supplémentaires correspondent à la position "OVERLAP" (dépassement).
 * on fait la conversion tension pot -> angle (calcul d'un ratio, arrondi à 1,9 dans ce cas (1024/(360+180))
 * la position est lue toutes les secondes, les commandes touchscreens sont lues en permanence et déclanchent les triggers de commande 
 * des relais.
 * Une interface de protection d'entrée pour la lecture de position est nécessaire, un ampli op monté en suiveur 
 * et un filtre passe bas "costaud" avec une diode zener limitant à vmax = 5.1 v pour éviter la destruction de l'AOP et de l'entrée ADC
 * de l'arduino sont mis en place. La tension aux bornes du portar coté + doit pouvoir être ajustée pour que l'on puisse lire +5V à l'entrée
 * de l'ADC quand le poter est à vmax.
 * une protection inhibe les touchescreen lorsque les touches mécaniques sont activées 
 * afin de ne pas mettre le circuit d'alim du moteur en court-circuit si l'on appuie sur les touchscreen en même temps
 * réalisé par la lecture d'une tension sur un pont diviseur pris sur la commande moteur sortie poussoir >>> sera peut-être modifiée si pas possible d'avoir 5 volt, 
 * dans ce cas on mettra les entrées de lectures sur broches ADC pour s'affranchir des 5V (fourchette entre 2,5 et 5)
 * 
 * ##################################################################################################################
 * 
 * Evolution potentielle : 
 * remplacement arduino par esp32 wifi permettant une commande par ordi, 
 * avec prépositionnement memorisé par indicatif recherché (intégrer le server web dans l'esp32 
 * et utiliser un thin client (tel,tablette ou ordi pour passer les commandes)............
 * voir interfacer avec HRD ou OMNIRIG
 * 
 */


#define relgauche 4  //relais de commande gauche
#define reldroite 5  //relais de commande droite
#define relAlimD 6 // relais alim
#define relAlimG 7 // relais alim 
#define btndroite 32 // bouton mecanique droit enfencé sera utilisé ultérieurement
#define btngauche 33 // bouton mécanique gauche enfoncé sera utilisé ultérieurement

/////////////////////////////init nextion//////////////////////////////////////////
EasyNex myNex(Serial1); 
bool overlap = false;
int valeur,newangle,oldangle,AZ_prog;
int potar = A0;
float ratio; 
int tempo;
int Xcent, Ycent,X,Y,X1,Y1,rayon;
float Sinang,Cosang;
String commande;
int evenement = 0; //sera utilisé ultérieurement
int touchemeca = 0 ; // blocage touchscreen sera utilisé ultérieurement
int emergency = 0;
/////////////////////////////////////////////////////////////////////////

void setup() {  
  myNex.begin(9600);//com avec nextion
  Serial.begin(9600);//debugpinMode(relAlimD, OUTPUT);
  analogReference(DEFAULT); // ref 5V pour l'acquisition 
  pinMode(relAlimG, OUTPUT);
  pinMode(relAlimD, OUTPUT);
  pinMode(relgauche, OUTPUT);
  pinMode(reldroite, OUTPUT);
  Serial.println("relais posit zero");
  digitalWrite(relgauche, HIGH); //relais au repos
  digitalWrite(reldroite, HIGH);  //relais au repos
  digitalWrite(relAlimG, HIGH); //relais au repos
  digitalWrite(relAlimD, HIGH);  //relais au repos
  //Serial.println("relais test ");
  //test_relais(); //test des relais  debug commenter en operation
 
  delay(2000);
  tempo=millis();
  myNex.writeStr("page page0"); // initialise la page pour afficher la recopie
  Serial.println("initialise page zero");
  delay(50);
}
////////////////////////////////////////////////////////////

void loop() {
/* //sécurité touches mécaniquess à definir plus tard

evenement = digitalRead(btndroite);
if (evenement ==1) { touchemeca =1; } // touche mecanique enfoncée !!!
else {touchemeca = 0;}
evenement = digitalRead(btngauche);
if (evenement ==1) { touchemeca =1; } // touche mecanique enfoncée !!!
else {touchemeca = 0;}
*/

     delay(300); // 1000 c'est trop, 300 ça doit etre bon a tester !
     //sert à la commande par les touch boutons 
     myNex.NextionListen(); //check pour un message "printh 23 02 54 00/01/02/.... pour activation des triggers"
     lit_posit(); //lire la valeur du potar et determiner l'angle de l'antenne / nord 0 a 360°    
     Serial.println(newangle);
     ecrit_posit(); //dessine la position de l'antenne
}
//////////////////////////////////////////////////////////////

void calc_angle(){ //sert à dessiner le vecteur sur l'écran
 Serial.println("calc_angle..."); 
 Xcent=240;
 Ycent=160  ;
 rayon = 90;
 Sinang=sin(newangle * 0.01745329); //axe des Y en radians
 Cosang=cos(newangle * 0.01745329); //axe des X en radians
 //utilisation de line:
 X = Xcent;
 Y = Ycent;
 X1=0;
 Y1=0;
 //calcul 1er cadran (0->90°)
 if ((newangle >=0) && (newangle<90)){
 Serial.println("calc 0-90"); 
 X1 = Xcent+int(rayon * Sinang); //axe des X
 Y1 = Ycent-int(rayon * Cosang); // axe des Y
 }
 if ((newangle >=90) && (newangle<180)) {
  Serial.println("calc 90-180"); 
   X1 = Xcent+int(rayon * Sinang); //axe des X
   Y1 = Ycent-int(rayon * Cosang); // axe des Y
   Serial.println(String(int(rayon * Cosang)));
   Serial.println(String(int(rayon * Sinang)));  
   }
 if ((newangle>=180) && (newangle<270)) {
  Serial.println("calc 180-270"); 
   X1 = Xcent+int(rayon * Sinang); //axe des X
   Y1 = Ycent-int(rayon * Cosang); // axe des Y
   Serial.println(String(int(rayon * Cosang)));
   Serial.println(String(int(rayon * Sinang)));  
   }
 if ((newangle>=270) && (newangle<360)) {
  Serial.println("calc 270-360"); 
   X1 = Xcent+int(rayon * Sinang); //axe des X
   Y1 = Ycent-int(rayon * Cosang); // axe des Y
   Serial.println(String(int(rayon * Cosang)));
   Serial.println(String(int(rayon * Sinang)));  
   } 
}
/////////////////////////////////////////////////////////////////////

void ecrit_posit() {
String message;
String oldmessage;
//ecrit la valeur de l'angle sur le nextion
myNex.writeNum("az.val",newangle);
Serial.println("ecrit_posit " + String(newangle));
//affiche graphiquement la position
calc_angle(); // pour affichage sur nextion
message = "line "+String(X)+","+String(Y)+","+String(X1)+","+String(Y1)+",BLACK";

//oldmessage = "line "+String(X)+","+String(Y)+","+String(X1)+","+String(Y1)+",44405"; //efface l'ancienne position
oldmessage = "cirs Xcent.val,Ycent.val,95,44405";
Serial.println("ecrit_posit " + message);
if (newangle != oldangle) { //efface derniere position
    myNex.writeStr(message);
    delay(200); // a ajuster !!!!!!!
    if ((oldangle>=newangle+2) || (oldangle <=newangle-2)) {
    myNex.writeStr(oldmessage);  
    }
   }
oldangle = newangle; //mise a jour
}
/////////////////////////////////////////////////////////////////////

void lit_posit()  {
  String message;
  valeur = analogRead(potar);
  ratio = 1.896;
  newangle = int(valeur/ratio)-33; //correction position
  //oldangle=newangle;
 // la pas ok n'imp !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
 // message = "vis t2,0";
  Serial.println("lit-posit.....");
 //  myNex.writeStr(message);
   //angle > 360 on est en overlap
   if (newangle > 360) { 
    overlap = true;
    newangle = newangle-360; //depassement overlap
    message = "vis t2,1"; 
    myNex.writeStr(message);
  }
   else {overlap=false;}  
}
////////////////////////////////////////////////////////////////////////////

void trigger0(){
 //fleche gauche appuyée cw
 
 Serial.println("commande CW");
 commut_rel_CCW(); 
}
////////////////////////////////////////////////////////////////////////////

void trigger1(){
 //fleche gauche relachée cw  
 Serial.println("commande CW relachée");
 arret_relais();
}
////////////////////////////////////////////////////////////////////////////

void trigger2(){
  Serial.println("commande CCW");
  commut_rel_CW();   
}
////////////////////////////////////////////////////////////////////////////

void trigger3(){
 Serial.println("commande CCW relachée");
 arret_relais();   
}
////////////////////////////////////////////////////////////////////////////

void trigger5(){
  // récupère l'azimut programmé
 AZ_prog = myNex.readNumber("page0.n2.val");
 Serial.println("azimut programmé" + String(AZ_prog));
 lit_posit(); // on lit la position actuelle 
 if (AZ_prog > newangle) {
  while(AZ_prog > newangle){
      Serial.println("azimut programmé" + String(AZ_prog));
      commut_rel_CW();
      lit_posit();
      calc_angle();
      ecrit_posit();
      myNex.NextionListen();
      if (emergency==1) {break;}
      //Serial.println("AZ_prog =" + String(AZ_prog)+" newangle = "+String(newangle));
      }
   arret_relais();
   AZ_prog=newangle;
   emergency=0;    
   }
 
 if (AZ_prog < newangle) {
   while(AZ_prog < newangle){
      commut_rel_CCW();
      lit_posit();
      calc_angle();
      ecrit_posit();
      myNex.NextionListen();
      if (emergency==1) {break;}
      //Serial.println("AZ_prog =" + String(AZ_prog)+" newangle = "+String(newangle));
      }
    arret_relais();
    AZ_prog=newangle;
    emergency=0;  
    }
}
//////////////////////////////////////////////////////////////////////////

void trigger7(){
 emergency = 1;
 arret_relais(); 
}
//////////////////////////////////////////////////////////////////////////

void commut_rel_CW() { //sens horaire
  //commutation des relais dans l'ordre, d'abord le sens de rotation, puis l'alimentation
  //pour le sens horaire les relais sont au repos, se referer à la doc technique, on ne commute finalement que les relais d'alim
  digitalWrite(relAlimD, LOW);
  digitalWrite(relAlimG, LOW);
}
//////////////////////////////////////////////////////////////////////////

void commut_rel_CCW() { //sens anti-horaire
  //commutation des relais dans l'ordre, d'abord le sens de rotation, puis l'alimentation
   //pour le sens anti-horaire CCW les relais sont atous activés, se referer à la doc technique, on commute d'abord le sens puis l'alim
  digitalWrite(relgauche, LOW);
  digitalWrite(reldroite, LOW);
  delay(500);
  digitalWrite(relAlimD, LOW);
  digitalWrite(relAlimG, LOW);  
}
///////////////////////////////////////////////////////////////////////////

void arret_relais() {
  //alim d'abord
  digitalWrite(relAlimD, HIGH);
  digitalWrite(relAlimG, HIGH);
  digitalWrite(relgauche, HIGH);
  digitalWrite(reldroite, HIGH); 
  emergency = 1; 
}
////////////////////////////////////////////////////////////////////////////
   
void test_relais() {
//test des relais moteur
digitalWrite(relgauche, LOW);
digitalWrite(reldroite, LOW);
delay(1000);
digitalWrite(relgauche, HIGH);
digitalWrite(reldroite, HIGH);
delay(2000);  
//test des relais alim
digitalWrite(relAlimD, LOW);
delay(1000);
digitalWrite(relAlimD, HIGH);
delay(1000);
digitalWrite(relAlimG, LOW);
delay(1000);
digitalWrite(relAlimG, HIGH);
}
