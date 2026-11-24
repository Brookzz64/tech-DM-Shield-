#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#include "arduino_secrets.h"
#include "thingProperties.h"
#include "sprites.h"

// ---------------- CONFIGURAÇÕES ----------------

#define LARGURA_TELA 128
#define ALTURA_TELA 64

#define PINO_BOTAO 3

#define MAX_PERSONAGENS 6

#define CENTRO_X 48
#define ESPACO_FILA 40

#define VELOCIDADE 2
#define INTERVALO_MOVIMENTO 120

#define TEMPO_DEBOUNCE 40

Adafruit_SSD1306 display(
  LARGURA_TELA,
  ALTURA_TELA,
  &Wire,
  -1
);

// ---------------- ESTRUTURA DA FILA ----------------

struct Animacao {
  PersonagemID id;
  int x;
  int y;
};

Animacao fila[MAX_PERSONAGENS];

PersonagemID ordemSalva[MAX_PERSONAGENS];

uint8_t quantidadeFila = 0;
uint8_t quantidadeOrdem = 0;

// ---------------- ESTADOS DO SISTEMA ----------------

bool personagemSaindo = false;
bool modoMenu = true;

bool cloudSincronizado = false;

String ultimaOrdemProcessada = "";

uint8_t frameAndando = 1;

unsigned long ultimoFrameMovimento = 0;

// ---------------- BOTÃO ----------------

bool estadoBotaoEstavel = HIGH;
bool ultimaLeituraBotao = HIGH;

unsigned long ultimoDebounce = 0;

// ====================================================
// INICIALIZAÇÃO
// ====================================================

void setup() {

  Serial.begin(115200);

  pinMode(PINO_BOTAO, INPUT_PULLUP);

  Wire.begin(8, 9);

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("Erro ao iniciar OLED!");
    while (true) {}
  }

  display.clearDisplay();
  display.display();

  // Mostra o logo ao ligar o dispositivo
  modoMenu = true;
  mostrarLogo();

  // Inicializa Arduino Cloud
  initProperties();

  ArduinoCloud.addCallback(
    ArduinoIoTCloudEvent::SYNC,
    aoSincronizar
  );

  ArduinoCloud.begin(
    ArduinoIoTPreferredConnection
  );

  setDebugMessageLevel(2);
  ArduinoCloud.printDebugInfo();

  Serial.println("Sistema iniciado!");
}

// ====================================================
// LOOP PRINCIPAL
// ====================================================

void loop() {

  ArduinoCloud.update();

  lerBotao();

  atualizarSistema();
}

// ====================================================
// LOGO E TELA
// ====================================================

void mostrarLogo() {

  display.clearDisplay();

  display.drawBitmap(
    0,
    0,
    logoIdle,
    128,
    64,
    SSD1306_WHITE
  );

  display.display();
}

// ====================================================
// DESENHAR CENÁRIO
// ====================================================

void desenharFundo() {

  int fundo = FundoSelecionado;

  if (fundo < 0 || fundo >= TOTAL_FUNDOS) {
    fundo = 0;
  }

  display.drawBitmap(
    0,
    0,
    fundosSuperiores[fundo],
    128,
    16,
    SSD1306_WHITE
  );

  display.drawBitmap(
    0,
    48,
    fundosInferiores[fundo],
    128,
    16,
    SSD1306_WHITE
  );
}

// ====================================================
// DESENHAR PERSONAGENS
// ====================================================

void desenharTela() {

  if (modoMenu) {
    mostrarLogo();
    return;
  }

  display.clearDisplay();

  desenharFundo();

  for (uint8_t i = 0; i < quantidadeFila; i++) {

    uint8_t frame;

    int destino = CENTRO_X - (i * ESPACO_FILA);

    // Personagem da frente
    if (i == 0) {

      if (personagemSaindo) {
        frame = frameAndando;
      }
      else if (fila[i].x >= CENTRO_X) {
        frame = 0;
      }
      else {
        frame = frameAndando;
      }
    }

    // Demais personagens
    else {

      if (fila[i].x >= destino) {
        frame = 1;
      }
      else {
        frame = frameAndando;
      }
    }

    display.drawBitmap(
      fila[i].x,
      fila[i].y,
      sprites[fila[i].id][frame],
      LARGURA_SPRITE,
      ALTURA_SPRITE,
      SSD1306_WHITE
    );
  }

  display.display();
}

// ====================================================
// MONTAR FILA
// ====================================================

void montarFila() {

  quantidadeFila = quantidadeOrdem;

  personagemSaindo = false;
  modoMenu = false;

  frameAndando = 1;

  for (uint8_t i = 0; i < quantidadeOrdem; i++) {

    fila[i].id = ordemSalva[i];

    fila[i].x = -32 - (i * ESPACO_FILA);

    fila[i].y = 16;
  }

  ultimoFrameMovimento = millis();
}

// ====================================================
// IDENTIFICAR PERSONAGEM PELO NOME
// ====================================================

bool identificarPersonagem(
  String nome,
  PersonagemID &id
) {

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

// ====================================================
// RECEBER COMANDO DE INICIATIVA
// ====================================================

void configurarIniciativa(String lista) {

  String comando = lista;

  comando.trim();
  comando.toUpperCase();

  // Comando MENU
  if (comando == "MENU" || comando.length() == 0) {

    quantidadeFila = 0;
    quantidadeOrdem = 0;

    personagemSaindo = false;
    modoMenu = true;

    status = "Menu";

    mostrarLogo();

    Serial.println("Voltando ao menu");

    return;
  }

  // Monta uma nova ordem temporária
  PersonagemID novaOrdem[MAX_PERSONAGENS];

  uint8_t novaQuantidade = 0;

  int inicio = 0;

  while (inicio < comando.length()) {

    int separador = comando.indexOf(',', inicio);

    String nome;

    if (separador == -1) {

      nome = comando.substring(inicio);

      inicio = comando.length();
    }
    else {

      nome = comando.substring(inicio, separador);

      inicio = separador + 1;
    }

    nome.trim();

    if (nome.length() == 0) {
      continue;
    }

    if (novaQuantidade >= MAX_PERSONAGENS) {
      break;
    }

    PersonagemID personagem;

    if (identificarPersonagem(nome, personagem)) {

      novaOrdem[novaQuantidade] = personagem;

      novaQuantidade++;
    }
    else {

      status = "Personagem desconhecido: " + nome;

      Serial.print("Personagem desconhecido: ");
      Serial.println(nome);

      return;
    }
  }

  if (novaQuantidade == 0) {

    status = "Nenhum personagem encontrado";

    return;
  }

  // Salva a ordem original
  quantidadeOrdem = novaQuantidade;

  for (uint8_t i = 0; i < quantidadeOrdem; i++) {

    ordemSalva[i] = novaOrdem[i];
  }

  montarFila();

  status = "Iniciativa carregada: " + String(quantidadeOrdem);

  Serial.println("Nova iniciativa carregada!");
}

// ====================================================
// AVANÇAR INICIATIVA
// ====================================================

void avancarFila() {

  if (modoMenu) return;

  if (quantidadeFila == 0) return;

  if (personagemSaindo) return;

  // Impede avançar antes do personagem chegar ao centro
  if (fila[0].x < CENTRO_X) {

    status = "Aguarde o personagem chegar";

    return;
  }

  personagemSaindo = true;

  status = "Personagem saindo";

  Serial.println("Avancando iniciativa");
}

// ====================================================
// REMOVER PRIMEIRO PERSONAGEM
// ====================================================

void removerPrimeiro() {

  if (quantidadeFila == 0) return;

  for (uint8_t i = 1; i < quantidadeFila; i++) {

    fila[i - 1] = fila[i];
  }

  quantidadeFila--;

  personagemSaindo = false;
}

// ====================================================
// ATUALIZAR MOVIMENTAÇÃO
// ====================================================

void atualizarSistema() {

  if (modoMenu) return;

  if (quantidadeFila == 0) return;

  unsigned long agora = millis();

  if (agora - ultimoFrameMovimento < INTERVALO_MOVIMENTO) {
    return;
  }

  ultimoFrameMovimento = agora;

  // Alterna os frames de caminhada
  if (frameAndando == 1) {
    frameAndando = 2;
  }
  else {
    frameAndando = 1;
  }

  // Se o personagem da frente está saindo
  if (personagemSaindo) {

    fila[0].x += VELOCIDADE;

    // Quando sair completamente da tela
    if (fila[0].x >= LARGURA_TELA) {

      removerPrimeiro();

      // Último personagem saiu:
      // começa uma nova rodada com a mesma ordem
      if (quantidadeFila == 0 && quantidadeOrdem > 0) {

        montarFila();

        status = "Nova rodada iniciada";

        Serial.println("Nova rodada iniciada");
      }
    }
  }

  // Se ninguém está saindo, movimenta a fila
  else {

    for (uint8_t i = 0; i < quantidadeFila; i++) {

      int destino = CENTRO_X - (i * ESPACO_FILA);

      if (fila[i].x < destino) {

        fila[i].x += VELOCIDADE;

        // Não deixa ultrapassar o destino
        if (fila[i].x > destino) {
          fila[i].x = destino;
        }
      }
    }
  }

  desenharTela();
}

// ====================================================
// LEITURA DO BOTÃO FÍSICO
// ====================================================

void lerBotao() {

  bool leitura = digitalRead(PINO_BOTAO);

  if (leitura != ultimaLeituraBotao) {

    ultimoDebounce = millis();
  }

  if ((millis() - ultimoDebounce) > TEMPO_DEBOUNCE) {

    if (leitura != estadoBotaoEstavel) {

      estadoBotaoEstavel = leitura;

      // Botão pressionado
      if (estadoBotaoEstavel == LOW) {

        avancarFila();
      }
    }
  }

  ultimaLeituraBotao = leitura;
}

// ====================================================
// SINCRONIZAÇÃO COM ARDUINO CLOUD
// ====================================================

void aoSincronizar() {

  cloudSincronizado = true;

  // Registra o valor que já estava no Cloud.
  // Assim, ele não será executado automaticamente ao ligar.
  ultimaOrdemProcessada = ordemIniciativa;

  ultimaOrdemProcessada.trim();
  ultimaOrdemProcessada.toUpperCase();

  quantidadeFila = 0;
  quantidadeOrdem = 0;

  personagemSaindo = false;
  modoMenu = true;

  avancar = false;

  status = "Menu";

  mostrarLogo();

  Serial.println("Arduino Cloud sincronizado");
}

// ====================================================
// CALLBACKS DO ARDUINO CLOUD
// ====================================================

void onOrdemIniciativaChange() {

  if (!cloudSincronizado) return;

  String comando = ordemIniciativa;

  comando.trim();
  comando.toUpperCase();

  // Ignora o valor antigo recebido na sincronização
  if (comando == ultimaOrdemProcessada) {
    return;
  }

  ultimaOrdemProcessada = comando;

  configurarIniciativa(comando);
}


void onAvancarChange() {

  if (!cloudSincronizado) return;

  if (avancar) {

    avancarFila();

    // Reseta o botão do Dashboard
    avancar = false;
  }
}


void onFundoSelecionadoChange() {

  if (FundoSelecionado < 0 || FundoSelecionado >= TOTAL_FUNDOS) {

    status = "Cenario invalido";

    return;
  }

  status = "Cenario alterado";

  desenharTela();
}


void onStatusChange() {

  // Não precisamos executar nenhuma ação aqui.
}
