# 🌟 Projeto: Simulação de controle de acesso com Raspberry Pi Pico W

## 📖 Descrição

Este projeto utiliza um **Raspberry Pi Pico W** para controlar uma matriz de LEDs **WS2812B**, um display **OLED SSD1306** e um **buzzer**, permitindo interações por meio de botões e um joystick. O sistema é capaz de exibir valores e status no display, tocar sons e controlar a intensidade dos LEDs com base na entrada do joystick.

## ⚙️ Funcionalidades

- 🎨 Controle de LEDs RGB com intensidade variável
- 🖥️ Exibição de valores de eixo X e Y do joystick em um display OLED
- 🔐 Entrada de senha através do joystick
- 🎶 Feedback sonoro com buzzer
- ⏱️ Interrupções para tratamento de botões

## 📂 Estrutura do Código

O código é organizado em funções que gerenciam diferentes aspectos do projeto:

- **Inicialização de pinos**: Configuração dos pinos GPIO para LEDs, botões, buzzer e joystick.
- **Interrupção de GPIO**: Gerenciamento de interrupções para botões e joystick.
- **Controle de PWM**: Controle da intensidade dos LEDs e do buzzer.
- **Desenho em matriz**: Envio de comandos para a matriz de LEDs.
- **Atualização do display**: Exibição de informações no display OLED.

## 🛠️ Componentes Necessários

- Raspberry Pi Pico
- Matriz de LEDs WS2812B
- Display OLED SSD1306
- Joystick
- Buzzer
- Resistores e fios de conexão

## 📥 Instalação

1. Clone o repositório:
   ```bash
   git clone https://github.com/Santos-Luan/Projeto-final.git
   cd Projeto-final
   ```
