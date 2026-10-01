# SmartLocker
## 🔐 Armário Inteligente com ESP32

Projeto de um armário inteligente baseado em ESP32, desenvolvido para controlar o acesso por RFID, monitorar o estado da porta e controlar eletronicamente a trava por meio de um servo motor.
O sistema possui também uma interface web hospedada diretamente pelo ESP32, permitindo acompanhar o estado do armário pelo navegador de um celular ou computador conectado à mesma rede Wi-Fi.
O projeto está sendo desenvolvido de forma modular, permitindo adicionar novas funcionalidades posteriormente sem precisar desmontar a estrutura eletrônica já montada.

## 🚀 Funcionalidades atuais

- 📡 Leitura de cartões e tags RFID utilizando o RC522
- 🔐 Controle da trava através de um servo motor MG90S
- 🚪 Detecção de porta aberta/fechada utilizando um reed switch
- 🔊 Avisos sonoros através de um buzzer ativo
- 💡 Indicação de estados através de LEDs
- 🌐 Servidor web executado diretamente no ESP32
- 📱 Interface acessível por celular ou computador
- 🔄 Atualização do estado do armário na interface web
- 🔓 Destravamento através de RFID
- 🔒 Retravamento automático após o fechamento da porta
- 📋 Exibição do último UID RFID detectado
- 📝 Registro do último evento ocorrido no sistema

## 🧰 Componentes
- ESP32 DevKit V1
- RFID RC522
- Servo MG90S
- Reed Switch
- Módulo Buzzer Ativo
- LEDs Variados
- Resistores Variados
- Protoboard 830 pontos
- Jumpers	Diversos

## 🔌 Pinagem
### RFID RC522
- SDA / SS	GPIO 21
- SCK	GPIO 18
- MOSI	GPIO 23
- MISO	GPIO 19
- IRQ	Não conectado
- GND	GND
- RST	GPIO 22
- 3.3V	3V3

### Outros componentes
- Servo MG90S	GPIO 13
- Reed Switch	GPIO 27
- Buzzer — I/O	GPIO 26
- LED verde	GPIO 25
- LED vermelho	GPIO 33
- LED auxiliar	GPIO 32

O RC522 é alimentado com 3,3 V.

## 🌐 Interface Web

O ESP32 funciona como um pequeno servidor web.
Após conectar-se à rede Wi-Fi, ele informa seu endereço IP. Esse endereço pode ser acessado pelo navegador:
**http://IP_DO_ESP32**

A interface apresenta informações como:
- Estado da porta
- Estado da trava
- Status do RFID
- Último UID detectado
- Último evento do sistema
- Status do ESP32 e do servidor web

## 🔄 Funcionamento

O fluxo atual do sistema é:

        ┌──────────────┐
        │  Cartão RFID │
        └──────┬───────┘
               │
               ▼
        ┌──────────────┐
        │    RC522     │
        └──────┬───────┘
               │
               ▼
        ┌──────────────┐
        │     ESP32    │
        └──────┬───────┘
               │
        ┌──────┴───────┐
        │              │
        ▼              ▼
     Servo           Buzzer
        │
        ▼
      Trava
        │
        ▼
      Porta
        │
        ▼
    Reed Switch
        └──────► ESP32


Quando uma tag RFID é detectada, o ESP32 aciona o servo e destrava o armário.

Depois que a porta é aberta e posteriormente fechada, o reed switch informa o novo estado ao ESP32. Após um pequeno intervalo, o servo retorna à posição de travamento.

## 🔊 Buzzer

O buzzer não permanece ligado durante o funcionamento normal.

Ele é acionado somente em eventos específicos.

Comportamento planejado:

#### Evento ->	Sinal
- Inicialização ->	Silencioso
- Wi-Fi conectado ->	Silencioso
- Porta aberta ->	Silencioso
- Porta fechada ->	Silencioso
- Acesso autorizado ->	1 bip
- Armário trancado ->	2 bips
- Acesso negado ->	3 bips

## 💻 Tecnologias

- ESP32
- Arduino IDE
- C++
- Wi-Fi
- HTTP / Web Server
- SPI
- RFID / MFRC522
- Servo motor

## 📚 Bibliotecas

O projeto utiliza:

MFRC522

ESP32Servo

As bibliotecas podem ser instaladas pelo Library Manager da Arduino IDE.

## 🛠️ Instalação
1. Preparar a Arduino IDE
Instale o suporte para placas ESP32 através do Board Manager da Arduino IDE.
Selecione:

NodeMCU-32S

2. Instalar as bibliotecas

No Library Manager, instale:

- MFRC522
- ESP32Servo

3. Configurar o Wi-Fi

No código, informe as credenciais da sua rede:

const char* WIFI_SSID = "NOME_DA_SUA_REDE";

const char* WIFI_PASSWORD = "SENHA_DA_SUA_REDE";

4. Fazer o upload

Conecte o ESP32 ao computador, selecione a porta correspondente e faça o upload do programa.

Após a inicialização, o ESP32 exibirá no Monitor Serial o endereço IP obtido:

WiFi conectado!

IP: 192.168.1.XX

Servidor Web iniciado!

Acesse esse endereço pelo navegador para abrir o painel.

## ⚠️ Alimentação

O RC522 deve ser alimentado com 3,3 V.

O servo MG90S pode exigir uma corrente considerável, especialmente durante movimentos ou quando submetido a carga. Em uma montagem definitiva, recomenda-se utilizar uma fonte adequada para o servo e manter o GND da fonte do servo conectado ao GND do ESP32.

A alimentação utilizada na protoboard durante os testes não necessariamente será a melhor solução para a versão final do armário.

🔮 Próximos passos

O projeto ainda está em desenvolvimento. Algumas funcionalidades planejadas são:

- Cadastro de cartões RFID autorizados
- Identificação do usuário através do UID
- Sistema de acesso autorizado/negado
- Histórico de acessos
- Data e hora dos eventos
- Interface web mais completa
- Controle manual da trava pelo painel
- Configuração dos ângulos do servo
- Detecção de tentativa de acesso não autorizado
- Armazenamento permanente dos usuários
- Banco de dados
- Acesso remoto pela internet
- Sistema de autenticação para o painel
- Melhorias na segurança física e eletrônica

## 🎯 Objetivo do projeto

O objetivo é desenvolver um armário inteligente de baixo custo, utilizando componentes acessíveis e um ESP32 como unidade central de controle.

Além de servir como um sistema funcional de controle de acesso, o projeto também tem como objetivo explorar conceitos de:

- Sistemas embarcados
- Internet das Coisas (IoT)
- RFID
- Automação
- Comunicação SPI
- Servidores web embarcados
- Controle de motores
- Sensores digitais
- Desenvolvimento de interfaces web

## 📌 Status

🟢 Em desenvolvimento

A versão atual já possui:

RFID + ESP32 + servo + reed switch + buzzer + LEDs + interface web funcionando.

Novas funcionalidades serão adicionadas progressivamente ao projeto.
