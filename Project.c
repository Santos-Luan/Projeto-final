#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "pico/stdlib.h"
#include "lib/ssd1306.h"
#include "hardware/pio.h"
#include "hardware/gpio.h"
#include "hardware/clocks.h"
#include "hardware/pwm.h"
#include "hardware/adc.h"
#include "pico/time.h"
#include "pico/bootrom.h"

// arquivo .pio
#include "build/Project.pio.h"

#include "lib/matrix_numbers.h"

// rotina para acionar a matrix de leds - ws2812b
void desenho_pio(double *desenho, uint32_t valor_led, PIO pio, uint sm, double r, double g, double b);

// Definição dos pinos dos LEDs e dos botões
#define LED_VERMELHO 13
#define LED_VERDE 11
#define LED_AZUL 12
#define BTN_PIN_A 5
#define BTN_PIN_B 6

// número de LEDs
#define NUM_PIXELS 25
#define DEBOUNCE_DELAY 350 // Delay de debounce em milissegundos

// pino de saída matriz
#define OUT_PIN 7

// pino buzzer
#define buzz 10

// Display
#define PORTA_I2C i2c1
#define I2C_SDA 14
#define I2C_SCL 15
#define endereco 0x3C

// JoyStick
#define EIXO_X 26         // GPIO para eixo X
#define EIXO_Y 27         // GPIO para eixo Y
#define BOTAO_JOYSTICK 22 // GPIO para botão do joystick
uint16_t valor_x;
uint16_t valor_y;

// display
ssd1306_t display;
const uint16_t PERIODO_PWM = 4095;
const float DIVISOR_PWM = 16.0;
bool leds_ativos = true;
bool estado_led_verde = false;
bool cor = true;

// Matriz
PIO pio = pio0;
uint16_t i;
uint32_t valor_led;
double r = 0.0, b = 0.0, g = 0.0;
uint sm;

// Variável para debounce da interrupção
static volatile uint64_t last_interrupt_time = 0;
static volatile bool interrupt_flag = false; // Sinalizador da interrupção
static volatile int16_t a = 0;
static volatile bool btnA = false, btnB = false;
static volatile uint64_t last_interrupt_time_A = 0;
static volatile uint64_t last_interrupt_time_B = 0;
static volatile uint64_t last_interrupt_time_JoyStick = 0;

// Senha no display
static volatile int16_t senha[4];
static volatile int16_t senhaCerta[4] = {1, 2, 3, 4};
static volatile int16_t contSenha = 0;

// Função para inicialização dos pinos
void inicializar_pinos();

// Função para definir cores dos LEDs
void set_leds(bool red, bool green, bool blue);

// Função de interrupção
static void gpio_irq_handler(uint gpio, uint32_t events);

// Função para definir a intensidade das cores
uint32_t matrix_rgb(double b, double r, double g);

// Função configurar display
void ConfigurarDisplay();

void configurar_pwm(uint gpio);                 // Configuração do PWM
void atualizar_display(uint16_t x, uint16_t y); // Atualiza a exibição no display
uint16_t calcular_pwm(uint16_t valor);          // Calcula o nível PWM com base na posição do joystick

// Funções do Buzzer
void beep(uint freq, uint duration);
int64_t beep_stop_callback(alarm_id_t id, void *user_data);

// função principal
int main()
{
    // Inicializa todos os códigos stdio padrão que estão ligados ao binário.
    stdio_init_all();
    inicializar_pinos();
    ConfigurarDisplay();

    desenho_pio(desenho0, valor_led, pio, sm, r, g, b);

    while (true)
    {
        adc_select_input(0);
        valor_x = adc_read();
        adc_select_input(1);
        valor_y = adc_read();

        if (!gpio_get(BTN_PIN_A) && a < 10)
        {
            btnA = 1;
            // Configuração da interrupção para o botão A (borda de subida e descida)
            gpio_set_irq_enabled_with_callback(BTN_PIN_A, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);
        }
        else if (!gpio_get(BTN_PIN_B) && a > -1)
        {
            btnB = 1;
            // Configuração da interrupção para o botão B (borda de subida e descida)
            gpio_set_irq_enabled_with_callback(BTN_PIN_B, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);
        }
        else if (!gpio_get(BOTAO_JOYSTICK))
        {
            gpio_set_irq_enabled_with_callback(BOTAO_JOYSTICK, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);
        }

        if (valor_x >= 2000 && valor_x <= 2140 && valor_y >= 1870 && valor_y <= 2010)
        {
            pwm_set_gpio_level(LED_VERMELHO, 0);
            pwm_set_gpio_level(LED_AZUL, 0);
        }
        else
        {
            pwm_set_gpio_level(LED_VERMELHO, calcular_pwm(valor_x));
            pwm_set_gpio_level(LED_AZUL, calcular_pwm(valor_y));
        }
        atualizar_display(valor_x, valor_y);
    }
}

void inicializar_pinos()
{
    stdio_init_all();

    // Inicialização dos LEDs como saída
    gpio_init(LED_VERMELHO);
    gpio_set_dir(LED_VERMELHO, GPIO_OUT);
    gpio_init(LED_VERDE);
    gpio_set_dir(LED_VERDE, GPIO_OUT);
    gpio_init(LED_AZUL);
    gpio_set_dir(LED_AZUL, GPIO_OUT);

    // Inicializar Buzzer como PWM
    gpio_set_function(buzz, GPIO_FUNC_PWM);
    uint slice = pwm_gpio_to_slice_num(buzz);
    pwm_set_clkdiv(slice, 16.0); // Ajuste do divisor do clock
    uint32_t periodo_pwm = 125000000 / 1000;
    pwm_set_wrap(slice, periodo_pwm);

    // Inicialização dos botões como entrada
    gpio_init(BTN_PIN_A);
    gpio_set_dir(BTN_PIN_A, GPIO_IN);
    gpio_pull_up(BTN_PIN_A); // Pull-up para o botão A

    gpio_init(BTN_PIN_B);
    gpio_set_dir(BTN_PIN_B, GPIO_IN);
    gpio_pull_up(BTN_PIN_B); // Pull-up para o botão B

    gpio_init(BOTAO_JOYSTICK);
    gpio_set_dir(BOTAO_JOYSTICK, GPIO_IN);
    gpio_pull_up(BOTAO_JOYSTICK); // Pull-up para o botão JoyStick

    // Configurar Eixos do JoyStick
    adc_init();
    adc_gpio_init(EIXO_X);
    adc_gpio_init(EIXO_Y);

    // Configurar leds no PWM
    configurar_pwm(LED_AZUL);
    configurar_pwm(LED_VERMELHO);

    // configurações da PIO
    uint offset = pio_add_program(pio, &pio_matrix_program);
    sm = pio_claim_unused_sm(pio, true);
    pio_matrix_program_init(pio, sm, offset, OUT_PIN);
}
// Função para ligar os leds solicitados
void set_leds(bool red, bool green, bool blue)
{
    gpio_put(LED_VERMELHO, red);
    gpio_put(LED_VERDE, green);
    gpio_put(LED_AZUL, blue);
}

// Tratamento de interrupções dos botões
static void gpio_irq_handler(uint gpio, uint32_t events)
{
    uint64_t current_time = to_ms_since_boot(get_absolute_time()); // Obtém o tempo atual em milissegundos desde o boot do Raspberry Pi Pico

    if (gpio == BTN_PIN_A)
    {
        if ((current_time - last_interrupt_time_A) > DEBOUNCE_DELAY)
        {
            if (a <= 9)
            {
                a++;                                                   // Incrementa o número
                desenho_pio(desenhos[a], valor_led, pio, sm, r, g, b); // Chamada da função para imprimir os desenhos
            }
            last_interrupt_time_A = current_time;
            printf("a incrementado para: %i\n", a);
        }
    }
    else if (gpio == BTN_PIN_B)
    {
        if ((current_time - last_interrupt_time_B) > DEBOUNCE_DELAY)
        {
            if (a > 0)
            {
                a--;                                                   // Decrementa o número
                desenho_pio(desenhos[a], valor_led, pio, sm, r, g, b); // Chamada da função para imprimir os desenhos
            }
            last_interrupt_time_B = current_time;
            printf("a decrementado para: %i\n", a);
        }
    }
    else if (gpio == BOTAO_JOYSTICK)
    {
        if ((current_time - last_interrupt_time_JoyStick) > DEBOUNCE_DELAY)
        {
            if (contSenha < 4)
            {
                senha[contSenha] = a;
            }

            contSenha++;

            if (contSenha == 5) // Após inserir os 5 dígitos
            {
                bool senhaCorreta = true;
                for (int i = 0; i < 4; i++)
                {
                    if (senha[i] != senhaCerta[i])
                    {
                        senhaCorreta = false;
                        break;
                    }
                }

                if (senhaCorreta)
                {
                    beep(1000, 500); // Toca um beep de 500ms a 1kHz
                }

                // Resetar senha após verificação
                contSenha = 0;
                for (int i = 0; i < 4; i++)
                {
                    senha[i] = 0;
                }
            }
        }
        last_interrupt_time_JoyStick = current_time;
    }
}

// Converte valores RGB para o formato aceito pela matriz
uint32_t matrix_rgb(double b, double r, double g)
{
    unsigned char R, G, B;
    R = r * 255;

    G = g * 255;
    B = b * 255;
    return (G << 24) | (R << 16) | (B << 8);
}

// Função para enviar o desenho para a matriz de LEDs
void desenho_pio(double *desenho, uint32_t valor_led, PIO pio, uint sm, double r, double g, double b)
{
    for (int16_t i = 0; i < NUM_PIXELS; i++)
    {
        valor_led = matrix_rgb(desenho[24 - i], desenho[24 - i], g = 0.0);
        pio_sm_put_blocking(pio, sm, valor_led);
    }
}

void configurar_pwm(uint gpio)
{ // Configura o PWM para um determinado pino
    gpio_set_function(gpio, GPIO_FUNC_PWM);
    uint slice = pwm_gpio_to_slice_num(gpio);
    pwm_set_clkdiv(slice, DIVISOR_PWM);
    pwm_set_wrap(slice, PERIODO_PWM);
    pwm_set_enabled(slice, true);
}

void ConfigurarDisplay()
{
    i2c_init(PORTA_I2C, 400 * 1000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);
    ssd1306_init(&display, WIDTH, HEIGHT, false, endereco, PORTA_I2C);
    ssd1306_fill(&display, false);
    ssd1306_send_data(&display);
}

// Atualiza as informações do display
void atualizar_display(uint16_t x, uint16_t y)
{
    char str_x[5], str_y[5], str_senha[5];
    sprintf(str_x, "%d", x);
    sprintf(str_y, "%d", y);
    sprintf(str_senha, "%d%d%d%d", senha[0], senha[1], senha[2], senha[3]);

    ssd1306_fill(&display, false);
    ssd1306_draw_string(&display, "X:", 10, 10);
    ssd1306_draw_string(&display, str_x, 40, 10);
    ssd1306_draw_string(&display, "Y:", 10, 25);
    ssd1306_draw_string(&display, str_y, 40, 25);
    ssd1306_draw_string(&display, "SENHA:", 10, 40);
    ssd1306_draw_string(&display, str_senha, 70, 40);
    ssd1306_send_data(&display);
}

// Calcula o nível PWM baseado no valor do joystick
uint16_t calcular_pwm(uint16_t valor)
{
    if (valor >= 2048)
    {
        return (valor - 2048) * 2;
    }
    else
    {
        return (2048 - valor) * 2;
    }
}

volatile bool beep_ativo = false;

void beep(uint freq, uint duration)
{
    if (!beep_ativo)
    {
        beep_ativo = true;
        uint slice = pwm_gpio_to_slice_num(buzz);
        pwm_set_wrap(slice, 125000000 / freq);
        pwm_set_gpio_level(buzz, (125000000 / (freq * 2)) / 2); // Garante um ciclo de trabalho de 50%
        pwm_set_enabled(slice, true);

        // Timer para desligar o buzzer após "duration" milissegundos
        add_alarm_in_ms(duration, beep_stop_callback, NULL, false);
    }
}

// Callback para desligar o beep
int64_t beep_stop_callback(alarm_id_t id, void *user_data)
{
    uint slice = pwm_gpio_to_slice_num(buzz);
    pwm_set_gpio_level(buzz, 0); // Garante que o buzzer pare
    pwm_set_enabled(slice, false);
    beep_ativo = false;
    return 0;
}