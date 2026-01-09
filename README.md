<div align="center">

# 🚦 Smart Traffic Light with ESP32-CAM & TinyML
### Semáforo Inteligente com Visão Computacional

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
![Status](https://img.shields.io/badge/Status-Completed-success)

[🇧🇷 Português](#-versão-em-português) | [🇺🇸 English](#-english-version)

</div>

---

<a name="-versão-em-português"></a>
## 🇧🇷 Versão em Português

Este projeto implementa um sistema de controle de tráfego adaptativo utilizando um **ESP32-CAM**. Diferente de temporizadores fixos ou sensores de barreira física, este sistema utiliza **Visão Computacional** e **TinyML** para detectar veículos em tempo real e ajustar o fluxo do cruzamento dinamicamente.

### 🧠 Sobre o Projeto

O objetivo principal é otimizar o fluxo em cruzamentos urbanos, priorizando uma via principal e liberando a via secundária apenas mediante confirmação visual de presença de veículos.

* **Processamento na Borda (Edge Computing):** Todo o processamento de imagem é feito no próprio ESP32-CAM, garantindo baixa latência e independência de conexão constante com a nuvem.
* **Detecção Não Invasiva:** Elimina a necessidade de obras civis para sensores indutivos no asfalto.
* **Inteligência Artificial:** Utiliza uma Rede Neural (treinada via Edge Impulse) capaz de identificar veículos.

### 🛠️ Hardware e Tecnologias

* **Microcontrolador:** ESP32-CAM (Módulo ESP-32S com câmera OV2640)
* **Plataforma de ML:** Edge Impulse (TinyML)
* **Linguagem:** C++ (Arduino IDE)
* **Componentes:**
    * 1x Módulo FTDI (para programação)
    * 6x LEDs (2 Verdes, 2 Amarelos, 2 Vermelhos)
    * 6x Resistores 220R
    * Protoboard e Jumpers

### 🔌 Pinagem (GPIO)

A conexão dos LEDs segue a tabela abaixo (adaptada para o ESP32-CAM):

| Semáforo | Cor | Pino GPIO (ESP32) |
| :--- | :--- | :--- |
| **Principal (Via 1)** | 🟢 Verde | GPIO 13 |
| **Principal (Via 1)** | 🟡 Amarelo | GPIO 12 |
| **Principal (Via 1)** | 🔴 Vermelho | GPIO 14 |
| **Secundário (Via 2)** | 🟢 Verde | GPIO 2 |
| **Secundário (Via 2)** | 🟡 Amarelo | GPIO 4 |
| **Secundário (Via 2)** | 🔴 Vermelho | GPIO 15 |

> **Nota:** Os pinos GPIO 0 e GPIO 16 são utilizados internamente pela câmera ou para boot.

### ⚙️ Lógica de Controle

O firmware opera com base na inferência da IA em tempo real:

1.  **Estado Padrão:** A Via Principal permanece **ABERTA (Verde)** e a Via Secundária **FECHADA (Vermelho)**.
2.  **Monitoramento:** A câmera monitora constantemente a Via Secundária.
3.  **Gatilho (Trigger):** Se a IA detectar o label `carro` com uma confiança superior a **70%**:
4.  **Ação de Troca:**
    * O Semáforo Principal inicia o fechamento (Verde > Amarelo > Vermelho).
    * O Semáforo Secundário abre (Vermelho > Verde).
5.  **Retorno:** Após o tempo de fluxo programado ou se a detecção cessar, o sistema retorna automaticamente ao Estado Padrão.

### 📂 Documentação

* [📄 Relatório Técnico Completo (PDF)](./docs/RELATORIO_PROJETO.pdf)
* [🖼️ Diagrama Elétrico](./assets/diagrama_circuito.png)

---

<a name="-english-version"></a>
## 🇺🇸 English Version

This project implements an adaptive traffic control system using an **ESP32-CAM**. Unlike fixed timers or physical barrier sensors, this system uses **Computer Vision** and **TinyML** to detect vehicles in real-time and dynamically adjust intersection flow.

### 🧠 About the Project

The main objective is to optimize flow at urban intersections by prioritizing a main road and opening the secondary road only upon visual confirmation of a vehicle's presence.

* **Edge Computing:** All image processing is performed directly on the ESP32-CAM, ensuring low latency and independence from constant cloud connectivity.
* **Non-Invasive Detection:** Eliminates the need for civil works such as inductive loop sensors in the pavement.
* **Artificial Intelligence:** Uses a Convolutional Neural Network (trained via Edge Impulse) capable of distinguishing vehicles from other objects.

### 🛠️ Hardware & Tech Stack

* **Microcontroller:** ESP32-CAM (ESP-32S Module with OV2640 camera)
* **ML Platform:** Edge Impulse (TinyML)
* **Language:** C++ (Arduino IDE)
* **Components:**
    * 1x FTDI Module (for programming)
    * 6x LEDs (2 Green, 2 Yellow, 2 Red)
    * 6x 220R Resistors
    * Breadboard and Jumpers

### 🔌 Pinout (GPIO)

LED connections follow the table below (adapted for ESP32-CAM):

| Traffic Light | Color | GPIO Pin (ESP32) |
| :--- | :--- | :--- |
| **Main (Road 1)** | 🟢 Green | GPIO 13 |
| **Main (Road 1)** | 🟡 Yellow | GPIO 12 |
| **Main (Road 1)** | 🔴 Red | GPIO 14 |
| **Secondary (Road 2)** | 🟢 Green | GPIO 2 |
| **Secondary (Road 2)** | 🟡 Yellow | GPIO 4 |
| **Secondary (Road 2)** | 🔴 Red | GPIO 15 |

> **Note:** GPIO 0 and GPIO 16 are used internally by the camera or for boot/flash modes.

### ⚙️ Control Logic

The firmware operates based on real-time AI inference:

1.  **Default State:** The Main Road remains **OPEN (Green)** and the Secondary Road **CLOSED (Red)**.
2.  **Monitoring:** The camera constantly monitors the Secondary Road.
3.  **Trigger:** If the AI detects the label `car` with a confidence score greater than **70%**:
4.  **Action:**
    * Main Traffic Light starts closing (Green > Yellow > Red).
    * Secondary Traffic Light opens (Red > Green).
5.  **Return:** After the scheduled flow time or if detection ceases, the system automatically returns to the Default State.

### 📂 Documentation

* [📄 Full Technical Report (PDF - PT-BR)](./docs/RELATORIO_PROJETO.pdf)
* [🖼️ Circuit Diagram](./assets/diagrama_circuito.png)

---

## 👥 Authors / Autores

* Samuel Deak Luiz 
* Arthur Feitosa Nogueira