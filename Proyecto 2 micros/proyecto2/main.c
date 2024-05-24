/*
/*
*UNIVERSIDAD DEL VALLE DE GUATEMALA
*IE2023: PROGRAMACIÓN DE MICROCONTROLADORES
*Lab2.asm
*AUTOR: Jose Andrés Velásquez Gacía
*PROYECTO: proyect2
*HARDWARE: ATMEGA328P
*CREADO: 30/04/2024
*ÚLTIMA MODIFICACIÓN: 30/04/2024 23:36*/
 */ 

//Librerías utl.
#define F_CPU 16000000
#include <avr/io.h>
#include <stdint.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <avr/eeprom.h>
#include <stdlib.h>
#include "PWM1/PWM1.h"
#include "PWM2/PWM2.h"

//Uso de Pines
#define botA1DC	PINB0 // activar 1er motor botones
#define servo1		PINB1
#define servo2		PINB2
#define PWM1DC		PINB3 //PWM 1 motor DC
#define botA2DC		PINB4 // activar segundo motor botones 
#define potServo1	PINC0
#define potServo2	PINC1
#define vel1DC	PINC2 //velocidad para 1 motor DC
#define vel2DC	PINC3 //velocidad para 2 motor DC
#define act1DCP		PIND2 //activar 1 DC desde progra
#define PWM2DC		PIND3 //PWM 2 motor DC
#define act2DCP		PIND4 //activar 2 DC desde progra
#define led1		PIND5
#define led2		PIND6
#define savePos		PIND7

//definición de constantes
#define manualMode	1
#define eepromMode	2
#define usartMode	3

//definición de variables
volatile char bufferRX;		//Es volatil cambia en cualquier tiempo
uint8_t count = 0;
uint8_t dir = 0;
uint8_t counterPos = 1;
uint8_t currentMode = manualMode;
uint8_t answer2 = 0;
uint8_t setServo1 = 0;
uint8_t setServo2 = 0;
uint8_t setPWM2DC = 0;
uint8_t setVel = 0;
uint8_t newAction = 0;

//definicion de funciones
void initADC(void);
void initPCINT(void);
void initUART9600(void);
void writeText(char* text);
void menu(void);
void sentChar(void);



int main(void){
	//limpio interrup.
	cli();	
	//Configuracion de pines entradas y salidas
	
	//configuracion PUERTOB 
	PORTB |= (1 << botA1DC) | (1 << botA2DC);	//ENTRADAS PULL-UP
	DDRB &= ~( (1 << botA1DC) | (1 << botA2DC) );
	DDRB |= (1 << servo1) | (1 << servo2);
	
	
	//CONFIG. PUERTOC, PUERTO COMPLETO COMO ENTRADA
	DDRC = 0;
	
	//CONFIG. PARA PUERTOD
	PORTD |= (1 << savePos);	//ENTRADA PULL-UP
	DDRD &= ~(1 << savePos);
	DDRD |= (1 << act1DCP) | (1 << act2DCP) | (1 <<  led1) | (1 << led2);
	
	//INI LIBERIAS Y OTRAS CONFIGURACIONS
	initADC();
	initPCINT();
	
	initUART9600();
	initFastPWM1(settedUp, 8);
	channel(channelA, nop);
	channel(channelB, nop);
	topValue(39999);	
	
	initPWM2A(no_invertido, 1024);
	initPWM2B(no_invertido, 1024);
	
	//HAB. INTRRUP GLOBALES
	sei();	
	writeText("Hello wolrd"); //MESAJE DE ENTRADA
	menu();
	
	ADCSRA |= (1 << ADSC);        //Conmienza la conversion
    PORTD |= (1 << led1);
	PORTD &= ~(1 << led2);
	while (1) 
    {
    }
}



//FUNCIONES Y MODULOS *****************************************************************************

void initADC(void){
	ADMUX = 0;
	//VONTAJE DE REFERENCIA 5Vs AVcc
	ADMUX |= (1 << REFS0);
	ADMUX &= ~(1 << REFS1);
	
	ADMUX |= (1 << ADLAR);	//AJUSTE PARA ADCH
	
	ADCSRA = 0;
	ADCSRA |= (1 << ADEN);	//ADC ON
	//INTERUPCION 
	ADCSRA |= (1 << ADIE);	
	
	//prescaler 128 > 125kHz
	ADCSRA |= (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);
	
	DIDR0 |= (1 << ADC0D) | (1 << ADC1D) | (1 << ADC2D) | (1 << ADC3D);	//DESACTIVAR DE PC0-PC3 ENTRADAS DIGITALES
}

ISR (ADC_vect){
	ADCSRA |= (1 << ADIF);	//APAGAR BANDERA
	if(currentMode == manualMode){
		if (count == 0){
			count = 1;
			ADMUX = (ADMUX & 0xF0);
			convertServo(ADCH, channelB);
		}else if(count == 1){
			count = 2;
			ADMUX = (ADMUX & 0xF0) | 1;
			convertServo(ADCH, channelA);
		}else if(count == 1){
			count = 3;
			ADMUX = (ADMUX & 0xF0) | 2;
			OCR2B = ADCH;
	    }else if(count == 3){
			count = 0;
			ADMUX = (ADMUX & 0xF0) | 3;
			OCR2A = ADCH;
		}
	}
	ADCSRA |= (1 << ADSC);		//EMPIEZA LA CONVERSION
}




void initPCINT(void){
	//PCINT0 Y PCINT2
	PCICR |= (1 << PCIE0) | (1 << PCIE2);		
	PCMSK0 |= (1 << PCINT0) | (1 << PCINT4);	//PB0 Y PB4
	PCMSK2 |= (1 << PCINT23);					//PD7
}

ISR (PCINT2_vect){
	//EN MODO MANUAL ENTRA EN FUNCIONAMIENTO
	if(currentMode == manualMode){
		
		if (!(PIND & (1 << PIND7))) {					//CAMBOS EN PINPD7
			
			if(counterPos == 1){
				counterPos = 2;							//SELECCIONA LA SIGUIENTE POSICION PRA GUARDADO
				writeText("\nGuardada posición 1\n\n");	//MENSAJE AFIRMATIVO 1
			}else if(counterPos == 2){
				counterPos = 3;							//SELECCIONA LA SIGUIENTE POSICION PRA GUARDADO
				writeText("\nGuardada posición 2\n\n");	//MENSAJE AFIRMATIVO 2
			}else if(counterPos == 3){
				counterPos = 4;							//SELECCIONA LA SIGUIENTE POSICION PRA GUARDADO
				writeText("\nGuardada posición 3\n\n");	//MENSAJE AFIRMATIVO 3
			}else if(counterPos == 4){
				counterPos = 1;							//SELECCIONA LA SIGUIENTE POSICION PRA GUARDADO
				writeText("\nGuardada posición 4\n\n");	//MENSAJE AFIRMATIVO 4
			}
			
		}
		
	}else{
		counterPos = 1;
	}
	
}


ISR (PCINT0_vect){
	if(currentMode == manualMode){
		if (!(PINB & (1 << PINB0))) {
			PORTD |= (1 << PORTD2) |(1 << PORTD4);// MODO MAN. CAMNIO EN EL BOTON PB0 SE ACTIVAN AMBOS DC
			dir = 1;
		
		}else if (PINB & (1 << PINB0)) {
			PORTD &= ~((1 << PORTD2) | (1 << PORTD4));//OTRO CAMBIO SOLTANDO BOTON PB0 SE DESACTIVAN LOS DC
			dir = 0;
		}
	}
}


void initUART9600(void){
	//CONFIG. RX ENTRADA Y TX SALIDA
	DDRD &= ~(1 << DDD0);		
	DDRD |= (1 << DDD1);		
	
	//Fast mode, U2X0
	UCSR0A = 0;
	UCSR0A |= (1 << U2X0);
	
	//CONFIG. REGISTRO B
	UCSR0B = 0;
	UCSR0B |= (1 << RXCIE0) | (1 << RXEN0) | (1 << TXEN0); //ISR HABILITA PAR RX Y TX
	
	//CONFIG. PARA REGISTRO C
	UCSR0C = 0;
	UCSR0C |= (1 << UCSZ01) | (1 << UCSZ00);		//TAMAÑO 8BITS, NO PARITY 1 BIT DE PARO
	
	//Baudrate
	UBRR0 = 207;		// 9600
}


void writeText(char* text){
	//uint8_t i;
	for (uint8_t i = 0; text[i] != '\0'; i++)
	{
		while(!(UCSR0A & (1 << UDRE0)));
		UDR0 = text[i];
	}
}


//ISR, RECIVE
ISR(USART_RX_vect){
	bufferRX = UDR0;
	
	//BUFFER ES 0 ENTONCES SI NO ESPERA
	while(!(UCSR0A & (1 << UDRE0)));
	char copy = bufferRX;
	uint8_t num = atoi(&copy);
	
	if (newAction == 0){
	
		if(answer2 == 0){
		
			if(num == manualMode){		//REVISA SI EL NUM RECIVIDO TIENE RELACION AL MANUAL
				PORTD |= (1 << led1);	//MUESTRA EL NUMERO DEL MODO ACTUAL
				PORTD &= ~(1 <<  led2);
				currentMode = manualMode;
				answer2 = 0;
				sentChar();
				writeText("\nMODO MAUAL\n");
				menu();
			}else if(num == eepromMode){
				PORTD |= (1 << led2);	//MIESTRA EL MODO ACTUAL
				PORTD &= ~(1 <<  led1);
				currentMode = eepromMode;
				answer2 = 0;
				sentChar();
				writeText("\nMODO MEM EEPROM\n");
				menu();
			}else if (num == usartMode){
				PORTD |= (1 << led2) | (1 <<  led1);	//MUESTRA EL NUMERO DEL MODO ACTUAL
				currentMode = usartMode;
				answer2  = 1;
				sentChar();
				writeText("\nMODO USART\n");
				writeText("\n\n******** ELIGA LA ACCION DESEADA ********\n");
				writeText("\t 1. Ir al modo manual\n");
				writeText("\t 2. Ir al modo EEPROM\n");
				writeText("\t 3. Control servo1 luces\n");
				writeText("\t 4. Control servo2 direccion\n");
				writeText("\t 5. Velocidad DC 1\n");				
				writeText("\t 6. Velocidad DC 2\n\n");
			}else{
				answer2 = 0;
				writeText("\n\nopcion desconocida\n\n");
				menu();
			}
		
		}else{
			sentChar();
			switch (num){
				case 1:
					PORTD |= (1 << led1);	//It shows the number of the current mode
					PORTD &= ~(1 <<  led2);
					currentMode = manualMode;
					answer2 = 0;
					//sentChar();
					writeText("\nMODO MAUAL SELECCIONADO\n");
					menu();
					break;
				
				case 2:
					PORTD |= (1 << led2);	//It shows the number of the current mode
					PORTD &= ~(1 <<  led1);
					currentMode = eepromMode;
					answer2 = 0;
					//sentChar();
					writeText("\nMODO EEPROM SELECCIONADO\n");
					menu();
					break;
			
				case 3:
					newAction = 1;
					setPWM2DC = 1;
					//sentChar();
					writeText("\nDirrecion selecionado\n");
					writeText("\n\n* selccione una direccion *\n");
					writeText("\t 1. izquiera\n");
					writeText("\t 2. derecha\n");
					writeText("\t 3. recto\n");
					break;
					
			
				case 4:
					newAction = 1;
					setPWM2DC = 1;
					//sentChar();
					writeText("\nDirrecion selecionado\n");
					writeText("\n\n* selccione una direccion *\n");
					writeText("\t 1. izquiera\n");
					writeText("\t 2. derecha\n");
					writeText("\t 3. recto\n");
					break;
					
				
				case 5:
					newAction = 1;
					setVel = 1;
					//sentChar();
					writeText("\nVelocidad seleccionada \n");
					writeText("\n\n* seleccione una accion *\n");
					writeText("\t 1. rapido\n");
					writeText("\t 2. lento\n");
					break;
				
				case 6:
					newAction = 1;
					setVel = 1;
					//sentChar();
					writeText("\nVelocity selected\n");
					writeText("\n\n* seleccione una accion*\n");
					writeText("\t 1. rapido\n");
					writeText("\t 2. lento\n");
					break;
				
				default: 
					newAction = 0;
					answer2 = 0;
					writeText("\n\n\t opcion invalida\n");
					break;
			}
		
		}
	}else{
		newAction = 0;
		answer2 = 0;
		sentChar();
		
		//control servo 1 seleccionado
		if(setServo1 == 1){
			setServo1 = 0;
			
			switch (num){
				case 1:
					writeText("\n Open door\n");
					break;
				
				case 2:
					writeText("\n Close door\n");
					break;
				
				default:
					writeText("\n Invalid option\n");
					break;
			}
		}
		
		//Control servo2 seleccionado
		if(setServo2 == 1){
			setServo2 = 0;
			
			switch (num){
				case 1:
					writeText("\n Open door\n");
					break;
				
				case 2:
					writeText("\n Close door\n");
					break;
				
				default:
					writeText("\n Invalid option\n");
					break;
			}
		}
		
		//control PWM2DC selected
		if(setPWM2DC == 1){
			setPWM2DC = 0;
			
			switch (num){
				case 1:
				writeText("\n izquiera\n");
				break;
				
				case 2:
				writeText("\n derecha\n");
				break;
				
				case 3:
				writeText("\n recto");
				break;
				
				default:
				writeText("\n Opcion invalida \n");
				break;
			}
		}
		
		
		if(setVel == 1){
			setVel = 0;
			
			switch (num){
				case 1:
				writeText("\n rapido\n");
				break;
				
				case 2:
				writeText("\n lento\n");
				break;
				
				default:
				writeText("\n opcion desconocidan\n");
				break;
			}
		}
		
		menu();		//muestra el menu 
	}
}


void menu(void){
	writeText("\n\n* seleccionar modo operando *\n");
	writeText("\t 1. mod Manual \n");
	writeText("\t 2. mod EEPROM \n");
	writeText("\t 3. mod USART \n\n");
}

void sentChar(void){
	UDR0 = bufferRX;
	writeText(" -> eniviar caracter");
}
//The end :D en nether
