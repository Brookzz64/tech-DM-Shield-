
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "sprites.h"

// =====================================
// CONFIGURAÇÕES
// =====================================

#define LARGURA 128
#define ALTURA 64
#define OLED_RESET -1
#define OLED_ENDERECO 0x3C

#define MAX_PERSONAGENS 20
#define INTERVALO_MOVIMENTO 30
#define VELOCIDADE_ANIMACAO 4
#define ESPACO_FILA 40

Adafruit_SSD1306 display(
  LARGURA,
  ALTURA,
  &Wire,
  OLED_RESET
);

// =====================================
// ESTRUTURA DOS PERSONAGENS
// =====================================

struct Animacao {
  PersonagemID id;
  int x;
  int xInicial;
  int y;
  bool ativo;
};

Animacao fila[MAX_PERSONAGENS] = {};

// =====================================
// ADICIONAR PERSONAGEM
// =====================================

void adicionarPersonagem(PersonagemID id, int x, int y) {

  uint8_t identificador = static_cast<uint8_t>(id);

  const uint8_t totalSprites =
    sizeof(sprites) / sizeof(sprites[0]);

  if (identificador >= totalSprites) {
    Serial.println("Personagem nao encontrado!");
    return;
  }

  for (int i = 0; i < MAX_PERSONAGENS; i++) {

    if (!fila[i].ativo) {

      fila[i].id = id;
      fila[i].x = x;
      fila[i].xInicial = x;
      fila[i].y = y;
      fila[i].ativo = true;

      return;
    }
  }

  Serial.println("Fila de personagens cheia!");
}

// =====================================
// INICIAR / REINICIAR FILA
// =====================================

void iniciarFila() {

  // Libera todas as posições
  for (int i = 0; i < MAX_PERSONAGENS; i++) {
    fila[i].ativo = false;
  }

  // Adiciona os personagens em sequência
  adicionarPersonagem(BALAZAR, -32, 16);
  adicionarPersonagem(DIANA, -32 - ESPACO_FILA, 16);
  adicionarPersonagem(KEYLETH, -32 - ESPACO_FILA * 2, 16);
  adicionarPersonagem(MILO, -32 - ESPACO_FILA * 3, 16);
  adicionarPersonagem(FRITZ, -32 - ESPACO_FILA * 4, 16);
  adicionarPersonagem(INIMIGO, -32 - ESPACO_FILA * 5, 16);
}

// =====================================
// ATUALIZAR ANIMAÇÕES
// =====================================

void atualizarSistema() {

  static unsigned long ultimoFrame = 0;

  unsigned long agora = millis();

  if (agora - ultimoFrame < INTERVALO_MOVIMENTO) {
    return;
  }

  ultimoFrame = agora;

  display.clearDisplay();

  bool filaVazia = true;

  for (int i = 0; i < MAX_PERSONAGENS; i++) {

    if (!fila[i].ativo) {
      continue;
    }

    // Movimenta o personagem
    fila[i].x++;

    // Verifica se saiu completamente da tela
    if (fila[i].x >= LARGURA) {
      fila[i].ativo = false;
      continue;
    }

    filaVazia = false;

    // Alterna entre os frames 2 e 3
    int frameAtual =
      1 + ((fila[i].x - fila[i].xInicial) /
      VELOCIDADE_ANIMACAO) % 2;

    uint8_t id = static_cast<uint8_t>(fila[i].id);

    display.drawBitmap(
      fila[i].x,
      fila[i].y,
      sprites[id][frameAtual],
      LARGURA_SPRITE,
      ALTURA_SPRITE,
      SSD1306_WHITE
    );
  }

  display.display();

  // Reinicia quando todos terminarem
  if (filaVazia) {
    iniciarFila();
  }
}

// =====================================
// SETUP
// =====================================

void setup() {

  Serial.begin(115200);

  Wire.begin(8, 9);

  if (!display.begin(
    SSD1306_SWITCHCAPVCC,
    OLED_ENDERECO
  )) {

    Serial.println("Erro ao iniciar OLED!");

    while (true) {
      delay(100);
    }
  }

  display.clearDisplay();
  display.display();

  iniciarFila();
}

// =====================================
// LOOP PRINCIPAL
// =====================================

void loop() {

  atualizarSistema();

  // Futuramente:
  // ArduinoCloud.update();

}
