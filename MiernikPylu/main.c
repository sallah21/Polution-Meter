/*
 * MiernikPylu.c
 *
 * Created: 2022-05-31 12:10:32
 * Author : oem1
 */ 
#include <avr/io.h>

#define F_CPU 16000000UL
#include <avr/interrupt.h>
#define RS 0
#define RW 1
#define E 2
#define AK 3
#define PRZYCISK_DDR DDRB
#define PRZYCISK PORTB
#define DIODA_DDR DDRA
#define DIODA PORTA
#define przycisk 0
#define BAUD 9600UL
#define my_ubrr F_CPU/16/BAUD-1
#include <util/delay.h>

uint8_t klawisz = 0;
uint8_t isButtonOn = 1;
uint8_t tryb = 0;
uint8_t udrState = 0;
uint16_t pm2 = 0,pm10 = 0;

// Do i2c
void TWI_Start(){
TWCR = (1<<TWINT)|(1<<TWSTA)|(1<<TWEN);
while(!(TWCR&(1<<TWINT)));
}

void TWI_Stop(){
TWCR = (1<<TWINT)|(1<<TWSTO)|(1<<TWEN);
while(TWCR&(1<<TWSTO));
}

void TWI_Write(uint8_t data){
TWDR = data;
TWCR = (1<<TWINT)|(1<<TWEN);
while(!(TWCR&(1<<TWINT)));
}

void TWI_init(){
TWBR = 92;
TWCR = (1<<TWEA);
}
//

void LCD_write(uint8_t dane,uint8_t rs){
	uint8_t co;
	TWI_Start();
	TWI_Write(0x4e);
	co = 0;
	co |= (rs<<RS)|(1<<AK)|(1<<E)|(dane& 0xf0);
	TWI_Write(co);
	co&= ~(1<<E);
	TWI_Write(co);

	co = 0;
	co |= (rs<<RS)|(1<<AK)|(1<<E)|( (dane<<4) & 0xf0);
	TWI_Write(co);
	co&= ~(1<<E);
	TWI_Write(co);
	TWI_Stop();
	}

	LCD_init(){
	LCD_write(0x33,0);
	LCD_write(0x32,0);
	LCD_write(0x28,0);
	LCD_write(0x06,0);
	LCD_write(0x0f,0);
	LCD_write(0x01,0);
	_delay_ms(2);
}

void LCD_text(char *tab){
	static uint8_t i =0;
	while(*tab){
	LCD_write(*tab++,1);
	if(i++==15){
	LCD_write(0xc0,0);
	}
	if(i==32)
	{
	LCD_write(0x80,0);
	i=0;
	}

	}
}
ISR(TIMER0_OVF_vect){
	//Handling przyciskow
	if (!(PINB & ((1<<przycisk)))){
		switch(klawisz){
			case 0:
			klawisz =1;
			break;
			case 1:
			klawisz =2;
			break;	
		}
	}
		else{
			switch(klawisz){
				case 3:
				klawisz =4;
				break;
				case 4:
				klawisz =0;
				break;
			}
		}
}

ISR(USART_RXC_vect){
	//LCD_text(UDR);
	// Zamienic na global scope, zeby obslugiwac wyslanie tego w while, a nie w przerwaniu
	// !!! Moze ustawic flage zeby w while obsluzyc
	//static uint16_t recChecksum=0, checksum = 0,pm2,pm10;
	
	static uint16_t recChecksum=0, checksum = 0;
	
	//int x=UDR;
	//if((x & 0xff)==0x42){	
	checksum+=UDR;
	if(UDR  == 0x42)
	{
		udrState=1;
		//LCD_text("text ");	
	}
	if(UDR==0x4d && udrState==1){
		udrState=2;
	}
	if(udrState>=2){
		//LCD_text("text ");
		// *PRZED*
		// if(udrState==6 || udrState==7){
		// 	pm2 += UDR;
		// }
		// if(udrState==8 || udrState==9){
		// 	pm10 +=UDR;
		// }
		// *PO*
		if(udrState==6 ){
			pm2 |= UDR<<8;
		}
		if(udrState==7){
			pm2 |= UDR;
		}
		if(udrState==8){
			pm10 |= UDR<<8;
		}
		if(udrState==9){
			pm10 |= UDR;
		}
		if(udrState ==30 ){
			// recChecksum += UDR<<8;
			recChecksum |= UDR<<8;
		}
		if(udrState==31){
			// recChecksum +=UDR;
			recChecksum |= UDR;

		}
		if(udrState == 31 && checksum==recChecksum){
			LCD_write(0x01,0); // Wyczyszczenie 
			LCD_write(pm2/100,1); //XBC
			LCD_write(pm2%100/10,1); //AXC
			LCD_write(pm2%100%10,1); //ABX
			LCD_write(0x80,0); // Nowa linia
			LCD_write(pm10/100,1); //XBC
			LCD_write(pm10%100/10,1); //AXC
			LCD_write(pm10%100%10,1); //ABX
		}
		if(udrState==31 )
		{
			//naprawic checksum, zle zwraca wartosc, rozmiar zmiennej(?), do write jest uint8 a mamy 16 bitowe dane
			LCD_write(recChecksum<<8,1);
			LCD_write(recChecksum,1);
			LCD_text(" ");
			LCD_write(checksum<<8,1);
			LCD_write(checksum,1);
			recChecksum=0;
			checksum=0;
		}
		udrState++;
		//LCD_write(udrState+48,1);
	}
	//LCD_text(" ");

}

void USART_init(uint16_t ubrr){
	//UCSRB = (1<<TXEN); // wlaczenie nadajnika
	UCSRB = (1<<RXEN)|(1<<RXCIE);
	UCSRC = (1<<URSEL)|(1<<UCSZ1)|(1<<UCSZ0); // Ustawienie wielkosci pamieci (?)
	UBRRH = (uint8_t)(ubrr>>8); // zapisanie polowy 16 bitowej zmiennej na 8 bitowy rejestr
	UBRRL = (uint8_t)ubrr;
}



int main(void)
{
/* Replace with your application code */
	USART_init(my_ubrr);
		
	PRZYCISK |= (1<<przycisk); //przycisk na porcie 0
	TCCR0 = (1<<CS02)|(1<<CS00); // preskaler 1024
	TIMSK |= (1<<TOIE0); // WLACZENIE PRZERWANIA OVF T0
	sei();
	TWI_init();
	LCD_init();
	LCD_write(0x80,0);
	//LCD_text("Teleinformatykaawwww");
	DIODA_DDR = 0xff;
	DIODA |= (1<<7)|(1<<6);
	while (1)
	{
		if(klawisz==2){
			/* Do zamiany na zmiane trybu zamiast przelaczanie diody*/
			if(isButtonOn++==0) DIODA |= (1<<7);
			else {
				DIODA &= ~(1<<7);
				isButtonOn=0;
			}
			/**/
			klawisz=3;
		}
		if(tryb == 1){
			LCD_write(0x01,0); //Wyczyszczenie ekranu
			if(pm2>300)LCD_text("bardzo zle"); //itp itd
		}
	}
}

