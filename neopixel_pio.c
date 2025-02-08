#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "ws2818b.pio.h"

#define LED_COUNT 25
#define LED_PIN 7

#define joystick_x 27
#define joystick_y 26
#define botao_a 5
#define botao_b 6

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
}

int ind[7][5]={
  {-1, -1, -1, -1, -1},
  {24, 23, 22, 21, 20},
  {15, 16, 17, 18, 19},
  {14, 13, 12, 11, 10},
  {5, 6, 7, 8, 9},
  {4, 3, 2, 1, 0},
  {-1, -1, -1, -1, -1}
};

int led[7][5]={
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

void setLEDColor(int i, int j, int r, int g, int b){
  int index = ind[i][j];
  npSetLED(index, r, g, b);
  return;
}

void drawColor(int i, int j){
  const uint c=led[i][j];
  if(c==0) setLEDColor(i, j, 0, 0, 0);            // apagado
  else if(c==1) setLEDColor(i, j, 50, 0, 0);     // vermelho
  else if(c==2) setLEDColor(i, j, 0, 50, 0);     // verde
  else if(c==3) setLEDColor(i, j, 0, 0, 50);     // azul
  else if(c==4) setLEDColor(i, j, 130, 120, 0);   // amarelo
  else if(c==5) setLEDColor(i, j, 127, 20, 20);   // rosa
  else if(c==6) setLEDColor(i, j, 50, 0, 50);   // roxo
  else if(c==7) setLEDColor(i, j, 127, 20, 0);   // laranja
  else printf("caractere nao corresponde a nenhuma cor\n");
  return;
}

void writeColor(){
  for(int i=1; i<6; i++){
    for(int j=0; j<5; j++) drawColor(i, j);
  }
  npWrite();
  return;
}

int x, y;
int speed=1000;
bool piso=false;

void gerarBloco(){
  x=0, y=2;
  int cor=(rand()%7)+1;
  led[x][y]=cor;
  piso=false;
}

void paraBaixo(){
  if(led[x+1][y]==0){
    led[x+1][y]=led[x][y];
    led[x][y]=0;
    writeColor();
    x+=1;
    printf("atual em: (%d, %d), valor de atual: %d\n", x, y, led[x][y]);
    piso=false;
  }else{
    printf("chegou no piso\n");
    piso=true;
  }
  return;
}

void paraDireita(){
  if(y<=3 && led[x][y+1]==0){
    led[x][y+1]=led[x][y];
    led[x][y]=0;
    y+=1;
    writeColor();
  }
  return;
}

void paraEsquerda(){
  if(y>=1 && led[x][y-1]==0){
    led[x][y-1]=led[x][y];
    led[x][y]=0;
    y-=1;
    writeColor();
  }
}

int main() {

  stdio_init_all();
  gpio_init(joystick_x);
  gpio_init(botao_a);
  gpio_init(botao_b);
  gpio_set_dir(joystick_x, GPIO_IN);
  gpio_set_dir(botao_a, GPIO_IN);
  gpio_set_dir(botao_b, GPIO_IN);
  gpio_pull_up(botao_a);
  gpio_pull_up(botao_b);
  srand(21);

  npInit(LED_PIN); // Inicializa a matriz de LEDs
  npClear();

  writeColor();

  gerarBloco();

  sleep_ms(1000);

  while (true) {

    while(!piso){
      int a=gpio_get(botao_a);
      int b=gpio_get(botao_b);
      if(a && !b) paraEsquerda();
      if(!a && b) paraDireita();
      paraBaixo();
      sleep_ms(speed);
    }
    gerarBloco();
    sleep_ms(speed);

  }
}
