
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
#define PINO_BOTAO 3

#define INTERVALO_MOVIMENTO 30
#define VELOCIDADE_ANIMACAO 4
#define ESPACO_FILA 40

#define CENTRO_X ((LARGURA - LARGURA_SPRITE) / 2)

Adafruit_SSD1306 display(
  LARGURA,
  ALTURA,
  &Wire,
  OLED_RESET
);

// =====================================
// BACKGROUND
// =====================================

// Ponteiros para o fundo selecionado
const uint8_t* fundoSuperiorAtual;
const uint8_t* fundoInferiorAtual;

// Seleciona o background no setup
void configurarFundo() {
  fundoSuperiorAtual = fundoSuperior;
  fundoInferiorAtual = fundoInferior;
}

// Desenha as duas faixas
void desenharFundo() {

  display.drawBitmap(
    0, 0,
    fundoSuperiorAtual,
    LARGURA_FUNDO,
    ALTURA_FAIXA,
    SSD1306_WHITE
  );

  display.drawBitmap(
    0, 48,
    fundoInferiorAtual,
    LARGURA_FUNDO,
    ALTURA_FAIXA,
    SSD1306_WHITE
  );
}

// =====================================
// ESTRUTURA DA FILA
// =====================================

struct Animacao {
  PersonagemID id;
  int x;
  int xInicial;
  int y;
};

Animacao fila[MAX_PERSONAGENS];

uint8_t quantidadeFila = 0;

bool personagemSaindo = false;

// =====================================
// CONTROLE DO BOTÃO
// =====================================

bool ultimaLeituraBotao = HIGH;
bool estadoEstavelBotao = HIGH;

unsigned long tempoDebounce = 0;

#define DEBOUNCE_MS 40

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

  if (quantidadeFila >= MAX_PERSONAGENS) {
    Serial.println("Fila cheia!");
    return;
  }

  fila[quantidadeFila].id = id;
  fila[quantidadeFila].x = x;
  fila[quantidadeFila].xInicial = x;
  fila[quantidadeFila].y = y;

  quantidadeFila++;
}

// =====================================
// INICIAR FILA
// =====================================

void iniciarFila() {

  quantidadeFila = 0;
  personagemSaindo = false;

  adicionarPersonagem(BALAZAR, -32, 16);
  adicionarPersonagem(DIANA, -72, 16);
  adicionarPersonagem(KEYLETH, -112, 16);
  adicionarPersonagem(MILO, -152, 16);
  adicionarPersonagem(FRITZ, -192, 16);
  adicionarPersonagem(INIMIGO, -232, 16);
}

// =====================================
// REMOVER PRIMEIRO PERSONAGEM
// =====================================

void removerPrimeiro() {

  for (int i = 0; i < quantidadeFila - 1; i++) {
    fila[i] = fila[i + 1];
  }

  if (quantidadeFila > 0) {
    quantidadeFila--;
  }
}

// =====================================
// AVANÇAR INICIATIVA
// =====================================

void avancarFila() {

  if (quantidadeFila == 0 || personagemSaindo) {
    return;
  }

  if (fila[0].x < CENTRO_X) {
    Serial.println("Aguarde o personagem chegar ao centro!");
    return;
  }

  personagemSaindo = true;

  Serial.println("Personagem saindo!");
}

// =====================================
// LER BOTÃO
// =====================================

void lerBotao() {

  unsigned long agora = millis();

  bool leitura = digitalRead(PINO_BOTAO);

  if (leitura != ultimaLeituraBotao) {
    ultimaLeituraBotao = leitura;
    tempoDebounce = agora;
  }

  if (agora - tempoDebounce >= DEBOUNCE_MS) {

    if (leitura != estadoEstavelBotao) {

      estadoEstavelBotao = leitura;

      if (estadoEstavelBotao == LOW) {
        avancarFila();
      }
    }
  }
}

// =====================================
// DESENHAR PERSONAGEM
// =====================================

void desenharPersonagem(int indice, int frame) {

  uint8_t id = static_cast<uint8_t>(fila[indice].id);

  display.drawBitmap(
    fila[indice].x,
    fila[indice].y,
    sprites[id][frame],
    LARGURA_SPRITE,
    ALTURA_SPRITE,
    SSD1306_WHITE
  );
}

// =====================================
// ATUALIZAR SISTEMA
// =====================================

void atualizarSistema() {

  static unsigned long ultimoFrame = 0;

  unsigned long agora = millis();

  if (agora - ultimoFrame < INTERVALO_MOVIMENTO) {
    return;
  }

  ultimoFrame = agora;

  display.clearDisplay();

  // Desenha o background primeiro
  desenharFundo();

  // =================================
  // PERSONAGEM ESTÁ SAINDO
  // =================================

  if (personagemSaindo) {

    fila[0].x++;

    if (fila[0].x >= LARGURA) {

      removerPrimeiro();

      personagemSaindo = false;

      if (quantidadeFila == 0) {
        iniciarFila();
      }
    }
  }

  // =================================
  // DESENHAR SAÍDA
  // =================================

  if (personagemSaindo) {

    int frameAtual =
      1 + ((fila[0].x - fila[0].xInicial) /
      VELOCIDADE_ANIMACAO) % 2;

    desenharPersonagem(0, frameAtual);

    for (int i = 1; i < quantidadeFila; i++) {
      desenharPersonagem(i, 1);
    }
  }

  // =================================
  // MOVIMENTO NORMAL
  // =================================

  else {

    for (int i = 0; i < quantidadeFila; i++) {

      int destinoX = CENTRO_X - (i * ESPACO_FILA);

      bool andando = fila[i].x < destinoX;

      if (andando) {
        fila[i].x++;
      }

      int frameAtual;

      if (i == 0 && fila[i].x >= CENTRO_X) {
        // Primeiro personagem olhando para frente
        frameAtual = 0;
      }
      else if (andando) {
        // Caminhada: frames 2 e 3
        frameAtual =
          1 + ((fila[i].x - fila[i].xInicial) /
          VELOCIDADE_ANIMACAO) % 2;
      }
      else {
        // Espera de perfil
        frameAtual = 1;
      }

      desenharPersonagem(i, frameAtual);
    }
  }

  display.display();
}

// =====================================
// SETUP
// =====================================

void setup() {

  Serial.begin(115200);

  Wire.begin(8, 9);

  pinMode(PINO_BOTAO, INPUT_PULLUP);

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

  // Configura o background
  configurarFundo();

  // Inicializa a fila
  iniciarFila();

  // Exibe o fundo imediatamente
  desenharFundo();
  display.display();
}

// =====================================
// LOOP PRINCIPAL
// =====================================

void loop() {

  lerBotao();
  atualizarSistema();

  // Futuramente:
  // ArduinoCloud.update();

}
