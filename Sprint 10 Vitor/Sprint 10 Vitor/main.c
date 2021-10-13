// Laboratório de Arquitetura de Sistemas Digitais
// Sprint 10
// Vitor Freire Bezerra - 120111272



#define F_CPU 16000000UL
#include <avr/interrupt.h>
#include <util/delay.h>
#include <avr/io.h>
#include <stdio.h>
#include "nokia5110.h"

//Definindo macros
#define tst_bit(y,bit) (y&(1<<bit))//retorna 0 ou 1 

typedef enum
{
	tela1, tela2, tela3, tela4 ,size_enumSelet
} enum_selet;

uint8_t verde = 1;
uint8_t amarelo = 1;
uint8_t verm = 1;
uint32_t time_ms = 0;
uint16_t numero_carros = 0;
enum_selet selet_tela = 0;
uint8_t selet_modo = 0;
uint8_t att_tela = 0;
uint8_t flagLUX = 0;
uint16_t lux = 0;
uint8_t pessoa = 0;
uint8_t frequecia = 0;	
uint8_t periodo = 0;	


void switch_display (uint32_t tempo);
void LCD_nokia (uint8_t *tela_atualizada);
void leituraLUX(uint8_t *flag_lux);

ISR(TIMER0_COMPA_vect)
{
	time_ms++;  // Add 1ms

	if (time_ms % 50){
		att_tela = 1;
	}

	if (time_ms % 300)
	flagLUX = 1;

	if(time_ms % periodo)	// tempo em que irá permanecer ligado / desligado
	frequecia = !frequecia;		

}

ISR(PCINT1_vect)
{
	pessoa = !pessoa;
}

ISR(PCINT2_vect)
{
	

	if(!tst_bit(PIND,PD2)) // Para o sensor de carros
	{
		static uint32_t carro_anterior = 0;
		numero_carros = 60000/((time_ms - carro_anterior));
		carro_anterior = time_ms;
	}
	else if(!tst_bit(PIND,PD4)) // Botão (+)
	{
		switch (selet_tela){
			case tela1:
			selet_modo = !selet_modo;
			break;
			case tela2:
			if(verde<9)
			verde++;
			break;
			case tela3:
			if(verm<9)
			verm++;
			break;
			case tela4:
			if(amarelo<9)
			amarelo++;
			break;
		}
		_delay_ms(200);
	}
	else if(!tst_bit(PIND,PD5)) // Botão (-)
	{
		switch (selet_tela){
			case tela1:
			selet_modo = !selet_modo;
			break;
			case tela2:
			if(verde>1)
			verde--;
			break;
			case tela3:
			if(verm>1)
			verm--;
			break;
			case tela4:if(amarelo>1)
			amarelo--;
			break;
		}
		_delay_ms(200);
	}
	else if(!tst_bit(PIND,PD6)) // Botão de seleção (S)
	{
		static uint8_t flag_tela = 0;
		if (selet_modo)
		{
			if (flag_tela)
			{
				if (selet_tela < (size_enumSelet-1))
				selet_tela++;
				else
				selet_tela = 0;
			}
			flag_tela = !flag_tela;
		}
		_delay_ms(200);
	}

}

int main(void){

	// Config. GPIO
	DDRB = 0b11111111; // Habilita os pinos PB0 ao PB7 todos como saida
	DDRD = 0b10001011; // Habilita os pinos PD7, PD3, PD1 e PD0 como saídas
	PORTD = 0b01110100; // Habilita o resistor de pull up dos pinos PD2, PD4, PD5 e PD6
	DDRC = 0b01000000; // Habilita o pino PC6 como saida
	PORTC = 0b01000000; // Habilita o resistor de pull up do pino PC6

	//Configs das interrupções
	PCICR  = 0b00000110;
	PCMSK1 = 0b01000000;
	PCMSK2 = 0b01110100;

	
	TCCR0A = 0b00000010; // habilita o CTC
	TCCR0B = 0b00000011; // Liga TC0 com prescaler = 64
	OCR0A  = 249;        // ajusta o comparador para TC0 contar até 249
	TIMSK0 = 0b00000010; 

	
	ADMUX = 0b01000000; // Vcc com referencia canal PC0
	ADCSRA= 0b11100111; 
	ADCSRB= 0b00000000; // Modo de conversão contínua
	DIDR0 = 0b00000000; // Pinos PC0 e PC1 como entrada digitais

	//CONFIGURAÇÃO PWM
	TCCR2A = 0b10100011;
	TCCR2B = 0b00000101;
	OCR2B = 0;

	sei(); //Para habilitar interrupções globais

	nokia_lcd_init();

	while (1){

		switch_display (time_ms); // Valor do tempo para que o LED possa ser atualizado
		LCD_nokia (&att_tela);
		leituraLUX (&flagLUX);
	}

}

void switch_display (uint32_t tempo){ // Para atualizar o tempo dos leds

	const uint16_t estados[9] = {0b000001111, 0b000000111, 0b000000011, 0b000000001, 0b100000000, 0b011110000, 0b001110000, 0b000110000, 0b000010000};
	static uint8_t i = 0;
	static uint32_t tempo_anterior_ms = 0;
	periodo = i*1000; // Para a frequência variar

	// Semáforos: Pedestres
	if (estados[i] & 0b000001111)	// se o semáforo estiver verde 
	PORTD |= (1<<0);	// nível de tensão PD0 será aumentado
	if (estados[i] & 0b011110000)	// se o semáforo estiver vermelho
	PORTD &= 0b11111110;	// nível de tensão PD0 será diminuído

	
	PORTB = estados[i] & 0b011111111;
	if (estados[i] & 0b100000000)
	PORTD |= (1<<7); 
	else
	PORTD &= 0b01111111; 
	
	/* Leds verdes */
	if (i<=3)
	{
		PORTD &= 0b11111101; //desativa PD1
		if ((tempo - tempo_anterior_ms) >= (verde*250))
		{
			i++;
			tempo_anterior_ms += (verde*250);
		}
	}
	else
	{
		/* Leds amarelos */
		if(i<=4)
		{
			if((tempo - tempo_anterior_ms) >= (amarelo*1000))
			{
				i++;
				tempo_anterior_ms += (amarelo*1000);

			}
		}
		else
		{
			/* Leds vermelhos */
			if(i<=8)
			{
				// para simular a velocidade de batidas
				if(frequecia)
				PORTD |= (1<<1);
				else
				PORTD &= 0b11111101;
				
				if((tempo - tempo_anterior_ms) >= (verm*250))
				{
					i++;
					tempo_anterior_ms += (verm*250);
				}
			}
			else
			{
				i=0;
				tempo_anterior_ms = tempo;
			}
		}}}

		void LCD_nokia (uint8_t *tela_atualizada){ // Atualização do display com base no tempo de cada LED

			unsigned char verde_strig[3];
			unsigned char verme_strig[3];
			unsigned char amare_strig[3];
			unsigned char num_carro_strig[5];
			unsigned char inten_lux[4];
			static unsigned char modo_string;

			if (selet_modo)
			modo_string = 'M';
			else{
				modo_string = 'A';
				verde = 9 - (numero_carros/60);
				verm = (numero_carros/60) + 1;
			}

			sprintf (&verde_strig, "%u", verde);
			sprintf (&verme_strig, "%u", verm); // Conversão: Variáveis inteiras -> Char
			sprintf (&amare_strig, "%u", amarelo);
			sprintf (&num_carro_strig, "%u", numero_carros);
			sprintf (&inten_lux, "%u", lux);

			if(*tela_atualizada)
			{
				switch(selet_tela){
					case tela1:
					nokia_lcd_clear();
					nokia_lcd_draw_Hline(0,47,40); //(Y_inicial, X, Y_final)

					nokia_lcd_set_cursor(0, 5);
					nokia_lcd_write_string("Modo", 1);
					nokia_lcd_set_cursor(30, 5);
					nokia_lcd_write_char(modo_string, 1);
					nokia_lcd_set_cursor(42, 5);
					nokia_lcd_write_string("<", 1);

					nokia_lcd_set_cursor(0, 15);
					nokia_lcd_write_string("T.Vd", 1);
					nokia_lcd_set_cursor(30,15);
					nokia_lcd_write_string(verde_strig, 1);
					nokia_lcd_set_cursor(35,15);
					nokia_lcd_write_string("s", 1);

					nokia_lcd_set_cursor(50,0);
					nokia_lcd_write_string(inten_lux, 2);
					nokia_lcd_set_cursor(50,15);
					nokia_lcd_write_string("lux", 1);

					nokia_lcd_set_cursor(50,25);
					nokia_lcd_write_string(num_carro_strig, 2);
					nokia_lcd_set_cursor(50,40);
					nokia_lcd_write_string("c/min", 1);

					nokia_lcd_set_cursor(0, 25);
					nokia_lcd_write_string("T.Vm", 1);
					nokia_lcd_set_cursor(30,25);
					nokia_lcd_write_string(verme_strig, 1);
					nokia_lcd_set_cursor(35,25);
					nokia_lcd_write_string("s", 1);

					nokia_lcd_set_cursor(0,35);
					nokia_lcd_write_string("T.Am", 1);
					nokia_lcd_set_cursor(30,35);
					nokia_lcd_write_string(amare_strig, 1);
					nokia_lcd_set_cursor(35,35);
					nokia_lcd_write_string("s", 1);

					nokia_lcd_render();
					break;

					case tela2:
					nokia_lcd_clear();
					nokia_lcd_draw_Hline(0,47,40); //(Y_inicial, X, Y_final)

					nokia_lcd_set_cursor(0, 5);
					nokia_lcd_write_string("Modo", 1);
					nokia_lcd_set_cursor(30, 5);
					nokia_lcd_write_char(modo_string, 1);

					nokia_lcd_set_cursor(0, 15);
					nokia_lcd_write_string("T.Vd", 1);
					nokia_lcd_set_cursor(30,15);
					nokia_lcd_write_string(verde_strig, 1);
					nokia_lcd_set_cursor(35,15);
					nokia_lcd_write_string("s", 1);
					nokia_lcd_set_cursor(42, 15);
					nokia_lcd_write_string("<", 1);

					nokia_lcd_set_cursor(50,0);
					nokia_lcd_write_string(inten_lux, 2);
					nokia_lcd_set_cursor(50,15);
					nokia_lcd_write_string("lux", 1);

					nokia_lcd_set_cursor(50,25);
					nokia_lcd_write_string(num_carro_strig, 2);
					nokia_lcd_set_cursor(50,40);
					nokia_lcd_write_string("c/min", 1);

					nokia_lcd_set_cursor(0, 25);
					nokia_lcd_write_string("T.Vm", 1);
					nokia_lcd_set_cursor(30,25);
					nokia_lcd_write_string(verme_strig, 1);
					nokia_lcd_set_cursor(35,25);
					nokia_lcd_write_string("s", 1);

					nokia_lcd_set_cursor(0,35);
					nokia_lcd_write_string("T.Am", 1);
					nokia_lcd_set_cursor(30,35);
					nokia_lcd_write_string(amare_strig, 1);
					nokia_lcd_set_cursor(35,35);
					nokia_lcd_write_string("s", 1);

					nokia_lcd_render();
					break;

					case tela3:
					nokia_lcd_clear();
					nokia_lcd_draw_Hline(0,47,40); //(Y_inicial, X, Y_final)

					nokia_lcd_set_cursor(0, 5);
					nokia_lcd_write_string("Modo", 1);
					nokia_lcd_set_cursor(30, 5);
					nokia_lcd_write_char(modo_string, 1);

					nokia_lcd_set_cursor(0, 15);
					nokia_lcd_write_string("T.Vd", 1);
					nokia_lcd_set_cursor(30,15);
					nokia_lcd_write_string(verde_strig, 1);
					nokia_lcd_set_cursor(35,15);
					nokia_lcd_write_string("s", 1);

					nokia_lcd_set_cursor(50,0);
					nokia_lcd_write_string(inten_lux, 2);
					nokia_lcd_set_cursor(50,15);
					nokia_lcd_write_string("lux", 1);

					nokia_lcd_set_cursor(50,25);
					nokia_lcd_write_string(num_carro_strig, 2);
					nokia_lcd_set_cursor(50,40);
					nokia_lcd_write_string("c/min", 1);

					nokia_lcd_set_cursor(0, 25);
					nokia_lcd_write_string("T.Vm", 1);
					nokia_lcd_set_cursor(30,25);
					nokia_lcd_write_string(verme_strig, 1);
					nokia_lcd_set_cursor(35,25);
					nokia_lcd_write_string("s", 1);
					nokia_lcd_set_cursor(42, 25);
					nokia_lcd_write_string("<", 1);

					nokia_lcd_set_cursor(0,35);
					nokia_lcd_write_string("T.Am", 1);
					nokia_lcd_set_cursor(30,35);
					nokia_lcd_write_string(amare_strig, 1);
					nokia_lcd_set_cursor(35,35);
					nokia_lcd_write_string("s", 1);

					nokia_lcd_render();
					break;

					case tela4:
					nokia_lcd_clear();
					nokia_lcd_draw_Hline(0,47,40); //(Y_inicial, X, Y_final)

					nokia_lcd_set_cursor(0, 5);
					nokia_lcd_write_string("Modo", 1);
					nokia_lcd_set_cursor(30, 5);
					nokia_lcd_write_char(modo_string, 1);

					nokia_lcd_set_cursor(0, 15);
					nokia_lcd_write_string("T.Vd", 1);
					nokia_lcd_set_cursor(30,15);
					nokia_lcd_write_string(verde_strig, 1);
					nokia_lcd_set_cursor(35,15);
					nokia_lcd_write_string("s", 1);

					nokia_lcd_set_cursor(50,0);
					nokia_lcd_write_string(inten_lux, 2);
					nokia_lcd_set_cursor(50,15);
					nokia_lcd_write_string("lux", 1);

					nokia_lcd_set_cursor(50,25);
					nokia_lcd_write_string(num_carro_strig, 2);
					nokia_lcd_set_cursor(50,40);
					nokia_lcd_write_string("c/min", 1);

					nokia_lcd_set_cursor(0, 25);
					nokia_lcd_write_string("T.Vm", 1);
					nokia_lcd_set_cursor(30,25);
					nokia_lcd_write_string(verme_strig, 1);
					nokia_lcd_set_cursor(35,25);
					nokia_lcd_write_string("s", 1);

					nokia_lcd_set_cursor(0,35);
					nokia_lcd_write_string("T.Am", 1);
					nokia_lcd_set_cursor(30,35);
					nokia_lcd_write_string(amare_strig, 1);
					nokia_lcd_set_cursor(35,35);
					nokia_lcd_write_string("s", 1);
					nokia_lcd_set_cursor(42, 35);
					nokia_lcd_write_string("<", 1);

					nokia_lcd_render();
					break;
				}
			}
			*tela_atualizada = !*tela_atualizada;
		}

		void leituraLUX(uint8_t *flag_lux){
			static float testelux = 0;

			if(*flag_lux)
			{
				testelux = ADC;
				lux = (1023000/testelux)-1000;

				if (lux < 300){
					if(pessoa == 1 || numero_carros > 0)
					OCR2B = 255;
					else if(pessoa == 0 || numero_carros <= 0)
					OCR2B = 77;
				}
				else
				OCR2B = 0;
			}
			*flag_lux = 0;
		}
