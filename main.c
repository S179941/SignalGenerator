/*
 * Projekt z Zastosowania Procesorów Sygna³owych
 * Szablon projektu dla DSP TMS320C5535
 * Micha³ Ostrowski
 */


// Do³¹czenie wszelkich potrzebnych plików nag³ówkowych
#include "usbstk5515.h"
#include "usbstk5515_led.h"
#include "aic3204.h"
#include "PLL.h"
#include "bargraph.h"
#include "oled.h"
#include "pushbuttons.h"
#include "dsplib.h"


// Czêstotliwoœæ próbkowania (48 kHz)
#define CZESTOTL_PROBKOWANIA 48000L

// Krok przyrostu amplitudy (policzone rêcznie 137)
#define PRZYROST 137

// Wzmocnienie wejœcia w dB: 0 dla wejœcia liniowego, 30 dla mikrofonowego
#define WZMOCNIENIE_WEJSCIA_dB 30

// Wymiar tablicy na próbki
#define ROZMIAR_TABLICY 5000

// Próg do fali prostok¹tnej (0 dla 50%, 16384 dla 25%)
#define EDGE 0

// Bufor na próbki
Int16 bufor[ROZMIAR_TABLICY];

// Indeks bufora
int bufor_index = 0;

// G³owna procdura programu

void main(void) {

	// Inicjalizacja uk³adu DSP
	USBSTK5515_init();			// BSL
	pll_frequency_setup(100);	// PLL 100 MHz
	aic3204_hardware_init();	// I2C
	aic3204_init();				// kodek dŸwiêku AIC3204
	USBSTK5515_ULED_init();		// diody LED
	SAR_init_pushbuttons();		// przyciski
	oled_init();				// wyœwielacz LED 2x19 znaków

	//Inicjalizacja generatora liczb pseudolosowych
	rand16init();

	// ustawienie czêstotliwoœci próbkowania i wzmocnienia wejœcia
	set_sampling_frequency_and_gain(CZESTOTL_PROBKOWANIA, WZMOCNIENIE_WEJSCIA_dB);

	// wypisanie komunikatu na wyœwietlaczu
	// 2 linijki po 19 znaków, tylko wielkie angielskie litery
	oled_display_message("PROJEKT ZPS        ", "                   ");

	// 'krok' oznacza tryb pracy wybrany przyciskami
	unsigned int krok = 0;
	unsigned int poprzedni_krok = 99;

	// zmienne do przechowywania wartoœci próbek
	//Int16 lewy_wejscie;
	//Int16 prawy_wejscie;
	Int16 lewy_wyjscie;
	Int16 prawy_wyjscie;
	//Int16 mono_wejscie;

	// Zmienna okreœlaj¹ca amplitudê sygna³u trójk¹tnego
	Int16 amplituda = 0;

	// Przetwarzanie próbek sygna³u w pêtli
	while (1) {

		// odczyt próbek audio, zamiana na mono
		//aic3204_codec_read(&lewy_wejscie, &prawy_wejscie);
		//mono_wejscie = (lewy_wejscie >> 1) + (prawy_wejscie >> 1);

		// sprawdzamy czy wciœniêto przycisk
		// argument: maksymalna liczba obs³ugiwanych trybów
		krok = pushbuttons_read(4);
		if (krok == 0) // oba wciœniête - wyjœcie
			break;
		else if (krok != poprzedni_krok) {
			// nast¹pi³a zmiana trybu - wciœniêto przycisk
			USBSTK5515_ULED_setall(0x0F); // zgaszenie wszystkich diód
			if (krok == 1) {
				// wypisanie informacji na wyœwietlaczu
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

		// Generowanie próbek sygna³u trójk¹tnego
		amplituda = amplituda + PRZYROST;

		// zadadnicze przetwarzanie w zale¿noœci od wybranego kroku

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

		//Zapisanie próbki do bufora
		bufor[bufor_index] = lewy_wyjscie;

		//Przesuniêcie indeksu bufora
		//bufor_index = _circ_incr(bufor_index, 1, ROZMIAR_TABLICY);
		bufor_index++;
		if(bufor_index == 5000){
			bufor_index = 0;
		}

		// zapisanie wartoœci na wyjœcie audio
		aic3204_codec_write(lewy_wyjscie, prawy_wyjscie);

	}


	// zakoñczenie pracy - zresetowanie kodeka
	aic3204_disable();
	oled_display_message("KONIEC PRACY       ", "                   ");
	//SW_BREAKPOINT;
}
