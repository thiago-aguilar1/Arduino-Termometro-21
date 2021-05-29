


Sumario
=======

<!--ts-->

   * [Sumario](#sumario)
   * [Componentes Utilizados](#componentes-utilizados)
   * [Sobre o Trabalho](#sobre-o-trabalho)
   * [Notas sobre a logica envolvida](#notas-sobre-a-logica-envolvida)
   * [O Codigo](#o-codigo)
   * [Analisando o codigo](#analisando-o-codigo)
      * [Declaracao das variaveis](#declaracao-das-variaveis)
      * [void setup](#void-setup)
         * [Definicao das variaveis e pinos](#definicao-das-variaveis-e-pinos)
         * [Apresentacao inicial e animada no LCD](#apresentacao-inicial-e-animada-no-lcd]
      * [void loop](#void-loop)
         * [Medicao da temperatura](#medicao-da-temperatura)
         * [Tratamento da Estabilidade](#tratamento-da-estabilidade)
            * [1 Verificacao de possivel histerese](#1-verificacao-de-possivel-histerese)
            * [2 Validacao de presenca de histerese e aprovacao de estabilidade](#2-validacao-de-presenca de histerese e aprovacao de estabilidade)
            * [3 Validacao de estabilildade:](#3-validacao-de-estabilildade)
         * [Atualizacao das maiores e menos temperaturas](#atualizacao-das-maiores-e-menos-temperaturas)
         * [Tratamento da buzina e do LED](#tratamento-da-buzina-e-do-led)
         * [Impressao e escrita na tela do Display](#impressao-e-escrita-na-tela-do-display)
         * [Logica de scrolling (rolagem) da segunda linha](#logica-de-scrolling-da-segunda-linha)

         
<!--te-->



Componentes utilizados:
=======================


1. Placa Arduino Uno;
2. Display LCD 16x2 verde;
3. Módulo I2C;
4. Sensor de temperatura LM35;
5. Módulo buzzer;
6. LED RGB.


Sobre o trabalho
================

* Temperatura deve ser apresentada com duas casas decimais.
* Mostrar as temperaturas máximas e mínimas registradas assim que o projeto de termômetro for ligado.
* Buzina deve acionar quando o termômetro detectar uma temperatura fora de uma **faixa de controle**.
* A faixa de controle está entre 25.20°C e 32.60°C.





Notas sobre a logica envolvida
==============================

#### Busca por estabilidade
  Uma grande preocupação ao se fazer esse código foi a busca por estabilidade no valor apresentado. O sensor, quando diante de uma temperatura constante, envia valores correspondentes de temperatura ao Arduino com certas variações em torno de uma temperatura média. Quando o termômetro deste projeto se encontra nesta estado, que eu chamo de histere, apresenta-se a temperatura que é a maioria num conjunto de 6 últimas temperaturas coletadas, conjunto esse que é continuamente atualizado a cada iteração do void loop.

#### Scroll apenas na segunda linha
  A primeira linha do display 16x2 é usada para apresentar a temperatura atual da medida do sensor, enquanto a sedunda linha é usada para mostrar a maior e a menor temperatura já registrada desde o momento em que o projeto termômetro foi ligado. Como são usadas duas casas decimais, foi necessário a implementação de uma lógica que rolasse apenas a segunda linha do display.
  
#### Transição de entrada e saída
  A buzina deverá acionarar quando o termômetro estiver fora da **faixa de controle**. Há também um LED RGB que complementa esse alerta: dentro da faixa, o LED fica verde; fora da faixa, o led fica vermelho (temperatura acima de 32.60°C) ou fica azul (temperatura abaixo de 25.20°C). Para entendimento da código, é interessane notar a diferença entre o momento em que a temperatura sai da faixa de controle (o que chamo de transição de saída) e o momento em que a temperatura entra na faixa (transição de entrada).
  
  
  
  
  
  
  

O Codigo
========

```c++

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


 
 
                 /* ***************************  aqui começa o tratamento da buzina e para o LED ***************************** */
                 
                 
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
  else if (amortecimento > 0) amortecimento-- ;   
  
                             
                                         /* *********************** aqui que termina o tratamento da buzina  **********************  */






    
  lcd.setCursor(0, 0);                /*  É nesta linha que será apresentada a temperatura atual medida pelo sensor lm35.  */
  lcd.print("  Temp:" + String(temp) + " C"); 
  lcd.setCursor(12, 0);
  lcd.write(1);  
  
  lcd.setCursor(0, 1);               /* Para a impressão da linha 2, a linha inferior.  */  
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

 ```
 
 
 


Declaracao das variaveis
------------------------

```c++

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

 ```
 
* É feita a declaração das variáveis.
* inclusão de bibliotecas por causa do modulo I2C, apenas.
* as declarações de arrays do tipo *byte* vistas acima servirão para a criação de caractes customizados no LCD (símbolo grau, quadrado oco e quadrado cheio).








void setup
==========



Definicao das variaveis e pinos:
--------------------------------

 
```c++

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
  lcd.createChar(1, grau);              /* caractere customizado grau criado. Será chamado, na perspectiva do LCD, de 1. */
  lcd.createChar(2, quadradoOco);        /* carctere customizado com forma de quadrado oco criado. Será chamado, na perspectiva do LCD, de 2. */
  lcd.createChar(3, quadradoCheio);         /* carctere customizado com forma de quadrado cheio criado. Será chamado, na perspectiva do LCD, de 3. */
  

```
  Aqui foi finalmente criado os 3 caracteres customizados. Lembrando que caracteres customizados *não são impressos* no LCD (como os tradicionais números e letras). Eles *são escritos* no LCD, para que assim eles possam aparecer no display. 







Apresentacao inicial e animada no LCD:
--------------------------------------

```c++


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

```
  Apenas uma animação inicial em que é mostrado nome da equipe. Lembra como se fosse um arquivo se carregando.
  
  





void loop
=========


Medicao da temperatura:
-----------------------

```c++

/*   ************************************************************** void loop ***********************************************************************************  
 *    ********************************************************************************************************************************************************* */
void loop()         
{ 


  temp = ( 5.0 * analogRead(lm35) * 100.0) / 1024.0;               /*  medindo temperatura   */
  delay(300);                                                   /*  delay de tempo após cada medição de temperatura    */
  
```
Calculo simples para converter o sinal analógico do LM35 para graus Celsius.


Voltar para o [Sumario](#sumario)?




Tratamento da Estabilidade
==========================

  Esse tratamento foi levado muito em conta neste projeto. Ele pode ser subdividido em 3 parte:
  1. Verificação de possível histerese (porta de entrada para a estabilidade) ;
  2. Validação de presença de histerese e aprovação de estabilidade (confere se vale a pena entrar na estabilidade);
  3. Validação de estabilidade (a estabilidade é dada aqui. Também é aqui a saída da estabilidade).




1 Verificacao de possivel histerese:
-------------------------------------

```c++

/*  ************************************* Começa o tratamento para a estabilidade da apresentação da temperatura **********************************************  */

  if(!histerese)                                /*  tratamento para uma variação rápida de temperatura.   */
    if(temp == temperatura[0])  histerese = true;     /* o temperatura[0] é referente à medição anterior  */
    else temperatura[0] = temp;           /*agora o temperatura[0] se referirá à temperatura atual  */

```    
  Quando a última temperatura medida pelo lm35 for igual à temperatura medida na iteração anterior do void loop, considera-se que há uma possível histerese. 
  Quando o sensor lm35 fica em um ambiente de temperatura constante, a temperatura fornecida pelo sensor ao arduino costuma oscilar um pouco em torno de uma temperatura central, caracterizando essa espécie de histerese térmica. Um dos valores, entre os demais durante a histerese, fornecidos pelo sensor acaba aparacendo com mais frequência, e é esse valor que se objetiva apresentar no display.




2 Validacao de presenca de histerese e aprovacao de estabilidade:
-----------------------------------------------------------------

```c++
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

```
  Sempre que há suspeita de histerese (supeita de se estar diante de uma temperatura constante), a temperatura mensurada pelo sensor é registrada num conjunto que tem até 6 amostras de temperatura, sendo 5 delas provenientes de iterações anteriores e a primeira referente à medição presente.
  Se houver 3 amostras iguais nesse conjunto de 6 amostras, a condição de estabilidade é aprovada **_(o grau de dificuldade para a entrada da estalidade é de 3 em 6)_**.
  Prosseque-se então para a parte seguinte do código, já que a variável **estavel** passa a ser **true**.  
  

  
  
3 Validacao de estabilildade:
-----------------------------

```c++
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

```
  Depois de entrado na estabilidade, a próxima iteração do código (próxima leitura do sensor e julgamento do valor medido) irá direto para esta região específica do código fonte. 
  Esta região sempre faz uma conferência de validação da estabilidade. Se não houver pelo menos 4 amostras iguais num conjunto de 6 últimas amostras coletadas, a condição de estabilidade é desfeita **_(o grau de facilidade para a saída da estabilidade)_**  , o Display mostra _livremente_ a temperatura medida pelo LM35.  




Conclusão:
* **variável histerese** quando **false** significa ou que não há nem suspeita de histerese (display está liberado para apresentar qualquer variação do LM35).
* **variável histerese** quando **true** significa ou que há suspeita de histerese, mas um valor estável não é imposto (forçado) no display; ou porque está na condição de estabilidade, já aí o display é forçado a mostrar um valor estável.  
* **variável estavel** quando **false** significa que a condição de estabilidadde ainda não foi alcançada. O display está *solto/livre* pra apresentar qualquer valor do LM35.
* **variável estável** quando **true** significa que a condição de estabilidade foi alcançada, e o display está *preso/obrigado* a apresentar um valor que é maioria num conjunto de 6 últimas amostras coletadas.





## O uso dessa metodologia se mostrou satisfatória.
  Pois assim o display apresentou um bom equilíbrio entre apresentar um valor "sossegado" quando não há mudanças de temperatura, e apresentar as mudanças de temperatura com certa rapidez. Uma vez que buscar muita estabilidade pode acabar "predendo" demais o display, trazendo certa latência (atrazo de resposta) quanto às mudanças de temperatura que podem vir a ocorrer após um momento de estabilidade.
  Lembrando que o delay entre as leituras do lm35, neste projeto, foi de 300 milissegundos. 
  Esse valor "sossegado" de vez em quando sofre certas variações, mesmo estando na histese (em um ambiente de temperatura constante). O que no meu ponto de vista não chega a ser algo ruim, dando até certo *charme*.
  Caso se queira eliminar esse *charme*, basta dificultar **_ o grau de facilidade para a saída da estabilidade _** (ao invez de se exigir 4 amostras iguais, exige-se apenas 2 amostras coincidentes como condição de continuação de estabilidade, "segurando" o valor no display por mais tempo). 







Atualizacao das maiores e menos temperaturas 
============================================

```c++
  if (temp > maxTemp) maxTemp = temp;         /*atualiza as temperaturas max e min já sentidas pelo sensor */
  else if (temp < minTemp) minTemp = temp; 

```

  É Verificado se a temperatura medida pela atual iteração ultrapassa os extremos já registrados pelo projeto enquanto esteve ligado.







Tratamento da buzina e do LED
=============================


```c++

                 /* ***************************  aqui começa o tratamento da buzina e para o LED ***************************** */
                 
                 
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
  else if (amortecimento > 0) amortecimento-- ;   
  
                             
                                         /* *********************** aqui que termina o tratamento da buzina  **********************  */

```



* A faixa de temperatura de controle está entre 32.6°C e 25.2°C.
* Gosto de dizer, a título de organização das ideias, que quando a temperatura sai da faixa de controle ocorre uma _transição de saída_. E, quando a temperatura entra na faixa de controle, ocorre uma _transição de entreda_.


| O que?                 |  Quando isso acontece?                              | O que vai acontecer                       |
|------------------------|-----------------------------------------------------|-------------------------------------------|
| _Transição de saída_   |  quando a temperatura sai da faixa de controle      | aciona buzina e LED fica vermelho ou azul |
| _Transição de entrada_ |  quando a temperatura entra na faixa de controle    | desliga buzina e LED fica verde           |


* Foi feita uma lógica para se evitar que a buzina fique tocando por razões de amortecimento.
* Ao se colocar no sensor gelo e ferro de solda, ocorre a transiçao de saída, como esperado. 
* Ao retirar o gelo ou o ferro de solda, ocorre a transição de entrada acompanhada com um amortecimento.
* Aliás, a demostração do trabalho no dia da apresentação vai ser dessa forma. Assim, achei útil achar uma rápida solução para esse amortecimento.
* Quando digo amortecimento, estou querendo dizer sobre grandes oscilações decrescentes nos dados do sensor que são traduzidos como temperatura fora e dentro da faixa de controle.
* Um zigue e zague entre temperaturas dentro e fora do controle. Várias _transições de saída e de entrada_ intercaladas em um intervalo de tempo muito curto.
* Um rápido e continuado liga e desliga da buzina.
* Foi por essa razão que a variável **_amortecimento_** foi criada.
* Quando acontece uma _transição de entrada_ (desligando a buzina e mudando a cor do LED), é necessário aguardar 20 iterações ( do void loop para se liberar o acionamento da buzina (e do LED). Caso ocorra uma _trancição de saída_ antes das 20 iterações, impõe-se uma nova espera de 16 iterações. Se ocorrer outra _transição de saída_ antes da espera imposta, a espera é renovada com o 16 iterações; e assim por diante.






Impressao e escrita na tela do Display
======================================

```c++

    
  lcd.setCursor(0, 0);                /*  É nesta linha que será apresentada a temperatura atual medida pelo sensor lm35.  */
  lcd.print("  Temp:" + String(temp) + " C"); 
  lcd.setCursor(12, 0);
  lcd.write(1);  
  
  lcd.setCursor(0, 1);               /* Para a impressão da linha 2, a linha inferior.  */  
  lcd.print((" Max:"+String(maxTemp)+" C  Min:"+String(minTemp)+" C ").substring(comecoVisivel));   
  lcd.setCursor(posicaoGrauMax+corretor, 1);
  lcd.write(1);
  lcd.setCursor(posicaoGrauMin+corretor, 1);
  lcd.write(1);

```

  Impressão da primeira e da segunda linha no Display. Lembrando que a segunda linha é móvel. 
  Frases são impressas. Caracteres customizados são escritos.
  A variável **comecoVisivel** é usada para indicar qual pate da frase que vai ser impressa no começo Display (da segunda linha, claro). A frase é sempre a mesma, o que muda é a porção que vai ser apresentada na tela.
  O símbolo grau, um caractere que nós customizamos e o chamamos de **1**, é escrito na tela com auxílio da variável **corretor**, já que a linha 2 é uma linha móvel. 
  A **posicaoGrauMax** e a **posicaoGrauMin** são as posições dos símbolos grau assim que se energiza o projeto. Seus valores são constantes.
  Apenas as variáveis **comecoVisivel** e **corretor** é que mudam continuamente (ainda não seja no mesmo ritmo que as iterações do void loop). A variação dessas duas variáveis são regidas pelo lógica de scrolling que é explicada no tópico seguinte (na parte seguinte do código).
  







Logica de scrolling da segunda linha
====================================

```c++

if( !velocidadeLinhaInferior ){  
  if(comecoVisivel == 10){       /* verifica se já é hora de mudar o sentido de deslocamento da linha inferior.  */
    direcao = false;        /* troca de sentido  */
    delay(500);          /* pequena pausa charmosa na hora de trocar de direção  */
  }
  else if(comecoVisivel == 0){
    direcao = true;         /* troca de sentido  */
    delay(500);          /* para dar uma pequena pausa assim que mudar de direção  */
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

/*   **********************************************************  Acava o void loop ***************************************************************************  * 
 *   *********************************************************  e o programa também  ************************************************************************  */


```



  **velocidadeLinhaInferior** e **velocidadeDisplay**  mexem com a velocidade de rolagem da linha inferior. Sua lógica funciona assim: neste programa, **velocidadeDisplay = 2**, o que significa que a impressão e escrita do display muda de lugar a cada 3 iterações do **_void loop_**. Então velocidade de scrolling depende também do tempo de cada iteração do void loop, que é controlada pelo **_delay_** da _medição de temperatura_ (lá no primeiro tópico do _void loop_).
  
  



  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
