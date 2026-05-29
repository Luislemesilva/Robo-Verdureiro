# 🌱 Robô Verdureiro

Sistema autônomo de irrigação inteligente com ESP32 e IA hospedada no Hugging Face.

O robô avalia dados climáticos consultando um modelo de Machine Learning e decide, uma vez por dia, se deve ou não irrigar um conjunto de plantas em vasos. O processo mecânico é realizado por uma guilhotina (bico de irrigação vertical), uma esteira de passo e uma bomba d'água controlada por relé.

---

## Sumário

- [Visão Geral](#visão-geral)
- [Arquitetura do Sistema](#arquitetura-do-sistema)
- [Hardware](#hardware)
  - [Pinagem do ESP32](#pinagem-do-esp32)
  - [Diagrama de Conexões](#diagrama-de-conexões)
- [Servidor de IA (Hugging Face)](#servidor-de-ia-hugging-face)
  - [Estrutura de Arquivos](#estrutura-de-arquivos)
  - [Treinando o Modelo](#treinando-o-modelo)
  - [Deploy no Hugging Face](#deploy-no-hugging-face)
  - [Testando a API Manualmente](#testando-a-api-manualmente)
- [Firmware ESP32](#firmware-esp32)
  - [Dependências](#dependências)
  - [Configurações](#configurações)
  - [Modo Teste](#modo-teste)
  - [Fluxo de Operação](#fluxo-de-operação)
  - [Upload](#upload)
- [Histórico de Commits](#histórico-de-commits)
- [Estrutura do Repositório](#estrutura-do-repositório)
- [Licença](#licença)

---

## Visão Geral

```
┌─────────────────────────────────────────────────────┐
│                    ROBÔ VERDUREIRO                   │
│                                                      │
│  ESP32  ──WiFi──►  Hugging Face Space               │
│    │                  (FastAPI + RandomForest)       │
│    │                        │                       │
│    │◄───── "SIM" / "NAO" ───┘                       │
│    │                                                 │
│    ├──► Guilhotina (Motor de Passo)                  │
│    ├──► Esteira    (Motor de Passo)                  │
│    └──► Bomba      (Relé)                            │
└─────────────────────────────────────────────────────┘
```

Uma vez por dia o ESP32:

1. Coleta dados climáticos (precipitação, temperatura, ponto de orvalho, umidade, vento).
2. Envia esses dados via HTTPS para a API no Hugging Face.
3. Recebe a decisão: `"SIM"` (irrigar) ou `"NAO"` (não irrigar).
4. Se `"SIM"`, executa o ciclo mecânico para cada planta em sequência.

---

## Arquitetura do Sistema

```
robo_verdureiro/
├── esp32/
│   └── robo_verdureiro.ino       ← Firmware Arduino/ESP32
└── huggingface/
    ├── app.py                    ← Servidor FastAPI (IA)
    ├── treinar_modelo.py         ← Script de treino local
    ├── modelo.pkl                ← Modelo treinado (gerado localmente)
    ├── requirements.txt          ← Dependências Python
    ├── Dockerfile                ← Container para o Space
    └── README_SPACE.md           ← Metadados do Hugging Face Space
```

---

## Hardware

### Componentes

| Componente               | Modelo / Especificação               |
|--------------------------|--------------------------------------|
| Microcontrolador         | ESP32 DevKit (qualquer variante)     |
| Motor guilhotina         | Motor de passo (NEMA 17 ou similar)  |
| Driver guilhotina        | A4988 / DRV8825                      |
| Motor esteira            | Motor de passo (NEMA 17 ou similar)  |
| Driver esteira           | A4988 / DRV8825                      |
| Bomba d'água             | Bomba submersível 3–6V ou 12V        |
| Módulo relé              | Relé 1 canal, **active LOW**         |
| Fonte de alimentação     | 12V para motores + 3.3V/5V para ESP32|

---

## Servidor de IA (Hugging Face)

O servidor é uma API REST feita com **FastAPI** hospedada como um **Docker Space** no Hugging Face. Ele expõe o endpoint `POST /prever` que recebe dados climáticos e retorna a decisão de irrigação.

### Estrutura de Arquivos

```
huggingface/
├── app.py              ← API principal
├── treinar_modelo.py   ← Treina e gera modelo.pkl
├── modelo.pkl          ← Modelo serializado (gerado localmente)
├── requirements.txt    ← Dependências
└── Dockerfile          ← Container
```

### Treinando o Modelo

Execute **localmente** antes de fazer o deploy:

```bash
# 1. Instale as dependências
pip install scikit-learn numpy joblib

# 2. Treine o modelo
python treinar_modelo.py

# O arquivo modelo.pkl será gerado no mesmo diretório
```

O `treinar_modelo.py` inclui dados de exemplo. **Substitua pelos dados reais** da sua região e cultura para obter melhores resultados.

**Variáveis de entrada do modelo:**

| Campo          | Descrição                        | Unidade |
|----------------|----------------------------------|---------|
| `precipitacao` | Chuva acumulada nas últimas 24h  | mm      |
| `temperatura`  | Temperatura do ar                | °C      |
| `orvalho`      | Ponto de orvalho                 | °C      |
| `umidade`      | Umidade relativa do ar           | %       |
| `vento`        | Velocidade do vento              | km/h    |

**Saída:**
```json
{ "irrigar": "SIM" }
// ou
{ "irrigar": "NAO" }
```

### Deploy no Hugging Face

1. Crie uma conta em [huggingface.co](https://huggingface.co).
2. Crie um novo **Space**:
   - SDK: `Docker`
   - Visibilidade: `Public` (ou `Private`)
   - Nome sugerido: `robo-verdureiro`
3. Clone o repositório do Space:
   ```bash
   git clone https://huggingface.co/spaces/SEU_USUARIO/robo-verdureiro
   cd robo-verdureiro
   ```
4. Copie os arquivos da pasta `huggingface/` para dentro:
   ```bash
   cp ../huggingface/* .
   ```
5. Treine o modelo e adicione o `modelo.pkl`:
   ```bash
   python treinar_modelo.py
   # modelo.pkl gerado aqui
   ```
6. Faça o commit e push:
   ```bash
   git add .
   git commit -m "feat: deploy inicial do servidor de irrigação"
   git push
   ```
7. O Hugging Face fará o build automático do Docker. Aguarde alguns minutos.
8. A URL pública será: `https://SEU_USUARIO-robo-verdureiro.hf.space`

### Testando a API Manualmente

Acesse `https://SEU_USUARIO-robo-verdureiro.hf.space/docs` para a interface Swagger interativa, ou use `curl`:

```bash
curl -X POST https://SEU_USUARIO-robo-verdureiro.hf.space/prever \
  -H "Content-Type: application/json" \
  -d '{
    "precipitacao": 0.0,
    "temperatura": 31.5,
    "orvalho": 20.0,
    "umidade": 48.0,
    "vento": 12.0
  }'
```

Resposta esperada:
```json
{ "irrigar": "SIM" }
```

---

## Firmware ESP32

### Dependências

Instale as bibliotecas abaixo pela **Arduino IDE** (`Ferramentas → Gerenciar Bibliotecas`):

| Biblioteca      | Versão mínima | Fonte             |
|-----------------|---------------|-------------------|
| `WiFi`          | Incluída      | ESP32 Board       |
| `HTTPClient`    | Incluída      | ESP32 Board       |
| `ArduinoJson`   | 6.x           | Benoit Blanchon   |

Configure o **Board** como `ESP32 Dev Module` (ou equivalente ao seu hardware).

### Configurações

No arquivo `robo_verdureiro.ino`, edite as constantes no início:

```cpp
// WiFi
const char* SSID     = "NOME_DA_SUA_REDE";
const char* PASSWORD = "SENHA_DA_REDE";

// URL da API (troque pelo seu usuário do HF)
const char* SERVIDOR_URL = "https://SEU_USUARIO-robo-verdureiro.hf.space/prever";

// Pinos (altere conforme sua fiação)
const int PINO_RELE       = 23;
const int DIR_GUILHOTINA  = 18;
const int STEP_GUILHOTINA = 26;
const int DIR_ESTEIRA     = 13;
const int STEP_ESTEIRA    = 27;

// Parâmetros mecânicos
int passosGuilhotina = 300;  // passos para descer/subir a guilhotina
int passosEsteira    = 180;  // passos para avançar um vaso na esteira

int velocidadeDescida = 3000;  // µs entre pulsos (menor = mais rápido)
int velocidadeSubida  = 3000;
int velocidadeEsteira = 3000;

int quantidadePlantas = 4;  // número de vasos na esteira
```

### Modo Teste

Durante o desenvolvimento, ative o **modo teste** para forçar a irrigação sem consultar a API:

```cpp
bool MODO_TESTE = true;   // ← sempre irriga
bool MODO_TESTE = false;  // ← consulta a IA normalmente
```

Lembre de definir como `false` antes do deploy em produção.

### Fluxo de Operação

```
setup()
  ├── Inicializa pinos (relé, motores)
  ├── Conecta ao WiFi
  └── Executa decidirIrrigacao()

loop()
  └── Aguarda 24h → decidirIrrigacao()

decidirIrrigacao()
  ├── buscarClima()           ← obtém dados climáticos
  ├── consultarModelo()       ← POST /prever → "SIM" ou "NAO"
  └── Se "SIM": ciclo por cada planta
        ├── Guilhotina desce
        ├── Bomba liga (500 ms)
        ├── Bomba desliga
        ├── Guilhotina sobe
        └── Esteira avança um passo
      Ao final: esteira retorna à posição inicial
```

### Upload

1. Abra `robo_verdureiro.ino` na Arduino IDE.
2. Configure `SSID`, `PASSWORD` e `SERVIDOR_URL`.
3. Defina `MODO_TESTE = true` para testes iniciais.
4. Selecione a porta COM correta.
5. Clique em **Upload**.
6. Abra o **Monitor Serial** (115200 baud) para acompanhar os logs.
7. Quando tudo estiver funcionando, defina `MODO_TESTE = false` e faça o upload final.
