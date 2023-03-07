/*
 * Projekt z Zastosowania Procesor�w Sygna�owych
 * Szablon projektu dla DSP TMS320C5535
 * Micha� Ostrowski
 */


// Do��czenie wszelkich potrzebnych plik�w nag��wkowych
#include "usbstk5515.h"
#include "usbstk5515_led.h"
#include "aic3204.h"
#include "PLL.h"
#include "bargraph.h"
#include "oled.h"
#include "pushbuttons.h"
#include "dsplib.h"


// Cz�stotliwo�� pr�bkowania (48 kHz)
#define CZESTOTL_PROBKOWANIA 48000L

// Krok przyrostu amplitudy (policzone r�cznie 137)
#define PRZYROST 137

// Wzmocnienie wej�cia w dB: 0 dla wej�cia liniowego, 30 dla mikrofonowego
#define WZMOCNIENIE_WEJSCIA_dB 30

// Wymiar tablicy na pr�bki
#define ROZMIAR_TABLICY 5000

// Pr�g do fali prostok�tnej (0 dla 50%, 16384 dla 25%)
#define EDGE 0

// Bufor na pr�bki
Int16 bufor[ROZMIAR_TABLICY];

// Indeks bufora
int bufor_index = 0;

// G�owna procdura programu

void main(void) {

	// Inicjalizacja uk�adu DSP
	USBSTK5515_init();			// BSL
	pll_frequency_setup(100);	// PLL 100 MHz
	aic3204_hardware_init();	// I2C
	aic3204_init();				// kodek d�wi�ku AIC3204
	USBSTK5515_ULED_init();		// diody LED
	SAR_init_pushbuttons();		// przyciski
	oled_init();				// wy�wielacz LED 2x19 znak�w

	//Inicjalizacja generatora liczb pseudolosowych
	rand16init();

	// ustawienie cz�stotliwo�ci pr�bkowania i wzmocnienia wej�cia
	set_sampling_frequency_and_gain(CZESTOTL_PROBKOWANIA, WZMOCNIENIE_WEJSCIA_dB);

	// wypisanie komunikatu na wy�wietlaczu
	// 2 linijki po 19 znak�w, tylko wielkie angielskie litery
	oled_display_message("PROJEKT ZPS        ", "                   ");

	// 'krok' oznacza tryb pracy wybrany przyciskami
	unsigned int krok = 0;
	unsigned int poprzedni_krok = 99;

	// zmienne do przechowywania warto�ci pr�bek
	//Int16 lewy_wejscie;
	//Int16 prawy_wejscie;
	Int16 lewy_wyjscie;
	Int16 prawy_wyjscie;
	//Int16 mono_wejscie;

	// Zmienna okre�laj�ca amplitud� sygna�u tr�jk�tnego
	Int16 amplituda = 0;

	// Przetwarzanie pr�bek sygna�u w p�tli
	while (1) {

		// odczyt pr�bek audio, zamiana na mono
		//aic3204_codec_read(&lewy_wejscie, &prawy_wejscie);
		//mono_wejscie = (lewy_wejscie >> 1) + (prawy_wejscie >> 1);

		// sprawdzamy czy wci�ni�to przycisk
		// argument: maksymalna liczba obs�ugiwanych tryb�w
		krok = pushbuttons_read(4);
		if (krok == 0) // oba wci�ni�te - wyj�cie
			break;
		else if (krok != poprzedni_krok) {
			// nast�pi�a zmiana trybu - wci�ni�to przycisk
			USBSTK5515_ULED_setall(0x0F); // zgaszenie wszystkich di�d
			if (krok == 1) {
				// wypisanie informacji na wy�wietlaczu
				oled_display_message("PROJEKT ZPS        ", "KROK 1 SAWTOOTH    ");
				// zapalenie diody nr 1
				USBSTK5515_ULED_on(0);
			} else if (krok == 2) {
				oled_display_message("PROJEKT ZPS        ", "KROK 2 SQUARE WAVE ");
				USBSTK5515_ULED_on(1);
			} else if (krok == 3) {
				oled_display_message("PROJEKT ZPS        ", "KROK 3 SINE WAVE   ");
				USBSTK5515_ULED_on(2);
			} else if (krok == 4) {
				oled_display_message("PROJEKT ZPS        ", "KROK 4 WHITE NOISE ");
				USBSTK5515_ULED_on(3);
			}
			// zapisujemy nowo ustawiony tryb
			poprzedni_krok = krok;
		}

		// Generowanie pr�bek sygna�u tr�jk�tnego
		amplituda = amplituda + PRZYROST;

		// zadadnicze przetwarzanie w zale�no�ci od wybranego kroku

		if (krok == 1){
			lewy_wyjscie = amplituda;
			prawy_wyjscie = amplituda;

		}
		else if (krok == 2){
			if(amplituda > EDGE){
				lewy_wyjscie = 32767;
				prawy_wyjscie = lewy_wyjscie;
			}
			else{
				lewy_wyjscie = -32768;
				prawy_wyjscie = lewy_wyjscie;
			}
		}
		else if (krok == 3){
			sine((DATA*)&amplituda, (DATA*)&lewy_wyjscie, 1);
			prawy_wyjscie = lewy_wyjscie;
		}
		else if (krok == 4){
			rand16((DATA*)&lewy_wyjscie, 1);
			prawy_wyjscie = lewy_wyjscie;
		}

		//Zapisanie pr�bki do bufora
		bufor[bufor_index] = lewy_wyjscie;

		//Przesuni�cie indeksu bufora
		//bufor_index = _circ_incr(bufor_index, 1, ROZMIAR_TABLICY);
		bufor_index++;
		if(bufor_index == 5000){
			bufor_index = 0;
		}

		// zapisanie warto�ci na wyj�cie audio
		aic3204_codec_write(lewy_wyjscie, prawy_wyjscie);

	}


	// zako�czenie pracy - zresetowanie kodeka
	aic3204_disable();
	oled_display_message("KONIEC PRACY       ", "                   ");
	//SW_BREAKPOINT;
}
