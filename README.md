# 🔐 SmartLocker — Sistema Inteligente de Controle de Acesso

Sistema de controle de acesso para armários desenvolvido com **ESP32**, utilizando autenticação RFID, monitoramento físico da porta e controle remoto por meio de uma interface web.

O projeto integra hardware e software em uma solução ciberfísica, na qual sensores monitoram o ambiente físico, o ESP32 processa as informações e os atuadores executam ações de controle.

Desenvolvido como projeto acadêmico de Engenharia de Software, com foco em sistemas embarcados, comunicação cliente-servidor e Internet das Coisas (IoT).

---

## 📌 Funcionalidades

### 🔑 Controle de acesso
- Autenticação por cartões e tags RFID utilizando o módulo RC522.
- Identificação de usuários e administradores por meio de UIDs cadastrados.
- Controle de acesso com diferentes permissões.
- Identificação e rejeição de tags não autorizadas.

### 🚪 Monitoramento e segurança
- Monitoramento do estado da porta por meio de um sensor magnético Reed Switch.
- Controle de travamento e destravamento utilizando servo motor.
- Detecção de abertura indevida do armário.
- Acionamento de alarme sonoro e visual em situações de violação.
- Destravamento da trava em caso de violação, mantendo o alarme ativo até o silenciamento.
- Retravamento automático após o encerramento da situação de alarme e fechamento da porta.

### 🌐 Interface web
- Servidor HTTP hospedado diretamente no ESP32.
- Acesso por dispositivos conectados à mesma rede Wi-Fi.
- Página de autenticação para usuários e administradores.
- Consulta remota do estado da porta, da trava e do alarme.
- Destravamento remoto do armário.
- Silenciamento remoto do alarme.
- Interface administrativa com informações de conexão e histórico de eventos.

### 📋 Monitoramento e registros
- Registro de eventos do sistema.
- Classificação dos registros por níveis:
  - `INFO` — Informações gerais.
  - `AVISO` — Situações que exigem atenção.
  - `ERRO` — Falhas identificadas.
- Armazenamento persistente dos registros utilizando Preferences (NVS).
- Registro de data e hora por meio de sincronização NTP, com alternativa de referência temporal baseada no tempo de funcionamento do ESP32.
- Exibição do endereço IP, endereço MAC e estado da conexão Wi-Fi.

### 📡 Conectividade
- Comunicação Wi-Fi em modo Station (STA).
- ESP32 operando como servidor HTTP.
- Reconexão automática após perda de conexão Wi-Fi.
- Registro de eventos de desconexão e reconexão.
- Funcionamento local dos sensores e atuadores mesmo durante a indisponibilidade da rede.

---

## 🧰 Tecnologias e componentes

### Hardware

| Componente | Função |
|---|---|
| ESP32 | Unidade principal de processamento e servidor web |
| RC522 | Leitura de cartões e tags RFID |
| Servo motor | Travamento e destravamento do armário |
| Reed Switch | Detecção de abertura e fechamento da porta |
| Buzzer | Alertas sonoros |
| LED verde | Indicação visual de estado |
| LED vermelho | Indicação visual de estado e segurança |
| LED amarelo | Indicação de acesso não autorizado |

### Software e bibliotecas

- **Arduino IDE** — Desenvolvimento e gravação do firmware.
- **C/C++** — Linguagem utilizada no firmware.
- **WiFi.h** — Conectividade Wi-Fi do ESP32.
- **WebServer.h** — Implementação do servidor HTTP.
- **SPI.h** — Comunicação SPI com o leitor RFID.
- **MFRC522.h** — Controle do módulo RC522.
- **ESP32Servo.h** — Controle do servo motor.
- **Preferences.h** — Armazenamento persistente na memória NVS.
- **LittleFS** — Armazenamento dos arquivos da interface web.

---

## 🔌 Ligações dos componentes

A tabela apresenta a distribuição dos pinos utilizada no projeto.

| Componente | Pino ESP32 |
|---|---:|
| RC522 — SDA / SS | GPIO 21 |
| RC522 — RST | GPIO 22 |
| RC522 — SCK | GPIO 18 |
| RC522 — MISO | GPIO 19 |
| RC522 — MOSI | GPIO 23 |
| Servo motor | GPIO 13 |
| Reed Switch | GPIO 27 |
| Buzzer | GPIO 26 |
| LED verde | GPIO 25 |
| LED vermelho | GPIO 33 |
| LED amarelo | GPIO 32 |

**Observações:**
- O Reed Switch utiliza configuração `INPUT_PULLUP`.
- O buzzer utilizado é um módulo de três pinos, com acionamento em nível LOW.
- O servo motor representa o mecanismo físico de travamento do armário.
- Verifique a alimentação adequada dos módulos e do servo motor antes de realizar a montagem.

---

## 🏗️ Arquitetura do sistema

O SmartLocker utiliza uma arquitetura cliente-servidor, na qual o ESP32 atua como servidor e os dispositivos conectados à mesma rede Wi-Fi podem acessar a interface web.

O funcionamento pode ser dividido em três partes:

**1. Camada física**

Responsável pela leitura das tags RFID, detecção da porta e execução das ações físicas por meio do servo motor, LEDs e buzzer.

**2. Processamento e controle**

O ESP32 processa as informações recebidas dos sensores, verifica as permissões de acesso, controla os atuadores e registra os eventos na memória NVS.

**3. Comunicação e interface**

O servidor HTTP disponibiliza as páginas web e permite que usuários autenticados consultem o estado do armário e executem operações remotamente.

### Fluxo simplificado

```text
          DISPOSITIVO CLIENTE
             (Navegador)
                  |
                  | Wi-Fi / HTTP
                  |
             +---------+
             |  ESP32  |
             | Servidor|
             +---------+
                  |
       +----------+----------+
       |          |          |
     RC522    Reed Switch   NVS
       |          |          |
       +----------+----------+
                  |
          Processamento
                  |
          +-------+-------+
          |       |       |
        Servo   LEDs   Buzzer
```

---

## ⚙️ Instalação e configuração

### 1. Requisitos

- Arduino IDE instalada.
- Placa ESP32 configurada na Arduino IDE.
- Módulo ESP32.
- Componentes físicos descritos neste README.
- Bibliotecas necessárias instaladas.
- Dispositivo conectado à mesma rede Wi-Fi do ESP32.

### 2. Configuração do firmware

Abra o código-fonte do projeto na Arduino IDE e configure as credenciais da rede Wi-Fi de acordo com sua instalação.

As credenciais de acesso de usuário e administrador também devem ser configuradas no firmware.

**Importante:** não publique senhas reais, credenciais pessoais ou informações privadas no repositório.

### 3. Configuração do LittleFS

Os arquivos da interface web devem estar disponíveis no sistema de arquivos LittleFS do ESP32.

Arquivos utilizados:

```text
data/
├── login.html
├── user.html
├── admin.html
└── style.css
```

Faça o upload dos arquivos para o LittleFS utilizando o procedimento ou ferramenta compatível com a versão do ambiente Arduino configurado.

### 4. Gravação do firmware

1. Conecte o ESP32 ao computador.
2. Selecione a placa e a porta serial corretas.
3. Verifique se as bibliotecas necessárias estão instaladas.
4. Compile e grave o firmware.
5. Abra o Monitor Serial para acompanhar a inicialização e os eventos do sistema.

### 5. Acesso à interface

Após conectar-se à rede Wi-Fi, consulte o endereço IP apresentado pelo ESP32 no Monitor Serial ou na interface administrativa.

No navegador, acesse:

```text
http://IP_DO_ESP32
```

O sistema disponibiliza a página de login para autenticação.

---

## 🔒 Segurança

O sistema possui autenticação para acesso às páginas protegidas e controle de sessões para diferenciar usuários e administradores.

Algumas considerações importantes:

- Utilize credenciais próprias e senhas fortes.
- Não compartilhe credenciais de acesso no repositório.
- O servidor utiliza HTTP, sem criptografia TLS.
- O projeto foi desenvolvido para utilização em uma rede local controlada.
- Não é recomendado expor diretamente o servidor à internet sem implementar mecanismos adicionais de segurança.

---

## 🎯 Objetivos acadêmicos

O desenvolvimento do SmartLocker permite aplicar conceitos relacionados a:

- Sistemas ciberfísicos.
- Sistemas embarcados.
- Internet das Coisas (IoT).
- Comunicação cliente-servidor.
- Redes de computadores.
- Protocolos de comunicação TCP/IP.
- Sensores e atuadores.
- Armazenamento persistente de dados.
- Desenvolvimento de interfaces web.
- Segurança e controle de acesso.

---

## 🚀 Possíveis melhorias futuras

- Implementação de comunicação HTTPS.
- Atualização dos estados da interface sem recarregar a página.
- Gerenciamento de usuários e tags pela interface administrativa.
- Melhorias no sistema de notificações.
- Expansão do histórico de eventos.
- Monitoramento remoto por uma aplicação externa.
