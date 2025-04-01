#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>
#include "pico/stdlib.h"
#include "pico/time.h"
#include "hardware/pio.h"
#include "hardware/gpio.h"
#include "hardware/timer.h"
#include "hardware/clocks.h"
#include "ws2818b.pio.h"

#define botao_a 5
#define botao_b 6
#define botao_c 22

///////////-------Configurações da matriz de LEDs---------------Linhas 20 a 73------///////////////
#define LED_COUNT 25
#define LED_PIN 7

struct pixel_t {
  uint8_t G, R, B;
};
typedef struct pixel_t pixel_t;
typedef pixel_t npLED_t; 

npLED_t leds[LED_COUNT];

PIO np_pio;
uint sm;

void npInit(uint pin) {

  uint offset = pio_add_program(pio0, &ws2818b_program);
  np_pio = pio0;

  sm = pio_claim_unused_sm(np_pio, false);
  if (sm < 0) {
    np_pio = pio1;
    sm = pio_claim_unused_sm(np_pio, true);
  }

  ws2818b_program_init(np_pio, sm, offset, pin, 800000.f);

  for (uint i = 0; i < LED_COUNT; ++i) {
    leds[i].R = 0;
    leds[i].G = 0;
    leds[i].B = 0;
  }
}

void npSetLED(const int index, const uint8_t r, const uint8_t g, const uint8_t b) {
  leds[index].R = r;
  leds[index].G = g;
  leds[index].B = b;
}

void npClear() {
  for (int i = 0; i < LED_COUNT; ++i)
    npSetLED(i, 0, 0, 0);
}

void npWrite() {
  for (int i = 0; i < LED_COUNT; ++i) {
    pio_sm_put_blocking(np_pio, sm, leds[i].G);
    pio_sm_put_blocking(np_pio, sm, leds[i].R);
    pio_sm_put_blocking(np_pio, sm, leds[i].B);
  }
  sleep_us(100);
  return;
}
///////////-------Configurações da matriz de LEDs---------------Linhas 20 a 73------///////////////

volatile int ind[7][5]={  //matriz para converter a matriz led[7][5] para o buffer
  {-1, -1, -1, -1, -1},
  {24, 23, 22, 21, 20},
  {15, 16, 17, 18, 19},
  {14, 13, 12, 11, 10},
  {5, 6, 7, 8, 9},
  {4, 3, 2, 1, 0},
  {-1, -1, -1, -1, -1}
};

volatile int led[7][5]={  //matriz que representa o estado atual do jogo
  {0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0},
  {0, 0, 0, 0, 0},
  {11, 11, 11, 11, 11}
};

// 0 apagado
// 1 vermelho
// 2 verde
// 3 azul
// 4 amarelo
// 5 rosa
// 6 roxo
// 7 laranja
// 8 branco

void setLEDColor(int i, int j, int r, int g, int b){  //seta a cor de um LED da matriz
  int index = ind[i][j];
  npSetLED(index, r, g, b);
  return;
}

void drawColor(int i, int j){                         //seta a cor de um elemento da matriz
  const uint c=led[i][j];
  if(c==0) setLEDColor(i, j, 0, 0, 0);                // apagado
  else if(c==1) setLEDColor(i, j, 50, 0, 0);          // vermelho
  else if(c==2) setLEDColor(i, j, 0, 50, 0);          // verde
  else if(c==3) setLEDColor(i, j, 0, 0, 50);          // azul
  else if(c==4) setLEDColor(i, j, 130, 120, 0);       // amarelo
  else if(c==5) setLEDColor(i, j, 127, 20, 20);       // rosa
  else if(c==6) setLEDColor(i, j, 50, 0, 50);         // roxo
  else if(c==7) setLEDColor(i, j, 127, 20, 0);        // laranja
  else if(c==8) setLEDColor(i, j, 127, 127, 127);     //branco
  else printf("cor invalida\n");
  return;
}

void writeColor(){                                    //seta a cor de todos os elementos da matriz
  for(int i=1; i<6; i++){
    for(int j=0; j<5; j++) drawColor(i, j);
  }
  npWrite();
  sleep_ms(10);
  return;
}

volatile int x, y;                //coordenadas do bloco atual
volatile int speed=500;           //velocidade do jogo
int pontos=0;                     //pontuação
volatile bool piso=false;           //variável que indica se chegou o fim da matriz  
volatile bool intf=false;           //flag interrupção GPIO
volatile bool flagBotao_a=false;    //flags dos botões
volatile bool flagBotao_b=false;
volatile bool flagBotao_c=false;

void int_botao(uint gpio, uint32_t events){     //tratamento da interrupção dos botões
  intf=true;
  if(gpio==botao_a) flagBotao_a=true;
  if(gpio==botao_b) flagBotao_b=true;
  if(gpio==botao_c) flagBotao_c=true;
  return;
}

void setup(){                         //inicializações gerais e dos botões
  stdio_init_all();

  gpio_init(botao_a);                 //configurações botao a
  gpio_set_dir(botao_a, GPIO_IN);
  gpio_pull_up(botao_a);

  gpio_init(botao_b);                 //configurações botao b
  gpio_set_dir(botao_b, GPIO_IN);
  gpio_pull_up(botao_b);

  gpio_init(botao_c);                 //configurações botao c
  gpio_set_dir(botao_c, GPIO_IN);
  gpio_pull_up(botao_c);
  
  gpio_set_irq_enabled_with_callback(botao_a, GPIO_IRQ_EDGE_FALL, true, &int_botao);
  gpio_set_irq_enabled_with_callback(botao_b, GPIO_IRQ_EDGE_FALL, true, &int_botao);
  gpio_set_irq_enabled_with_callback(botao_c, GPIO_IRQ_EDGE_FALL, true, &int_botao);
}

void gerarBloco(){          //gera um bloco de cor aleatória na linha zero e coluna 2 da matriz
  x=0, y=2;
  int cor=(rand()%7)+1;
  led[x][y]=cor;
  piso=false;
  return;
}

void paraBaixo(){             //move o bloco atual para baixo
  if(led[x+1][y]==0){
    led[x+1][y]=led[x][y];
    led[x][y]=0;
    writeColor();
    x+=1;
    //printf("atual em: (%d, %d), valor de atual: %d\n", x, y, led[x][y]);
    piso=false;
  }else{
    piso=true;
  }
  return;
}

void paraDireita(){                 //move o bloco atual para direita
  if(y<=3 && led[x][y+1]==0){
    led[x][y+1]=led[x][y];
    led[x][y]=0;
    y+=1;
    writeColor();
  }
  return;
}

void paraEsquerda(){                  //move o bloco atual para esquerda
  if(y>=1 && led[x][y-1]==0){
    led[x][y-1]=led[x][y];
    led[x][y]=0;
    y-=1;
    writeColor();
  }
  return;
}

bool componente[10][10];
bool visitado[10][10];
int tamanho_comp=0;

void buscar_componente(int i, int j, int cor){      //encontra uma componente de blocos e armazena resultados em componente e tamanho_comp
  componente[i][j]=true;
  visitado[i][j]=true;
  tamanho_comp++;
  
  int x1=i-1, y1=j;
  int x2=i, y2=j+1;
  int x3=i+1, y3=j;
  int x4=i, y4=j-1;

  if(x1>=1 && x1<=6 && y1>=0 && y1<=5) if(!visitado[x1][y1] && led[x1][y1]==cor) buscar_componente(x1, y1, cor);
  if(x2>=1 && x2<=6 && y2>=0 && y2<=5) if(!visitado[x2][y2] && led[x2][y2]==cor) buscar_componente(x2, y2, cor);
  if(x3>=1 && x3<=6 && y3>=0 && y3<=5) if(!visitado[x3][y3] && led[x3][y3]==cor) buscar_componente(x3, y3, cor);
  if(x4>=1 && x4<=6 && y4>=0 && y4<=5) if(!visitado[x4][y4] && led[x4][y4]==cor) buscar_componente(x4, y4, cor);
  return;
}

void reseta_componente(){         //reseta valores encontrados por buscar_componente
  for(int i=0; i<7; i++){
    for(int j=0; j<7; j++){
      componente[i][j]=false;
      visitado[i][j]=false;
    } 
  }
  tamanho_comp=0;
}

void apaga_componente(){            //apaga componente que foi encontrada
  for(int i=1; i<6; i++){
    for(int j=0; j<5; j++){
      if(componente[i][j]) led[i][j]=8;
    }
  }
  writeColor();
  sleep_ms(150);
  for(int i=1; i<6; i++){
    for(int j=0; j<5; j++){
      if(componente[i][j]){
        for(int k=i-1; k>0; k--) led[k+1][j]=led[k][j];
      }
    }
  }
  writeColor();
  sleep_ms(150);
}

void varredura(){               //percorre toda a matriz e faz buscas de componentes nos elementos
  for(int i=1; i<6; i++){
    for(int j=0; j<5; j++){
      int cor=led[i][j];
      if(cor){
        reseta_componente();
        buscar_componente(i, j, cor);
        if(tamanho_comp>=3){
          pontos+=tamanho_comp;
          printf("PONTUACAO:   %d\n", pontos);
          apaga_componente();
          varredura();
        } 
      }
    }
  }
}

bool tela_cheia(){      //retorna true se não há mais espaço vazio na matriz de LEDs
  if(x==0 && y==2) return true;
  if(led[1][1] && led[1][2] && led[1][3]) return true;
  return false;
}

void derrota(){               //animação da tela de derrota
  for(int i=5; i>=1; i--){
    for(int j=0; j<5; j++){
      if(led[i][j]){
        led[i][j]=8;
        writeColor();
        sleep_ms(150);
      }
    }
    for(int j=0; j<5; j++){
      led[i][j]=0;
      writeColor();
      sleep_ms(150);
    }
  }
}

int main() {

  //inicializações
  setup();
  srand(get_absolute_time()); //gera seed para rand()
  npInit(LED_PIN); // Inicializa a matriz de LEDs
  npClear();
  writeColor();
  gerarBloco();
  sleep_ms(1000);
  reseta_componente();

  while (true) {

    while(!piso && !intf){  //comportamento quando nada eh acionado
      paraBaixo();
      sleep_ms(speed);
      writeColor();
      sleep_ms(speed);
    }
    if(piso){               //se chegou no piso
      if(tela_cheia()){
        derrota();
        pontos=0;
      }
      varredura();
      gerarBloco();         //gera um novo bloco
      sleep_ms(speed);
    }
    else if(intf){          //interrupção gerada
      intf=false;
      if(flagBotao_a){      //pelo botão A
        flagBotao_a=false;
        paraEsquerda();
      }
      if(flagBotao_b){      //pelo botão B
        flagBotao_b=false;
        paraDireita();
      }
      if(flagBotao_c){      //pelo botão C
        flagBotao_c=false;
        paraBaixo();
      }

      sleep_ms(50);  
      writeColor();
      sleep_ms(50);
    }
  }
}
