
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#include "arduino_secrets.h"
#include "thingProperties.h"
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
// ESTRUTURA DOS PERSONAGENS
// =====================================

struct Animacao {
  PersonagemID id;
  int x;
  int y;
};

Animacao fila[MAX_PERSONAGENS];

PersonagemID ordemSalva[MAX_PERSONAGENS];

uint8_t quantidadeFila = 0;
uint8_t quantidadeOrdem = 0;

bool personagemSaindo = false;

// =====================================
// CONTROLE DO BOTÃO FÍSICO
// =====================================

bool ultimaLeituraBotao = HIGH;
bool estadoEstavelBotao = HIGH;

unsigned long tempoDebounce = 0;

#define DEBOUNCE_MS 40

// =====================================
// DESENHAR BACKGROUND
// =====================================

void desenharFundo() {

  int fundo = FundoSelecionado;

  // Proteção contra índices inválidos
  if (fundo < 0 || fundo >= TOTAL_FUNDOS) {
    fundo = 0;
  }

  // Faixa superior
  display.drawBitmap(
    0,
    0,
    fundosSuperiores[fundo],
    LARGURA_FUNDO,
    ALTURA_FAIXA,
    SSD1306_WHITE
  );

  // Faixa inferior
  display.drawBitmap(
    0,
    48,
    fundosInferiores[fundo],
    LARGURA_FUNDO,
    ALTURA_FAIXA,
    SSD1306_WHITE
  );
}

// =====================================
// IDENTIFICAR PERSONAGEM PELO NOME
// =====================================

bool identificarPersonagem(String nome, PersonagemID &id) {

  nome.trim();
  nome.toUpperCase();

  if (nome == "BALAZAR") {
    id = BALAZAR;
  }
  else if (nome == "DIANA") {
    id = DIANA;
  }
  else if (nome == "KEYLETH") {
    id = KEYLETH;
  }
  else if (nome == "MILO") {
    id = MILO;
  }
  else if (nome == "FRITZ") {
    id = FRITZ;
  }
  else if (nome == "INIMIGOG") {
    id = INIMIGOG;
  }
	else if (nome == "INIMIGOM") {
    id = INIMIGOM;
  }
	else if (nome == "INIMIGOP") {
    id = INIMIGOP;
  }
	else if (nome == "AZRIEL") {
    id = AZRIEL;
  }
	else if (nome == "BOB") {
    id = BOB;
  }
  else {
    return false;
  }

  return true;
}

// =====================================
// ADICIONAR PERSONAGEM
// =====================================

void adicionarPersonagem(PersonagemID id, int x, int y) {

  uint8_t identificador = static_cast<uint8_t>(id);

  const uint8_t totalSprites =
    sizeof(sprites) / sizeof(sprites[0]);

  if (identificador >= totalSprites ||
      quantidadeFila >= MAX_PERSONAGENS) {
    return;
  }

  fila[quantidadeFila].id = id;
  fila[quantidadeFila].x = x;
  fila[quantidadeFila].y = y;

  quantidadeFila++;
}

// =====================================
// MONTAR FILA A PARTIR DA ORDEM SALVA
// =====================================

void montarFila() {

  quantidadeFila = 0;
  personagemSaindo = false;

  for (int i = 0; i < quantidadeOrdem; i++) {

    int posicaoX =
      -LARGURA_SPRITE - (i * ESPACO_FILA);

    adicionarPersonagem(
      ordemSalva[i],
      posicaoX,
      16
    );
  }
}

// =====================================
// RECEBER ORDEM DO ARDUINO CLOUD
// =====================================

void configurarIniciativa(const String &lista) {

  quantidadeOrdem = 0;

  int inicio = 0;

  while (inicio < lista.length() &&
         quantidadeOrdem < MAX_PERSONAGENS) {

    int fim = lista.indexOf(',', inicio);

    if (fim == -1) {
      fim = lista.length();
    }

    String nome = lista.substring(inicio, fim);

    nome.trim();

    if (nome.length() > 0) {

      PersonagemID id;

      if (identificarPersonagem(nome, id)) {

        ordemSalva[quantidadeOrdem] = id;
        quantidadeOrdem++;

      }
      else {

        Serial.print("Personagem desconhecido: ");
        Serial.println(nome);

      }
    }

    inicio = fim + 1;
  }

  // Monta novamente a fila
  montarFila();

  status = "Iniciativa carregada: " +
           String(quantidadeOrdem) +
           " participantes";

  Serial.println(status);
}

// =====================================
// AVANÇAR INICIATIVA
// =====================================

void avancarFila() {

  if (quantidadeFila == 0 || personagemSaindo) {
    return;
  }

  // Aguarda chegar ao centro
  if (fila[0].x < CENTRO_X) {

    status = "Aguarde o personagem chegar ao centro";

    return;
  }

  personagemSaindo = true;

  status = "Personagem saindo";
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
// LER BOTÃO FÍSICO
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

  uint8_t id =
    static_cast<uint8_t>(fila[indice].id);

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
// ATUALIZAR ANIMAÇÃO
// =====================================

void atualizarSistema() {

  static unsigned long ultimoFrame = 0;

  unsigned long agora = millis();

  if (agora - ultimoFrame < INTERVALO_MOVIMENTO) {
    return;
  }

  ultimoFrame = agora;

  display.clearDisplay();

  // Fundo sempre desenhado primeiro
  desenharFundo();

  // =================================
  // PERSONAGEM SAINDO
  // =================================

  if (personagemSaindo && quantidadeFila > 0) {

    fila[0].x++;

    // Quando sair completamente
    if (fila[0].x >= LARGURA) {

      removerPrimeiro();

      personagemSaindo = false;

      // Reinicia a rodada quando todos saírem
      if (quantidadeFila == 0 &&
          quantidadeOrdem > 0) {

        montarFila();

        status = "Nova rodada iniciada";
      }
    }
  }

  // =================================
  // DESENHAR FILA
  // =================================

  if (personagemSaindo && quantidadeFila > 0) {

    // Personagem atual caminhando para fora
    int frameAtual =
      1 + ((agora / (INTERVALO_MOVIMENTO *
      VELOCIDADE_ANIMACAO)) % 2);

    desenharPersonagem(0, frameAtual);

    // Os demais aguardam de perfil
    for (int i = 1; i < quantidadeFila; i++) {
      desenharPersonagem(i, 1);
    }
  }

  // =================================
  // MOVIMENTO NORMAL
  // =================================

  else {

    for (int i = 0; i < quantidadeFila; i++) {

      int destinoX =
        CENTRO_X - (i * ESPACO_FILA);

      bool andando = fila[i].x < destinoX;

      if (andando) {
        fila[i].x++;
      }

      int frameAtual;

      if (i == 0 && fila[i].x >= CENTRO_X) {

        // Primeiro: olhando para frente
        frameAtual = 0;

      }
      else if (andando) {

        // Caminhando: frames 2 e 3
        frameAtual =
          1 + ((agora / (INTERVALO_MOVIMENTO *
          VELOCIDADE_ANIMACAO) + i) % 2);

      }
      else {

        // Esperando: perfil parado
        frameAtual = 1;

      }

      desenharPersonagem(i, frameAtual);
    }
  }

  display.display();
}

// =====================================
// CALLBACKS DO ARDUINO CLOUD
// =====================================

void onOrdemIniciativaChange() {

  configurarIniciativa(ordemIniciativa);
}

void onFundoSelecionadoChange() {

  if (FundoSelecionado < 0 ||
      FundoSelecionado >= TOTAL_FUNDOS) {

    status = "Fundo invalido";

  }
  else {

    status = "Fundo selecionado: " +
             String(FundoSelecionado);

  }

  Serial.println(status);
}

void onAvancarChange() {

  if (avancar) {

    avancarFila();

    // Retorna o botão virtual ao estado desligado
    avancar = false;
  }
}

void onStatusChange() {

  Serial.print("Status recebido: ");
  Serial.println(status);
}

// Chamado quando todas as propriedades são sincronizadas
void aoSincronizar() {

  Serial.println("Arduino Cloud sincronizado!");

  if (ordemIniciativa.length() > 0) {
    configurarIniciativa(ordemIniciativa);
  }

  status = "Escudo conectado e pronto";
}

// =====================================
// SETUP
// =====================================

void setup() {

  Serial.begin(115200);

  Wire.begin(8, 9);

  pinMode(PINO_BOTAO, INPUT_PULLUP);

  // Inicializa OLED
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

  // Inicializa as propriedades do Cloud
  initProperties();

  // Fila padrão enquanto aguarda a sincronização
  configurarIniciativa(
    "BALAZAR,DIANA,KEYLETH,MILO,FRITZ,INIMIGO"
  );

  // Inicializa conexão com Arduino Cloud
  ArduinoCloud.begin(
    ArduinoIoTPreferredConnection
  );

  // Detecta sincronização inicial
  ArduinoCloud.addCallback(
    ArduinoIoTCloudEvent::SYNC,
    aoSincronizar
  );

  setDebugMessageLevel(2);
  ArduinoCloud.printDebugInfo();

}

// =====================================
// LOOP PRINCIPAL
// =====================================

void loop() {

  ArduinoCloud.update();

  lerBotao();

  atualizarSistema();

}
