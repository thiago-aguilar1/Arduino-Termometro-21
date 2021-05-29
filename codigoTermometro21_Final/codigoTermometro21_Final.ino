#include <Wire.h>  
#include <LiquidCrystal_I2C.h>   

LiquidCrystal_I2C lcd(0x27, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE);  //ENDEREÇO DO I2C E DEMAIS INFORMAÇÕES
const int lm35 = A2;
const int red = 2;
const int green = 3;
const int blue = 4;
const int buzina = 8;
float sinVal;              /* variáveis que auxiliarão no cálculo da senoida da buzina */
int toneVal;
byte grau[8] = {B00110, B01001, B00110, B00000, B00000, B00000, B00000, B00000};
byte quadradoOco[8] = {B11111, B10001, B10001, B10001, B10001, B10001, B10001, B11111};
byte quadradoCheio[8] = {B11111, B11111, B11111, B11111, B11111, B11111, B11111, B11111};
  
int colunas = 16;          /* Configuracoes LCD 16x2 */
int linhas = 2;  
float meiaSenoide, porcaoTempinho, tempinho;

float temp = 0.0;
float temperatura[6];
float maxTemp = -100.0, minTemp = 100.0;
int votacao = 0;
bool histerese = false;
bool estavel = false;
float tempEstavel;
int amortecimento = 0;
 
int comecoVisivel = 0;
int corretor = 0;
int posicaoGrauMax = 10;
int posicaoGrauMin = 23;

bool direcao = true;      /*direção do sentido do deslocamento da linha móvel (linha inferior).   */

int velocidadeDisplay = 2;
int velocidadeLinhaInferior = velocidadeDisplay;

/*   ****************************************************************   void setup   **************************************************************************  
*   ***********************************************************************************************************************************************************  */


void setup() 
{  

  pinMode(buzina, OUTPUT);
  digitalWrite(buzina, HIGH);    /* pois a buzina aciona em nível baixo */

  pinMode(red, OUTPUT);
  pinMode(green, OUTPUT);
  pinMode(blue, OUTPUT);
  digitalWrite(red, LOW);
  digitalWrite(green, HIGH);
  digitalWrite(blue, LOW);

  
  //Inicializacao do display  
  lcd.begin(colunas,linhas);  
  lcd.setBacklight(HIGH);    //LIGA O BACKLIGHT (LUZ DE FUNDO).
  lcd.createChar(1, grau);
  lcd.createChar(2, quadradoOco);
  lcd.createChar(3, quadradoCheio);


   /* *****************  Apresentação inicial da equiipe no Display  ********************** */ 
lcd.clear();
lcd.setCursor(5, 0);
lcd.print("AMPERE");

  
for (int i = 0; i < 16; i++){   /*coloca os quadradinhos ocos */
  lcd.setCursor(i,1);
  lcd.write(2);
}


for (int i = 0, j = 15; i <= 90 && j >= 0 ; i+= 6, j--){     
  meiaSenoide = sin( float(i) * (3.1415/180.0) );
  porcaoTempinho = 3.0 * float(meiaSenoide*255.0/2.0);
  tempinho = 3.0 + porcaoTempinho;
  delay(int(tempinho));
  lcd.setCursor(j,1);
  lcd.write(3);         /* colocando aos poucos os quadradinhos cheios */
}
  
} 
/*   ************************************************************** void loop ***********************************************************************************  
 *    ********************************************************************************************************************************************************* */
void loop()         
{ 


  temp = ( 5.0 * analogRead(lm35) * 100.0) / 1024.0;               /*  medindo temperatura   */
  delay(300);                                                   /*  delay de tempo após cada medição de temperatura    */
  

/*  ************************************* Começa o tratamento para a estabilidade da apresentação da temperatura **********************************************  */

  if(!histerese)                                /*  tratamento para uma variação rápida de temperatura.   */
    if(temp == temperatura[0])  histerese = true;     /* o temperatura[0] é referente à medição anterior  */
    else temperatura[0] = temp;           /*agora o temperatura[0] se referirá à temperatura atual  */
    


/*  ******************************   */

  if(histerese){                              /* tratamento para temperatura média constante ou com variação lenta   */  

    for (int i = 5; i > 0 ; i--)           /* atualizando o histórico das temperaturas sentidas pelo sensor. */
      temperatura[i] = temperatura[i-1];
    temperatura[0] = temp;            /*agora o temperatura[0] se referirá à temperatura atual  */


    if(!estavel){   /* querendo entrar para estavel, ou seja, verifica a persistência do temperatura em parmenecer sempre a mesma  */
      for (int i = 2; i <= 5 ; i++)
        if(temperatura[i] == temp) 
          votacao++; 
      if(votacao >= 3){    /* ou seja, o mesmo valor está presente em 4 amostras, na amostra/medida atual, na consecutiva anterior e em alguma parte do histórico.*/
        estavel = true;
        tempEstavel = temp;   
      }
      else histerese = false;   
      votacao = 0;
    }

    
    
    if (estavel){    /* aqui é o estavel, ou seja, quanto há constância no valor da temperatura. Aqui se verifica se faz sentido continuar "segurando" o valor da temperatura por um valor constante. */
      for (int i = 1; i < 6 ; i++)                    /* aqui que traz latencia, ou seja, um certo atraso para o termômetro perceber quando não é mais válida a condição estável */ 
        if(tempEstavel == temperatura[i]) votacao++;          /*  verifica se ainda é válida a condição de estável  */
     
      if(votacao >= 4) temp = tempEstavel;                /* Perceba que muda o temp (para a estabilidade), mas não muda o histórico de temperaturas registradas. A temperatura atual já foi registrada no histórico, antes de a variável temp ser modificada pelo código estabilizador. */
      else{                                       
        estavel = false;
        histerese = false;
      }
      votacao = 0;
    } 
             
  }
 /* *****************************************************   Acabou o tratamento da estabilidade  **************************************************************  */



  
  if (temp > maxTemp) maxTemp = temp;         /*atualiza as temperaturas max e min já sentidas pelo sensor */
  else if (temp < minTemp) minTemp = temp; 


 
                 /* ************  aqui começa o tratamento da buzina e para o LED ************ */
                 
  if (temp > 32.60 || temp < 25.20) {      /* verifica se houve uma transição de saída */             

    if( amortecimento == 0 || amortecimento == 20 ){        /* verifica se já foi passado pelo menos 16 ciclos após a última transição de entrada  */
      for (int x = 0; x < 180; x++) {
      //converte graus para radianos, e depois obtém o valor do seno
      sinVal = (sin(x * (3.1416 / 180)));
      //gera uma frequência a partir do valor do seno
      toneVal = 2000 + (int(sinVal * 1000));
      tone(buzina, toneVal);
      delay(2);
    }
    
    if (temp > 32.6) {
      digitalWrite(red, HIGH);
      digitalWrite(green, LOW);
      digitalWrite(blue, LOW);
    }
    else{
      digitalWrite(red, LOW);
      digitalWrite(green, LOW);            
      digitalWrite(blue, HIGH);
    }
    amortecimento = 20;           /* esta variável "amortecimento" aqui vai me ajudar para aquele momento de transição da temperatura fora do controle para a temperatura de dentro do controle, que é quando a buzina para de tocar. O bojetivo do "amortecimento" é evitar o acionamento da buzina por um certo tempo. Neste caso, por 16 ciclos */ 
                                  /*  gosto de dizer que esse "amortecimento = 16" é para me ajudar na transição de entrada */
    }                                          
    else{
      amortecimento = 16;        /*  já o "amortecimento" aqui é para evitar o acionamento do buzer, caso haja uma transição muito recente em relação à última transição já acontecida, realimentando mais 14 ciclos para liberação da buzina.    */    
    }                            /* ou seja, esse "amortecimento = 14" é para quando houver mais outra transição de entrada, dado que já havia acontecido outra trasição de entrada em um intervalo menor que 16 ou 14 ciclos.   */ 
      
  }
  else if (amortecimento == 20) {
    noTone(buzina);                   
    digitalWrite(buzina, HIGH);
    
    digitalWrite(red, LOW);
    digitalWrite(green, HIGH);
    digitalWrite(blue, LOW);
    amortecimento--;
  }
  else if (amortecimento > 0) amortecimento-- ;    /* ********** aqui que termina o tratamento da buzina  *************  */





    
  lcd.setCursor(0, 0);                /*  É nesta linha que será apresentada a temperatura atual medida pelo sensor lm35.  */
  lcd.print("  Temp:" + String(temp) + " C"); 
  lcd.setCursor(12, 0);
  lcd.write(1);  
  
  lcd.setCursor(0, 1);               /* Para a mpressão da linha 2, a linha inferior.  */  
  lcd.print((" Max:"+String(maxTemp)+" C  Min:"+String(minTemp)+" C ").substring(comecoVisivel));   
  lcd.setCursor(posicaoGrauMax+corretor, 1);
  lcd.write(1);
  lcd.setCursor(posicaoGrauMin+corretor, 1);
  lcd.write(1);



if( !velocidadeLinhaInferior ){  
  if(comecoVisivel == 10){       /* verifica se já é hora de mudar o sentido de deslocamento da linha inferior.  */
    direcao = false;        /* troca de sentido  */
    delay(500);
  }
  else if(comecoVisivel == 0){
    direcao = true;         /* troca de sentido  */
    delay(500);
  }
  
  if(direcao){        /* tratamento para o movimento da linha inferior */             
    comecoVisivel++;     /*atualização do início porção de 16 caracteres que serão mostrados no display  */  
    corretor--;       /*atualização da posição dos símbolos de grau  */  
  }
  else{
    comecoVisivel--;      /*atualização do início porção de 16 caracteres que serão mostrados no display  */  
    corretor++;        /*atualização da posição dos símbolos de grau  */
  }
  velocidadeLinhaInferior = velocidadeDisplay;
}
else velocidadeLinhaInferior--;


}

/*  **********************************************************  Acava o void loop ***************************************************************************  * 
 *   *********************************************************  e o programa também  ************************************************************************  */

 
